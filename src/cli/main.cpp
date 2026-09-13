// jocky CLI — Phase 1 skeleton + Phase 3 `resolve` + Phase 4 `gate`
// + Phase 5 `shake`.
// Usage: jocky check <file.jky>
//        jocky resolve <file.jky> [--registry <dir>]
//        jocky gate <file.jky> [--registry <dir>]
//        jocky shake <file.jky> [--registry <dir>]
// `check` lexes, parses, and pretty-prints the AST (output frozen since
// Phase 1 — do not change it; baselines in wiki/10, wiki/11, and wiki/14
// depend on it). `resolve` additionally runs the Phase 3 call resolver
// against the registry index and prints each resolved call with its
// inferred type. `gate` runs the full Phase 4 chain (resolve → bind →
// gate) and prints the authorization verdict. `shake` runs the full
// Phase 5 chain (resolve → bind → gate → shake) and prints the
// dependency-ordered used-script list. `check` is parse-only and never
// runs semantic passes; `resolve`, `gate`, and `shake` run resolution
// then the Phase 5.5 for-bound check before binding/gating/shaking.
// Exit 0 on success, exit 1
// on usage errors or lex/parse/resolve/bound/bind/gate/shake failures
// (diagnostics on stderr in file:line:col style).

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <variant>

#include "jocky/ast/ast.hpp"
#include "jocky/fir/script_resolver.hpp"
#include "jocky/lexer/lexer.hpp"
#include "jocky/semantic/bound_checker.hpp"
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

