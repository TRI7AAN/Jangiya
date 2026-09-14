#pragma once
// JOCKY runtime predicate evaluator — Phase 7.5 execution-time control flow.
//
// The static passes (resolve/bind/gate/shake) are deliberately
// control-flow-INSENSITIVE: they collect calls from both branches and
// every loop body (see ast/walker.hpp). This header is the other half:
// it evaluates a Predicate AST node to a concrete bool DURING execution,
// against values known at that point in the program. It is consumed only
// by runtime/control_flow_executor.hpp, never by any static pass — the
// two directions must not share logic, or a future static change could
// silently alter runtime branch selection.
//
// Value model (documented, fail-closed):
// - Literals evaluate to themselves. Lists evaluate element-wise.
// - Field references resolve through the execution context: the
//   control-flow executor records, for every `let name = call ...` it
//   dispatches, the call's captured stdout under `name`. A field naming
//   any other value (never bound, bound by a non-call pipeline whose
//   table was never materialized, or a dotted path that is not a plain
//   binding name) is a PredicateError — never a silent default. Table
//   contents are not computed by the dispatcher, so guessing would be
//   unsound; refusing loudly is the AGENTS.md §2 fail-closed posture.
// - Call operands resolve to the captured stdout of that exact call-site
//   node (keyed by its source line:col, recorded when the executor
//   dispatches predicate-embedded calls before evaluating). A call node
//   with no recorded output is a PredicateError.
// - `source` / `correlate` operands are never materializable at dispatch
//   and are always a PredicateError.
//
// Comparison semantics (strict, no silent coercion):
// - == / !=: numeric-numeric compares numerically (int/float mix freely);
//   string-string, bool-bool compare directly; list-list compares
//   element-wise with ==. Anything else is a PredicateError. In
//   particular a string never implicitly equals a number.
// - < <= > >=: numeric-numeric numerically; string-string
//   lexicographically (byte order); anything else is a PredicateError.
// - `in`: right side must be a list; membership uses == semantics.
// - `contains`: string-contains-string (substring), or list membership
//   (==). Anything else is a PredicateError.
// - `contains_any`: right side must be a list. Left string: true when any
//   element is a string substring of it. Left list: true on any shared
//   element (==). Anything else is a PredicateError.
// - Bare atoms use truthiness: bool as-is; int/float nonzero; string/list
//   non-empty.
//
// Any evaluation failure throws PredicateError. The executor treats it as
// fail-closed run abort (manifest-logged error, no further dispatch), so
// a branch is never chosen on a guess.

#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include "jocky/ast/ast.hpp"

