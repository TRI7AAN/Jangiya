#pragma once
// JOCKY row-table pipeline engine — executes `| where/filter/having`,
// `| select`, `| sort_by`, `| limit`, `| emit`, and `| group_by`
// (group keys only; no aggregation functions exist in the grammar) over
// materialized call-output rows.
//
// Table model (documented, fail-closed):
// - A pipeline head's captured stdout is split into lines; a trailing
//   newline starts no phantom row. Each line is a row; commas split
//   fields (CSVs are the registry's output contract). Rows are kept as
//   raw text plus parsed fields.
// - Header detection: the first row is a header when it names at least
//   one field referenced by a later `select`/`sort_by`/`where` column,
//   or when it contains a non-numeric field while a later row is all
//   numeric. Otherwise all rows are data and columns are positional
//   (`col0`, `col1`, ...). Header detection never guesses silently: when
//   a column name matches nothing, the op aborts fail-closed.
// - `where`/`filter`/`having`: per-row predicate evaluation reusing the
//   predicate evaluator with row fields bound as string values (a field
//   reference `bytes` resolves to the row's `bytes` column; numeric
//   strings compare numerically via as_number, exactly like control-flow
//   conditions). List/call/source/correlate operands abort — they are
//   not row properties.
// - `select`: column projection by name (or `f as alias` renaming).
// - `sort_by`: stable lexicographic-or-numeric sort (numeric when both
//   sides parse as numbers) over the listed keys, `asc` (default) or
//   `desc`.
// - `limit N`: keep the first N rows.
// - `group_by`: validates the named columns exist (group keys) and
//   passes rows through in encounter order — there are no aggregation
//   functions in the grammar, so grouping is key validation only.
// - `emit name`: binds the table's current text under `name` for later
//   `count(name)` bounds and field references. `emit "string"` binds
//   nothing (legacy annotation form).
// - `write path` / `write report(path)`: the dispatcher already stages
//   declared write targets as outputs; here the table text is recorded
//   under the path so staged promotion carries computed (not raw) bytes.
//   Actual file promotion stays in the dispatcher.
//
// `correlate` as a pipe operator is rejected: joins need two named input
// tables and the pipe carries one. `correlate(...)` as an expression
// head is likewise rejected at execution: no join engine exists. Both
// abort loudly instead of passing data through unexamined.

#include <algorithm>
#include <cctype>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "jocky/ast/ast.hpp"
#include "jocky/runtime/predicate_evaluator.hpp"

