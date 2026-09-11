// jocky CLI — Phase 1 skeleton.
// Usage: jocky check <file.jky>
// Lexes, parses, and pretty-prints the AST. Exit 0 on success,
// exit 1 on usage errors or lex/parse failures (diagnostics on stderr).

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "jocky/ast/ast.hpp"
#include "jocky/lexer/lexer.hpp"
#include "jocky/parser/parser.hpp"

namespace {

std::string read_file(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("cannot open file '" + path + "'");
    }
    std::ostringstream out;
    out << in.rdbuf();
    return out.str();
}

void indent(std::ostream& os, int depth) {
    for (int i = 0; i < depth; ++i) {
        os << "  ";
    }
}

std::string join_field(const jocky::FieldRef& field) {
    std::string text;
    for (std::size_t i = 0; i < field.path.size(); ++i) {
        if (i != 0) {
            text += ".";
        }
        text += field.path[i];
    }
    return text;
}

void print_correlate(std::ostream& os, const jocky::CorrelateSpec& spec) {
    os << "correlate(" << spec.left << ", " << spec.right << ") within "
       << spec.within_value << spec.within_unit << " on ";
    for (std::size_t i = 0; i < spec.on_fields.size(); ++i) {
        if (i != 0) {
            os << ", ";
        }
        os << join_field(spec.on_fields[i]);
    }
}

void print_value(std::ostream& os, const jocky::ExprValue& value);

void print_call(std::ostream& os, const jocky::CallExpr& call) {
    for (std::size_t i = 0; i < call.function.size(); ++i) {
        if (i != 0) {
            os << ".";
        }
        os << call.function[i];
    }
    os << "(";
    for (std::size_t i = 0; i < call.args.size(); ++i) {
        if (i != 0) {
            os << ", ";
        }
        os << call.args[i].name << ": ";
        print_value(os, *call.args[i].value);
    }
    os << ")";
}

void print_value(std::ostream& os, const jocky::ExprValue& value) {
    using Kind = jocky::ExprValue::Kind;
    switch (value.kind) {
        case Kind::String: os << "\"" << value.str << "\""; break;
        case Kind::Int: os << value.integer; break;
        case Kind::Float: os << value.floating; break;
        case Kind::Bool: os << (value.boolean ? "true" : "false"); break;
        case Kind::Field: os << join_field(value.field); break;
        case Kind::Call:
            os << "call ";
            print_call(os, *value.call);
            break;
        case Kind::List:
            os << "[";
            for (std::size_t i = 0; i < value.list.size(); ++i) {
                if (i != 0) {
                    os << ", ";
                }
                print_value(os, *value.list[i]);
            }
            os << "]";
            break;
        case Kind::Source: os << "source " << value.source; break;
        case Kind::Correlate: print_correlate(os, value.correlate); break;
    }
}

void print_predicate(std::ostream& os, const jocky::Predicate& pred) {
    using Kind = jocky::Predicate::Kind;
    switch (pred.kind) {
        case Kind::Or:
            os << "(";
            for (std::size_t i = 0; i < pred.operands.size(); ++i) {
                if (i != 0) {
                    os << " or ";
                }
                print_predicate(os, *pred.operands[i]);
            }
            os << ")";
            break;
        case Kind::And:
            os << "(";
            for (std::size_t i = 0; i < pred.operands.size(); ++i) {
                if (i != 0) {
                    os << " and ";
                }
                print_predicate(os, *pred.operands[i]);
            }
            os << ")";
            break;
        case Kind::Not:
            os << "not ";
            print_predicate(os, *pred.inner);
            break;
        case Kind::Compare:
            print_value(os, *pred.left);
            os << " " << pred.op << " ";
            print_value(os, *pred.right);
            break;
        case Kind::Atom: print_value(os, *pred.atom); break;
    }
}