void print_pipeline_stmt(std::ostream& os, const jocky::PipelineStmt& stmt,
                         int depth) {
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

void print_stmt(std::ostream& os, const jocky::Stmt& stmt, int depth);

void print_block(std::ostream& os, const jocky::Block& block, int depth) {
    os << "{\n";
    for (const auto& stmt : block) {
        print_stmt(os, stmt, depth + 1);
    }
    indent(os, depth);
    os << "}";
}

void print_bound(std::ostream& os, const jocky::BoundExpr& bound) {
    switch (bound.kind) {
        case jocky::BoundExpr::Kind::Literal: os << bound.literal; break;
        case jocky::BoundExpr::Kind::Ident: os << bound.ident; break;
        case jocky::BoundExpr::Kind::Count:
            os << "count(" << bound.ident << ")";
            break;
    }
}

void print_stmt(std::ostream& os, const jocky::Stmt& stmt, int depth) {
    if (const auto* pipe =
            std::get_if<jocky::PipelineStmt>(&stmt.node)) {
        print_pipeline_stmt(os, *pipe, depth);
        return;
    }
    if (const auto* branch = std::get_if<jocky::IfStmt>(&stmt.node)) {
        indent(os, depth);
        os << "if (";
        print_predicate(os, branch->cond);
        os << ") ";
        print_block(os, branch->then_block, depth);
        if (branch->else_block.has_value()) {
            os << " else ";
            print_block(os, *branch->else_block, depth);
        }
        os << "\n";
        return;
    }
    if (const auto* loop = std::get_if<jocky::ForStmt>(&stmt.node)) {
        indent(os, depth);
        os << "for (int " << loop->loop_var << " = " << loop->start << "; "
           << loop->loop_var << " < ";
        print_bound(os, loop->bound);
        os << "; " << loop->loop_var << "++) ";
        print_block(os, loop->body, depth);
        os << "\n";
        return;
    }
    if (const auto* loop = std::get_if<jocky::WhileStmt>(&stmt.node)) {
        indent(os, depth);
        os << "while (";
        print_predicate(os, loop->cond);
        os << ") [requires_runtime_ceiling] ";
        print_block(os, loop->body, depth);
        os << "\n";
        return;
    }
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
        os << "]";
        // Phase 7.5: rendered only when the source wrote the field, so
        // every pre-7.5 program prints byte-identically to the frozen
        // wiki/11 §1 baseline.
        if (c.max_while_iterations.has_value()) {
            os << " max_while_iterations=" << *c.max_while_iterations;
        }
        os << "\n";
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
        jocky::ResolvedProgram resolved =
            jocky::resolve_program(prog, scanned.registry);
        // Bound check runs AFTER resolution (a bad bound may reference a
        // call result, so unknown-function errors keep precedence) and
        // BEFORE any binding/gating. `check` stays parse-only and never
        // runs this pass.
        jocky::check_bounds(prog);
        print_resolved(resolved);
    } catch (const jocky::SemanticError& ex) {
        std::cerr << "error: " << path << ":" << ex.line << ":" << ex.col
                  << ": " << ex.what() << "\n";
        return 1;
    } catch (const jocky::BoundCheckError& ex) {
        std::cerr << "error: " << path << ":" << ex.line << ":" << ex.col
                  << ": " << ex.what() << " (bound check)\n";
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
        jocky::check_bounds(prog);
        jocky::BoundProgram bound =
            jocky::bind_program(prog, std::move(resolved));
        jocky::GateResult gate = jocky::check_gate(bound);
        print_gate(path, gate);
        return gate.allowed ? 0 : 1;
    } catch (const jocky::SemanticError& ex) {
        std::cerr << "error: " << path << ":" << ex.line << ":" << ex.col
                  << ": " << ex.what() << "\n";
        return 1;
    } catch (const jocky::BoundCheckError& ex) {
        std::cerr << "error: " << path << ":" << ex.line << ":" << ex.col
                  << ": " << ex.what() << " (bound check)\n";
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

void print_shaken(const jocky::ShakeResult& shaken) {
    std::cout << "SHAKEN: " << shaken.scripts.size()
              << " scripts in dependency order\n";
    for (const jocky::ResolvedScript& script : shaken.scripts) {
        std::cout << "  " << script.metadata.function << " ("
                  << script.metadata.script_path << ") [" << script.reason
                  << "]\n";
    }
}

int run_shake(const std::string& path, const std::string& registry_dir) {
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
        std::cerr << "error: cannot shake against a rejected registry "
                     "index (fix headers or rebuild the registry)\n";
        return 1;
    }
    try {
        jocky::ResolvedProgram resolved =
            jocky::resolve_program(prog, scanned.registry);
        jocky::check_bounds(prog);
        jocky::BoundProgram bound =
            jocky::bind_program(prog, std::move(resolved));
        jocky::GateResult gate = jocky::check_gate(bound);
        if (!gate.allowed) {
            // Same verdict format as `jocky gate`: shake never runs on a
            // denied gate (fail-closed), and the denial is the error.
            print_gate(path, gate);
            return 1;
        }
        print_shaken(jocky::resolve_scripts(gate, scanned.registry));
    } catch (const jocky::SemanticError& ex) {
        std::cerr << "error: " << path << ":" << ex.line << ":" << ex.col
                  << ": " << ex.what() << "\n";
        return 1;
    } catch (const jocky::BoundCheckError& ex) {
        std::cerr << "error: " << path << ":" << ex.line << ":" << ex.col
                  << ": " << ex.what() << " (bound check)\n";
        return 1;
    } catch (const jocky::BindingError& ex) {
        if (ex.line > 0) {
            std::cerr << "error: " << path << ":" << ex.line << ":" << ex.col
                      << ": " << ex.what() << " (case binding)\n";
        } else {
            std::cerr << "error: " << path << ": " << ex.what()
                      << " (case binding)\n";
        }
        return 1;
    } catch (const jocky::ShakeError& ex) {
        // Shake failures are registry-level (line/col 0) — tagged
        // "(shake)" so the failing stage reads off the output.
        if (ex.line > 0) {
            std::cerr << "error: " << path << ":" << ex.line << ":" << ex.col
                      << ": " << ex.what() << " (shake)\n";
        } else {
            std::cerr << "error: " << path << ": " << ex.what()
                      << " (shake)\n";
        }
        return 1;
    }
    return 0;
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
    if ((argc == 3 || (argc == 5 && std::string(argv[3]) == "--registry")) &&
        std::string(argv[1]) == "shake") {
        const std::string registry =
            (argc == 5) ? argv[4] : "stat_scripts/";
        return run_shake(argv[2], registry);
    }
    std::cerr << "usage: jocky check <file.jky>\n"
                 "       jocky resolve <file.jky> [--registry <dir>]\n"
                 "       jocky gate <file.jky> [--registry <dir>]\n"
                 "       jocky shake <file.jky> [--registry <dir>]\n";
    return 1;
}