namespace jocky {
namespace pipeline_detail {

struct RowTable {
    std::vector<std::string> columns;
    std::vector<std::vector<std::string>> rows;
    bool has_header = false;
};

inline std::vector<std::string> split_csv_row(const std::string& line) {
    std::vector<std::string> fields;
    std::string current;
    for (char c : line) {
        if (c == ',') {
            fields.push_back(current);
            current.clear();
        } else {
            current += c;
        }
    }
    fields.push_back(current);
    return fields;
}

inline bool looks_numeric(const std::string& text) {
    if (text.empty()) return false;
    std::size_t i = 0;
    if (text[i] == '+' || text[i] == '-') ++i;
    bool digits = false;
    bool dot = false;
    for (; i < text.size(); ++i) {
        const char c = text[i];
        if (c >= '0' && c <= '9') {
            digits = true;
        } else if (c == '.' && !dot) {
            dot = true;
        } else {
            return false;
        }
    }
    return digits;
}

// Collect every bare field name a predicate references (dotted paths
// use their first segment; call/list operands are rejected later).
inline void predicate_fields(const Predicate& pred,
                             std::vector<std::string>& out) {
    switch (pred.kind) {
        case Predicate::Kind::Or:
        case Predicate::Kind::And:
            for (const auto& operand : pred.operands) {
                predicate_fields(*operand, out);
            }
            return;
        case Predicate::Kind::Not:
            predicate_fields(*pred.inner, out);
            return;
        case Predicate::Kind::Compare:
            break;
        case Predicate::Kind::Atom:
            break;
    }
    auto note = [&](const std::unique_ptr<ExprValue>& value) {
        if (value != nullptr &&
            value->kind == ExprValue::Kind::Field &&
            !value->field.path.empty()) {
            out.push_back(value->field.path.front());
        }
    };
    if (pred.kind == Predicate::Kind::Compare) {
        note(pred.left);
        note(pred.right);
    } else {
        note(pred.atom);
    }
}

inline RowTable materialize(const std::string& text,
                            const std::vector<std::string>& wanted) {
    RowTable table;
    std::istringstream stream(text);
    std::string line;
    std::vector<std::vector<std::string>> parsed;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        parsed.push_back(split_csv_row(line));
    }
    if (parsed.empty()) return table;
    std::size_t width = 0;
    for (const auto& row : parsed) width = std::max(width, row.size());
    bool header = false;
    if (parsed.size() >= 1) {
        for (const std::string& name : wanted) {
            for (const std::string& field : parsed.front()) {
                if (field == name) {
                    header = true;
                    break;
                }
            }
            if (header) break;
        }
        if (!header && parsed.size() >= 2) {
            bool first_text = false;
            for (const std::string& field : parsed.front()) {
                if (!looks_numeric(field)) {
                    first_text = true;
                    break;
                }
            }
            bool second_numeric = true;
            for (const std::string& field : parsed[1]) {
                if (!field.empty() && !looks_numeric(field)) {
                    second_numeric = false;
                    break;
                }
            }
            header = first_text && second_numeric;
        }
    }
    std::size_t start = 0;
    if (header) {
        table.columns = parsed.front();
        table.has_header = true;
        start = 1;
    } else {
        for (std::size_t i = 0; i < width; ++i) {
            table.columns.push_back("col" + std::to_string(i));
        }
    }
    for (std::size_t i = start; i < parsed.size(); ++i) {
        std::vector<std::string> row = parsed[i];
        row.resize(width);
        table.rows.push_back(std::move(row));
    }
    return table;
}

inline int column_index(const RowTable& table, const std::string& name) {
    for (std::size_t i = 0; i < table.columns.size(); ++i) {
        if (table.columns[i] == name) return static_cast<int>(i);
    }
    return -1;
}

inline std::string join_row(const std::vector<std::string>& row) {
    std::string out;
    for (std::size_t i = 0; i < row.size(); ++i) {
        if (i != 0) out += ",";
        out += row[i];
    }
    return out;
}

inline std::string render(const RowTable& table) {
    std::string out;
    for (const auto& row : table.rows) {
        out += join_row(row);
        out += "\n";
    }
    return out;
}

// Evaluate one row predicate with row columns bound as string values.
// Numeric strings compare numerically (same as_number path the
// control-flow conditions use); anything unresolvable throws.
inline bool row_matches(const Predicate& pred, const RowTable& table,
                        const std::vector<std::string>& row) {
    ExecValues ctx;
    for (std::size_t i = 0; i < table.columns.size() && i < row.size();
         ++i) {
        ctx.bindings[table.columns[i]] = row[i];
    }
    try {
        return predicate_detail::evaluate_node(pred, ctx);
    } catch (const PredicateError& ex) {
        throw PredicateError(std::string("row predicate failed: ") +
                             ex.what());
    }
}

inline void apply_filter(RowTable& table, const Predicate& pred,
                         const char* op) {
    std::vector<std::string> wanted;
    predicate_fields(pred, wanted);
    for (const std::string& name : wanted) {
        if (column_index(table, name) < 0) {
            throw PredicateError(
                std::string(op) + " references unknown column '" + name +
                "' (available: " + join_row(table.columns) + ")");
        }
    }
    std::vector<std::vector<std::string>> kept;
    for (const auto& row : table.rows) {
        if (row_matches(pred, table, row)) kept.push_back(row);
    }
    table.rows = std::move(kept);
}

inline void apply_select(RowTable& table,
                         const std::vector<SelectItem>& items) {
    std::vector<std::string> names;
    std::vector<int> indexes;
    for (const SelectItem& item : items) {
        const std::string name = item.field.path.empty()
                                     ? ""
                                     : item.field.path.front();
        const int index = column_index(table, name);
        if (index < 0) {
            throw PredicateError(
                "select references unknown column '" + name +
                "' (available: " + join_row(table.columns) + ")");
        }
        indexes.push_back(index);
        names.push_back(item.has_alias ? item.alias : name);
    }
    table.columns = std::move(names);
    for (auto& row : table.rows) {
        std::vector<std::string> projected;
        for (int index : indexes) {
            projected.push_back(
                static_cast<std::size_t>(index) < row.size()
                    ? row[static_cast<std::size_t>(index)]
                    : "");
        }
        row = std::move(projected);
    }
}

inline void apply_sort(RowTable& table,
                       const std::vector<FieldRef>& keys,
                       const std::string& dir) {
    std::vector<int> indexes;
    for (const FieldRef& key : keys) {
        const std::string name =
            key.path.empty() ? "" : key.path.front();
        const int index = column_index(table, name);
        if (index < 0) {
            throw PredicateError(
                "sort_by references unknown column '" + name +
                "' (available: " + join_row(table.columns) + ")");
        }
        indexes.push_back(index);
    }
    const bool desc = (dir == "desc");
    std::stable_sort(table.rows.begin(), table.rows.end(),
                     [&](const std::vector<std::string>& a,
                         const std::vector<std::string>& b) {
                         for (int index : indexes) {
                             const std::size_t i =
                                 static_cast<std::size_t>(index);
                             const std::string& left =
                                 i < a.size() ? a[i] : "";
                             const std::string& right =
                                 i < b.size() ? b[i] : "";
                             if (left == right) continue;
                             const bool left_num = looks_numeric(left);
                             const bool right_num = looks_numeric(right);
                             bool less = false;
                             if (left_num && right_num) {
                                 less = std::stod(left) < std::stod(right);
                             } else {
                                 less = left < right;
                             }
                             return desc ? !less : less;
                         }
                         return false;
                     });
}

inline void apply_group(RowTable& table,
                        const std::vector<FieldRef>& keys) {
    for (const FieldRef& key : keys) {
        const std::string name =
            key.path.empty() ? "" : key.path.front();
        if (column_index(table, name) < 0) {
            throw PredicateError(
                "group_by references unknown column '" + name +
                "' (available: " + join_row(table.columns) + ")");
        }
    }
}

}  // namespace pipeline_detail
}  // namespace jocky
