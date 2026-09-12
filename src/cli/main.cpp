// jocky CLI — Phase 1 skeleton + Phase 3 `resolve` + Phase 4 `gate`.
// Usage: jocky check <file.jky>
//        jocky resolve <file.jky> [--registry <dir>]
//        jocky gate <file.jky> [--registry <dir>]
// `check` lexes, parses, and pretty-prints the AST (output frozen since
// Phase 1 — do not change it; baselines in wiki/10, wiki/11, and wiki/14
// depend on it). `resolve` additionally runs the Phase 3 call resolver
// against the registry index and prints each resolved call with its
// inferred type. `gate` runs the full Phase 4 chain (resolve → bind →
// gate) and prints the authorization verdict. Exit 0 on success, exit 1
// on usage errors or lex/parse/resolve/bind/gate failures (diagnostics on
// stderr in file:line:col style).

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "jocky/ast/ast.hpp"
#include "jocky/lexer/lexer.hpp"
#include "jocky/parser/parser.hpp"
#include "jocky/policy/capability_gate.hpp"
#include "jocky/semantic/call_resolver.hpp"
#include "jocky/semantic/case_binder.hpp"
#include "jocky/stdlib/script_metadata.hpp"

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

// Lex + parse; throws runtime_error (I/O), LexError, or ParseError.
jocky::Program load_program(const std::string& path) {
    const std::string source = read_file(path);
    jocky::Lexer lexer(source);
    std::vector<jocky::Token> tokens = lexer.tokenize();
    jocky::Parser parser(std::move(tokens));
    return parser.parse_program();
}

void print_resolved(const jocky::ResolvedProgram& resolved) {
    for (const jocky::ResolvedCall& call : resolved.calls) {
        std::cout << "RESOLVED call " << call.function << " -> "
                  << jocky::type_to_string(call.result_type)
                  << " [capability " << call.capability << "]\n";
        for (const jocky::ResolvedArg& arg : call.args) {
            std::cout << "  arg " << arg.name << ": " << arg.declared_type;
            if (arg.used_default) {
                std::cout << " = <default \"" << arg.default_value << "\">";
            }
            std::cout << "\n";
        }
        if (call.has_binding) {
            std::cout << "BOUND " << call.binding << ": "
                      << jocky::type_to_string(call.result_type) << "\n";
        }
    }
    if (resolved.calls.empty()) {
        std::cout << "RESOLVED 0 calls\n";
    }
}

int run_check(const std::string& path) {
    try {
        print_program(std::cout, load_program(path));
    } catch (const jocky::LexError& ex) {
        std::cerr << "error: " << path << ":" << ex.line << ":" << ex.col
                  << ": " << ex.what() << "\n";
        return 1;
    } catch (const jocky::ParseError& ex) {
        std::cerr << "error: " << path << ":" << ex.line << ":" << ex.col
                  << ": " << ex.what() << "\n";
        return 1;
    } catch (const std::exception& ex) {
        std::cerr << "error: " << ex.what() << "\n";
        return 1;
    }
    return 0;
}

int run_resolve(const std::string& path, const std::string& registry_dir) {
    jocky::Program prog;
    try {
        prog = load_program(path);
    } catch (const jocky::LexError& ex) {
        std::cerr << "error: " << path << ":" << ex.line << ":" << ex.col
                  << ": " << ex.what() << "\n";
        return 1;
    } catch (const jocky::ParseError& ex) {
        std::cerr << "error: " << path << ":" << ex.line << ":" << ex.col
                  << ": " << ex.what() << "\n";
        return 1;
    } catch (const std::exception& ex) {
        std::cerr << "error: " << ex.what() << "\n";
        return 1;
    }
    jocky::ScanResult scanned = jocky::scan_registry_dir(registry_dir);
    if (!scanned.rejections.empty()) {
        for (const std::string& rejection : scanned.rejections) {
            std::cerr << "REJECT " << rejection << "\n";
        }
        std::cerr << "error: cannot resolve against a rejected registry "
                     "index (fix headers or rebuild the registry)\n";
        return 1;
    }
    try {
        print_resolved(jocky::resolve_program(prog, scanned.registry));
    } catch (const jocky::SemanticError& ex) {
        std::cerr << "error: " << path << ":" << ex.line << ":" << ex.col
                  << ": " << ex.what() << "\n";
        return 1;
    }
    return 0;
}