void print_op(std::ostream& os, const jocky::PipelineOp& op) {
    using Kind = jocky::PipelineOp::Kind;
    switch (op.kind) {
        case Kind::Filter:
            os << "filter ";
            print_predicate(os, *op.predicate);
            break;
        case Kind::Where:
            os << "where ";
            print_predicate(os, *op.predicate);
            break;
        case Kind::Having:
            os << "having ";
            print_predicate(os, *op.predicate);
            break;
        case Kind::Select:
            os << "select ";
            for (std::size_t i = 0; i < op.select_items.size(); ++i) {
                if (i != 0) {
                    os << ", ";
                }
                os << join_field(op.select_items[i].field);
                if (op.select_items[i].has_alias) {
                    os << " as " << op.select_items[i].alias;
                }
            }
            break;
        case Kind::Correlate: print_correlate(os, op.correlate); break;
        case Kind::Write:
            if (op.write_is_report) {
                os << "write report(\"" << op.write_path << "\")";
            } else {
                os << "write \"" << op.write_path << "\"";
            }
            break;
        case Kind::GroupBy:
            os << "group_by ";
            for (std::size_t i = 0; i < op.group_fields.size(); ++i) {
                if (i != 0) {
                    os << ", ";
                }
                os << join_field(op.group_fields[i]);
            }
            break;
        case Kind::SortBy:
            os << "sort_by ";
            for (std::size_t i = 0; i < op.sort_fields.size(); ++i) {
                if (i != 0) {
                    os << ", ";
                }
                os << join_field(op.sort_fields[i]);
            }
            if (!op.sort_dir.empty()) {
                os << " " << op.sort_dir;
            }
            break;
        case Kind::Limit: os << "limit " << op.limit; break;
        case Kind::Emit:
            os << "emit ";
            if (op.emit_is_string) {
                os << "\"" << op.emit_target << "\"";
            } else {
                os << op.emit_target;
            }
            break;
    }
}

void print_type(std::ostream& os, const jocky::TypeRef& type) {
    os << type.name;
    if (!type.args.empty()) {
        os << "<";
        for (std::size_t i = 0; i < type.args.size(); ++i) {
            if (i != 0) {
                os << ", ";
            }
            print_type(os, type.args[i]);
        }
        os << ">";
    }
}

void print_stmt(std::ostream& os, const jocky::PipelineStmt& stmt, int depth) {
    indent(os, depth);
    if (stmt.has_binding) {
        os << "let " << stmt.binding << " = ";
    }
    print_value(os, *stmt.expr.head);
    for (const auto& step : stmt.expr.steps) {
        os << " | ";
        print_op(os, step.op);
    }
    os << ";\n";
}

void print_program(std::ostream& os, const jocky::Program& prog) {
    os << "Program\n";
    for (const auto& c : prog.cases) {
        os << "  CaseDecl " << c.name << " capabilities=[";
        for (std::size_t i = 0; i < c.capabilities.size(); ++i) {
            if (i != 0) {
                os << ", ";
            }
            os << "\"" << c.capabilities[i] << "\"";
        }
        os << "]\n";
    }
    for (const auto& e : prog.evidence) {
        os << "  EvidenceDecl " << e.name << " adapter=" << e.adapter
           << " path=\"" << e.path << "\"\n";
    }
    for (const auto& r : prog.rules) {
        os << "  RuleDecl " << r.name << "(";
        for (std::size_t i = 0; i < r.params.size(); ++i) {
            if (i != 0) {
                os << ", ";
            }
            os << r.params[i].name << ": ";
            print_type(os, r.params[i].type);
        }
        os << ") -> ";
        print_type(os, r.returns);
        os << "\n";
        for (const auto& stmt : r.body) {
            print_stmt(os, stmt, 2);
        }
    }
    for (const auto& inv : prog.investigations) {
        os << "  InvestigationDecl " << inv.name << "\n";
        for (const auto& stmt : inv.body) {
            print_stmt(os, stmt, 2);
        }
    }
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 3 || std::string(argv[1]) != "check") {
        std::cerr << "usage: jocky check <file.jky>\n";
        return 1;
    }
    const std::string path = argv[2];
    std::string source;
    try {
        source = read_file(path);
    } catch (const std::exception& ex) {
        std::cerr << "error: " << ex.what() << "\n";
        return 1;
    }
    std::vector<jocky::Token> tokens;
    try {
        jocky::Lexer lexer(source);
        tokens = lexer.tokenize();
    } catch (const jocky::LexError& ex) {
        std::cerr << "error: " << path << ":" << ex.line << ":" << ex.col
                  << ": " << ex.what() << "\n";
        return 1;
    }
    try {
        jocky::Parser parser(std::move(tokens));
        jocky::Program prog = parser.parse_program();
        print_program(std::cout, prog);
    } catch (const jocky::ParseError& ex) {
        std::cerr << "error: " << path << ":" << ex.line << ":" << ex.col
                  << ": " << ex.what() << "\n";
        return 1;
    }
    return 0;
}