namespace jocky {

// Thrown when a predicate cannot be decided from execution-time values.
// Carries no position (positions live on the AST nodes the executor
// already knows); the message always names what was missing.
class PredicateError : public std::runtime_error {
public:
    explicit PredicateError(const std::string& message)
        : std::runtime_error(message) {}
};

// Values known while a program runs: what the executor has observed so
// far (prior call outputs), keyed by binding name and by call-site key.
struct ExecValues {
    // `let name = call ...` outputs, by binding name.
    std::map<std::string, std::string> bindings;
    // Predicate-embedded call outputs, keyed "line:col" of the node.
    std::map<std::string, std::string> call_outputs;
};

namespace predicate_detail {

struct Value {
    enum class Kind { Str, Int, Float, Bool, List };
    Kind kind = Kind::Str;
    std::string str;
    std::int64_t integer = 0;
    double floating = 0.0;
    bool boolean = false;
    std::vector<Value> list;
};

inline std::string join_field_path(const FieldRef& field) {
    std::string out;
    for (std::size_t i = 0; i < field.path.size(); ++i) {
        if (i != 0) out += ".";
        out += field.path[i];
    }
    return out;
}

inline std::string call_key(int line, int col) {
    return std::to_string(line) + ":" + std::to_string(col);
}

// Resolve one operand to a runtime value. Throws PredicateError for
// anything the dispatcher cannot know (never guesses).
inline Value resolve_operand(const ExprValue& value, const ExecValues& ctx) {
    Value out;
    switch (value.kind) {
        case ExprValue::Kind::String:
            out.kind = Value::Kind::Str;
            out.str = value.str;
            return out;
        case ExprValue::Kind::Int:
            out.kind = Value::Kind::Int;
            out.integer = value.integer;
            return out;
        case ExprValue::Kind::Float:
            out.kind = Value::Kind::Float;
            out.floating = value.floating;
            return out;
        case ExprValue::Kind::Bool:
            out.kind = Value::Kind::Bool;
            out.boolean = value.boolean;
            return out;
        case ExprValue::Kind::Field: {
            const std::string name = join_field_path(value.field);
            const auto found = ctx.bindings.find(name);
            if (found == ctx.bindings.end()) {
                throw PredicateError(
                    "cannot evaluate reference '" + name +
                    "' (no call-produced value bound under that name at "
                    "this point; table contents are not materialized at "
                    "dispatch time)");
            }
            out.kind = Value::Kind::Str;
            out.str = found->second;
            return out;
        }
        case ExprValue::Kind::Call: {
            const std::string key =
                call_key(value.call->line, value.call->col);
            const auto found = ctx.call_outputs.find(key);
            if (found == ctx.call_outputs.end()) {
                throw PredicateError(
                    "cannot evaluate call result at " + key +
                    " (no recorded output for that call site)");
            }
            out.kind = Value::Kind::Str;
            out.str = found->second;
            return out;
        }
        case ExprValue::Kind::List: {
            out.kind = Value::Kind::List;
            for (const auto& item : value.list) {
                out.list.push_back(resolve_operand(*item, ctx));
            }
            return out;
        }
        case ExprValue::Kind::Source:
            throw PredicateError(
                "cannot evaluate evidence-source reference '" +
                value.source +
                "' in a runtime condition (sources stream rows; they are "
                "not scalar values)");
        case ExprValue::Kind::Correlate:
            throw PredicateError(
                "cannot evaluate correlate result in a runtime condition "
                "(join results are not materialized at dispatch time)");
    }
    throw PredicateError("cannot evaluate operand (unknown value kind)");
}

// Numeric view: int/float directly. Anything else is not numeric.
inline bool as_number(const Value& value, double& number) {
    if (value.kind == Value::Kind::Int) {
        number = static_cast<double>(value.integer);
        return true;
    }
    if (value.kind == Value::Kind::Float) {
        number = value.floating;
        return true;
    }
    return false;
}

inline bool numbers_equal(double left, double right) {
    if (left == right) return true;
    const double spread = std::fabs(left) + std::fabs(right);
    if (spread == 0.0) return true;
    const double scale = (spread > 1.0) ? spread : 1.0;
    return std::fabs(left - right) <=
           std::numeric_limits<double>::epsilon() * scale * 4.0;
}

inline bool values_equal(const Value& left, const Value& right) {
    if (left.kind == Value::Kind::List ||
        right.kind == Value::Kind::List) {
        if (left.kind != Value::Kind::List ||
            right.kind != Value::Kind::List) {
            throw PredicateError(
                "cannot compare a list against a non-list with ==/!=");
        }
        if (left.list.size() != right.list.size()) return false;
        for (std::size_t i = 0; i < left.list.size(); ++i) {
            if (!values_equal(left.list[i], right.list[i])) return false;
        }
        return true;
    }
    double left_num = 0.0;
    double right_num = 0.0;
    const bool left_is_num = as_number(left, left_num);
    const bool right_is_num = as_number(right, right_num);
    if (left_is_num && right_is_num) {
        if (left.kind == Value::Kind::Int &&
            right.kind == Value::Kind::Int) {
            return left.integer == right.integer;
        }
        return numbers_equal(left_num, right_num);
    }
    if (left_is_num != right_is_num) {
        throw PredicateError(
            "cannot compare a number against a non-number with ==/!= "
            "(no implicit string/number coercion at runtime)");
    }
    if (left.kind == Value::Kind::Str && right.kind == Value::Kind::Str) {
        return left.str == right.str;
    }
    if (left.kind == Value::Kind::Bool &&
        right.kind == Value::Kind::Bool) {
        return left.boolean == right.boolean;
    }
    throw PredicateError(
        "cannot compare these operand kinds with ==/!= "
        "(need number/number, string/string, bool/bool, or list/list)");
}

inline bool values_ordered(const std::string& op, const Value& left,
                           const Value& right) {
    double left_num = 0.0;
    double right_num = 0.0;
    if (as_number(left, left_num) && as_number(right, right_num)) {
        if (op == "<") return left_num < right_num;
        if (op == "<=") return left_num <= right_num;
        if (op == ">") return left_num > right_num;
        if (op == ">=") return left_num >= right_num;
        throw PredicateError("unknown ordering operator '" + op + "'");
    }
    if (left.kind == Value::Kind::Str &&
        right.kind == Value::Kind::Str) {
        if (op == "<") return left.str < right.str;
        if (op == "<=") return left.str <= right.str;
        if (op == ">") return left.str > right.str;
        if (op == ">=") return left.str >= right.str;
        throw PredicateError("unknown ordering operator '" + op + "'");
    }
    throw PredicateError(
        "cannot order these operand kinds with '" + op +
        "' (need number/number or string/string)");
}

inline bool list_contains(const Value& list, const Value& item) {
    for (const Value& element : list.list) {
        if (values_equal(element, item)) return true;
    }
    return false;
}

inline bool values_truthy(const Value& value) {
    switch (value.kind) {
        case Value::Kind::Bool: return value.boolean;
        case Value::Kind::Int: return value.integer != 0;
        case Value::Kind::Float: return value.floating != 0.0;
        case Value::Kind::Str: return !value.str.empty();
        case Value::Kind::List: return !value.list.empty();
    }
    return false;
}

inline bool evaluate_node(const Predicate& pred, const ExecValues& ctx);

inline bool evaluate_compare(const Predicate& pred, const ExecValues& ctx) {
    Value left = resolve_operand(*pred.left, ctx);
    Value right = resolve_operand(*pred.right, ctx);
    const std::string& op = pred.op;
    if (op == "==") return values_equal(left, right);
    if (op == "!=") return !values_equal(left, right);
    if (op == "<" || op == "<=" || op == ">" || op == ">=") {
        return values_ordered(op, left, right);
    }
    if (op == "in") {
        if (right.kind != Value::Kind::List) {
            throw PredicateError(
                "right side of 'in' must be a list at runtime");
        }
        return list_contains(right, left);
    }
    if (op == "contains") {
        if (left.kind == Value::Kind::Str &&
            right.kind == Value::Kind::Str) {
            return left.str.find(right.str) != std::string::npos;
        }
        if (left.kind == Value::Kind::List) {
            return list_contains(left, right);
        }
        throw PredicateError(
            "runtime 'contains' needs string-in-string or "
            "element-in-list");
    }
    if (op == "contains_any") {
        if (right.kind != Value::Kind::List) {
            throw PredicateError(
                "right side of 'contains_any' must be a list at runtime");
        }
        if (left.kind == Value::Kind::Str) {
            for (const Value& item : right.list) {
                if (item.kind != Value::Kind::Str) {
                    throw PredicateError(
                        "runtime 'contains_any' over a string needs a "
                        "list of strings");
                }
                if (left.str.find(item.str) != std::string::npos) {
                    return true;
                }
            }
            return false;
        }
        if (left.kind == Value::Kind::List) {
            for (const Value& item : right.list) {
                if (list_contains(left, item)) return true;
            }
            return false;
        }
        throw PredicateError(
            "runtime 'contains_any' needs a string or list on the left");
    }
    throw PredicateError("unknown comparison operator '" + op + "'");
}

inline bool evaluate_node(const Predicate& pred, const ExecValues& ctx) {
    switch (pred.kind) {
        case Predicate::Kind::Or:
            for (const auto& operand : pred.operands) {
                if (evaluate_node(*operand, ctx)) return true;
            }
            return false;
        case Predicate::Kind::And:
            for (const auto& operand : pred.operands) {
                if (!evaluate_node(*operand, ctx)) return false;
            }
            return true;
        case Predicate::Kind::Not:
            return !evaluate_node(*pred.inner, ctx);
        case Predicate::Kind::Compare:
            return evaluate_compare(pred, ctx);
        case Predicate::Kind::Atom:
            return values_truthy(resolve_operand(*pred.atom, ctx));
    }
    throw PredicateError("cannot evaluate predicate (unknown node kind)");
}

}  // namespace predicate_detail

// Evaluate a runtime condition to a concrete bool. All failures throw
// PredicateError (fail-closed); there is no default branch value.
inline bool evaluate_predicate(const Predicate& pred,
                               const ExecValues& ctx) {
    return predicate_detail::evaluate_node(pred, ctx);
}

}  // namespace jocky