void print_gate(const std::string& path, const jocky::GateResult& gate) {
    if (gate.allowed) {
        std::cout << "ALLOWED: " << gate.authorized.size()
                  << " calls authorized under case '" << gate.case_name
                  << "'\n";
        for (const jocky::ResolvedCall& call : gate.authorized) {
            std::cout << "  call " << call.function << " -> "
                      << jocky::type_to_string(call.result_type)
                      << " [capability " << call.capability << "]\n";
        }
        return;
    }
    std::cout << "DENIED: " << gate.violations.size() << " violation(s)"
              << " under case '" << gate.case_name << "'\n";
    for (const jocky::GateViolation& violation : gate.violations) {
        std::cout << "  " << path << ":" << violation.line << ":"
                  << violation.col << ": call '" << violation.function
                  << "' requires capability '" << violation.capability
                  << "' not granted by case '" << violation.case_name
                  << "'\n";
    }
}

int run_gate(const std::string& path, const std::string& registry_dir) {
    jocky::Program prog;
    try {
        prog = load_program(path);
    } catch (const jocky::LexError& ex) {
        std::cerr << "error: " << path << ":" << ex.line << ":" << ex.col
                  << ": " << ex.what() << "\n";
        return 1;
    } catch (const jocky::ParseError& ex) {
        std::cerr << "error: " << path << ":" << ex.line << ":" << ex.col
                  << ": " << ex.what() << "\n";
        return 1;
    } catch (const std::exception& ex) {
        std::cerr << "error: " << ex.what() << "\n";
        return 1;
    }
    jocky::ScanResult scanned = jocky::scan_registry_dir(registry_dir);
    if (!scanned.rejections.empty()) {
        for (const std::string& rejection : scanned.rejections) {
            std::cerr << "REJECT " << rejection << "\n";
        }
        std::cerr << "error: cannot gate against a rejected registry "
                     "index (fix headers or rebuild the registry)\n";
        return 1;
    }
    try {
        jocky::ResolvedProgram resolved =
            jocky::resolve_program(prog, scanned.registry);
        jocky::BoundProgram bound =
            jocky::bind_program(prog, std::move(resolved));
        jocky::GateResult gate = jocky::check_gate(bound);
        print_gate(path, gate);
        return gate.allowed ? 0 : 1;
    } catch (const jocky::SemanticError& ex) {
        std::cerr << "error: " << path << ":" << ex.line << ":" << ex.col
                  << ": " << ex.what() << "\n";
        return 1;
    } catch (const jocky::BindingError& ex) {
        // Binder failures are program-level (line/col 0: no single token
        // to point at) — tagged "(case binding)" so fixture 3 vs 4 runs
        // show which stage refused, binder here vs gate in DENIED lines.
        if (ex.line > 0) {
            std::cerr << "error: " << path << ":" << ex.line << ":" << ex.col
                      << ": " << ex.what() << " (case binding)\n";
        } else {
            std::cerr << "error: " << path << ": " << ex.what()
                      << " (case binding)\n";
        }
        return 1;
    }
}

int main(int argc, char** argv) {
    if (argc == 3 && std::string(argv[1]) == "check") {
        return run_check(argv[2]);
    }
    if ((argc == 3 || (argc == 5 && std::string(argv[3]) == "--registry")) &&
        std::string(argv[1]) == "resolve") {
        const std::string registry =
            (argc == 5) ? argv[4] : "stat_scripts/";
        return run_resolve(argv[2], registry);
    }
    if ((argc == 3 || (argc == 5 && std::string(argv[3]) == "--registry")) &&
        std::string(argv[1]) == "gate") {
        const std::string registry =
            (argc == 5) ? argv[4] : "stat_scripts/";
        return run_gate(argv[2], registry);
    }
    std::cerr << "usage: jocky check <file.jky>\n"
                 "       jocky resolve <file.jky> [--registry <dir>]\n"
                 "       jocky gate <file.jky> [--registry <dir>]\n";
    return 1;
}
