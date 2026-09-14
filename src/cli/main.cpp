// jocky CLI — Phase 1 skeleton + Phase 3 `resolve` + Phase 4 `gate`
// + Phase 5 `shake` + Phase 8 interpreted `run` (bare `jocky <file.jky>`).
// Usage: jocky check <file.jky>
//        jocky resolve <file.jky> [--registry <dir>]
//        jocky gate <file.jky> [--registry <dir>]
//        jocky shake <file.jky> [--registry <dir>]
//        jocky <file.jky> [--registry <dir>] [--output-root <dir>]
//             [--manifest <path>] [--max-executions <n>]
//             [--max-iterations <n>]
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

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <variant>

#include "jocky/ast/ast.hpp"
#include "jocky/crypto/sha256.hpp"
#include "jocky/fir/script_resolver.hpp"
#include "jocky/lexer/lexer.hpp"
#include "jocky/runtime/control_flow_executor.hpp"
#include "jocky/runtime/verifier.hpp"
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

// `jocky verify <manifest.json>`: re-hash recorded artifacts and
// re-check per-execution integrity entries. Reads only the manifest
// and the artifacts it names; never executes scripts.
int run_verify(const std::string& path) {
    jocky::VerifyResult result = jocky::verify_manifest(path);
    if (result.pass) {
        std::cout << "VERIFY PASS " << path << "\n";
        return 0;
    }
    std::cout << "VERIFY FAIL " << path << " (" << result.issues.size()
              << " issue(s))\n";
    for (const jocky::VerifyIssue& issue : result.issues) {
        std::cout << "  " << issue.where << ": " << issue.message << "\n";
    }
    return 1;
}

// `jocky console <case-dir>`: offline multi-case console. A minimal
// read-eval loop over the .jky files in a directory — list cases,
// run check/resolve/gate/shake/execute/verify without leaving the
// session. No sockets, no listeners, no network: every command reuses
// the in-process run_* functions above against local files only.
int run_execute(const std::string& path,
                const std::vector<std::string>& raw_args);
int run_console(const std::string& case_dir,
                const std::string& registry_dir) {
    namespace fs = std::filesystem;
    std::error_code ec;
    if (!fs::is_directory(case_dir, ec) || ec) {
        std::cerr << "error: case directory not found: '" << case_dir
                  << "'\n";
        return 1;
    }
    std::cout << "JOCKY offline console — case dir '" << case_dir
              << "', registry '" << registry_dir << "'\n"
                 "commands: cases | check <f> | resolve <f> | gate <f> | "
                 "shake <f> | run <f> | verify <manifest> | quit\n";
    auto resolve_local = [&](const std::string& name) {
        fs::path path(name);
        if (path.is_absolute() || name.find('/') != std::string::npos) {
            return path.string();
        }
        return (fs::path(case_dir) / name).string();
    };
    std::string line;
    while (true) {
        std::cout << "jocky> " << std::flush;
        if (!std::getline(std::cin, line)) {
            std::cout << "\n";
            return 0;
        }
        std::istringstream words(line);
        std::string cmd;
        words >> cmd;
        if (cmd.empty()) continue;
        if (cmd == "quit" || cmd == "exit") return 0;
        if (cmd == "cases") {
            for (const auto& entry : fs::directory_iterator(case_dir, ec)) {
                if (ec) break;
                if (entry.path().extension() == ".jky") {
                    std::cout << "  " << entry.path().filename().string()
                              << "\n";
                }
            }
            if (ec) {
                std::cerr << "error: cannot list '" << case_dir
                          << "': " << ec.message() << "\n";
            }
            continue;
        }
        std::string arg;
        words >> arg;
        if ((cmd == "check" || cmd == "resolve" || cmd == "gate" ||
             cmd == "shake" || cmd == "run" || cmd == "verify") &&
            arg.empty()) {
            std::cerr << "usage: " << cmd << " <file>\n";
            continue;
        }
        if (cmd == "check") {
            run_check(resolve_local(arg));
        } else if (cmd == "resolve") {
            run_resolve(resolve_local(arg), registry_dir);
        } else if (cmd == "gate") {
            run_gate(resolve_local(arg), registry_dir);
        } else if (cmd == "shake") {
            run_shake(resolve_local(arg), registry_dir);
        } else if (cmd == "run") {
            run_execute(resolve_local(arg), {});
        } else if (cmd == "verify") {
            run_verify(resolve_local(arg));
        } else {
            std::cerr << "unknown command '" << cmd
                      << "' (try: cases, check, resolve, gate, shake, run, "
                         "verify, quit)\n";
        }
    }
}

// Phase 8 interpreter: run a .jky program directly against the
// filesystem registry — no compile step, no embedding, no shake step
// (shaking decides what to EMBED; with nothing embedded it is
// meaningless, and skipping it is deliberate — the gate already limits
// execution to authorized calls, and every attempt is still
// capability-rechecked at dispatch).
//
// The ONLY structural difference from a compiled binary is call
// sourcing: each authorized call is materialized through
// load_registry_runtime_call (bytes read from stat_scripts/ on disk)
// instead of an embedded byte buffer. Predicate evaluation, if/for/
// while semantics, ceilings, sandboxing, and the manifest writer are
// the shared, unmodified Phase 7.5 code path (execute_plan_tree over
// plan.program_source), so the two modes cannot drift.
const jocky::ScriptMetadata* find_registry_function(
    const std::vector<jocky::ScriptMetadata>& registry,
    const std::string& function) {
    for (const jocky::ScriptMetadata& meta : registry) {
        if (meta.function == function) {
            return &meta;
        }
    }
    return nullptr;
}

// Declared `write` targets in rules then investigations (walker order)
// — the same collection the compiler emits into build_plan, so both
// paths stage and promote the same outputs.
std::vector<std::string> collect_run_outputs(const jocky::Program& prog) {
    std::vector<std::string> outputs;
    auto collect = [&](const jocky::PipelineStmt& stmt) {
        for (const jocky::PipelineStep& step : stmt.expr.steps) {
            if (step.op.kind == jocky::PipelineOp::Kind::Write) {
                outputs.push_back(step.op.write_path);
            }
        }
    };
    for (const jocky::RuleDecl& rule : prog.rules) {
        jocky::walker::for_each_pipeline_stmt_in_block(rule.body, collect);
    }
    for (const jocky::InvestigationDecl& inv : prog.investigations) {
        jocky::walker::for_each_pipeline_stmt_in_block(inv.body, collect);
    }
    return outputs;
}

int run_execute(const std::string& path,
                const std::vector<std::string>& raw_args) {
    // Flag defaults mirror the compiled standalone's `run` branch
    // exactly (same names, same shared RuntimeOptions defaults).
    std::string registry_dir = "stat_scripts/";
    std::string output_root = "out";
    std::string manifest_path;
    std::optional<std::size_t> max_executions;
    std::optional<std::size_t> max_iterations;
    try {
        for (std::size_t i = 0; i < raw_args.size(); ++i) {
            const std::string& arg = raw_args[i];
            auto need_value = [&](const char* flag) -> std::string {
                if (i + 1 >= raw_args.size()) {
                    throw std::runtime_error(
                        std::string("unknown or incomplete run option '") +
                        arg + "' (flag '" + flag + "' needs a value)");
                }
                return raw_args[++i];
            };
            if (arg == "--registry") {
                registry_dir = need_value("--registry");
            } else if (arg == "--output-root") {
                output_root = need_value("--output-root");
            } else if (arg == "--manifest") {
                manifest_path = need_value("--manifest");
            } else if (arg == "--max-executions") {
                max_executions = static_cast<std::size_t>(
                    std::stoull(need_value("--max-executions")));
            } else if (arg == "--max-iterations") {
                max_iterations = static_cast<std::size_t>(
                    std::stoull(need_value("--max-iterations")));
            } else {
                throw std::runtime_error(
                    std::string("unknown or incomplete run option '") + arg +
                    "'");
            }
        }
    } catch (const std::exception& ex) {
        std::cerr << "error: " << ex.what() << "\n";
        return 1;
    }
    if (manifest_path.empty()) {
        manifest_path =
            (std::filesystem::path(output_root) / "manifest.json").string();
    }

    jocky::Program prog;
    std::string source;
    try {
        source = read_file(path);
        jocky::Lexer lexer(source);
        std::vector<jocky::Token> tokens = lexer.tokenize();
        jocky::Parser parser(std::move(tokens));
        prog = parser.parse_program();
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
        std::cerr << "error: cannot execute against a rejected registry "
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
            // Same verdict format as `jocky gate`: execution never runs
            // on a denied gate (fail-closed), and the denial is the error.
            print_gate(path, gate);
            return 1;
        }
        // Build the runtime plan exactly the way the compiler's
        // build_plan does — same case, capabilities, evidence, outputs,
        // ceiling, source text, and authorized calls in the same order.
        // Only the script bytes differ: read from disk here (with the
        // scanner digest preserved, so drift becomes a manifest-logged
        // integrity_denied at dispatch) instead of embedded buffers.
        jocky::RuntimePlan plan;
        plan.case_id = gate.case_name;
        plan.program_sha256 = jocky::sha256_bytes(source);
        plan.program_source = source;
        if (bound.bound_case.max_while_iterations.has_value()) {
            plan.max_while_iterations = static_cast<std::size_t>(
                *bound.bound_case.max_while_iterations);
        }
        for (const std::string& capability : bound.bound_case.capabilities) {
            plan.allowed_capabilities.push_back(capability);
        }
        for (const jocky::EvidenceDecl& evidence : prog.evidence) {
            plan.evidence.push_back({evidence.name, evidence.adapter,
                                     evidence.path});
        }
        for (const std::string& output : collect_run_outputs(prog)) {
            plan.declared_outputs.push_back(output);
        }
        for (const jocky::ResolvedCall& resolved_call : gate.authorized) {
            const jocky::ScriptMetadata* meta =
                find_registry_function(scanned.registry,
                                       resolved_call.function);
            if (meta == nullptr) {
                // Resolve passed on this same scan, so absence means the
                // registry changed mid-run: refuse, never guess.
                std::cerr << "error: " << path << ": cannot execute "
                          << "authorized call '" << resolved_call.function
                          << "' (no registry entry; registry changed after "
                             "scan?)\n";
                return 1;
            }
            std::vector<jocky::RuntimeArgument> args;
            for (const jocky::ResolvedArg& resolved_arg :
                 resolved_call.args) {
                args.push_back({resolved_arg.name,
                                resolved_arg.declared_type,
                                resolved_arg.value, resolved_arg.concrete});
            }
            jocky::RuntimeCall call =
                jocky::load_registry_runtime_call(*meta, std::move(args));
            call.line = resolved_call.line;
            call.col = resolved_call.col;
            plan.calls.push_back(std::move(call));
        }
        jocky::RuntimeOptions options;
        options.executable_path = jocky::runtime_detail::self_executable();
        options.working_directory =
            std::filesystem::current_path().string();
        options.output_root = output_root;
        options.manifest_path = manifest_path;
        if (max_executions.has_value()) {
            options.max_executions = *max_executions;
        }
        if (max_iterations.has_value()) {
            options.max_while_iterations = *max_iterations;
        }
        const jocky::DispatchResult result =
            jocky::run_runtime_plan(plan, options);
        std::cout << "MANIFEST " << options.manifest_path
                  << " status=" << result.manifest.status << "\n";
        // Human summary, grounded only in manifest facts: attempts
        // dispatched (each loop trip dispatches independently, so trips
        // are included in the count), per-function breakdown, capped
        // loops called out, final status.
        std::map<std::string, std::size_t> per_function;
        std::size_t capped_loops = 0;
        for (const jocky::ExecutionRecord& entry :
             result.manifest.executions) {
            if (entry.outcome == "while_ceiling") {
                ++capped_loops;
            } else {
                ++per_function[entry.function];
            }
        }
        std::cout << "EXECUTED " << result.manifest.executions.size()
                  << " calls";
        bool first = true;
        for (const auto& [function, count] : per_function) {
            std::cout << (first ? " (" : ", ") << function << " x" << count;
            first = false;
        }
        if (!per_function.empty()) {
            std::cout << ")";
        }
        if (capped_loops > 0) {
            std::cout << " [" << capped_loops << " loop(s) capped at "
                      << "ceiling; see while_ceiling entries]";
        }
        std::cout << ", status=" << result.manifest.status << "\n";
        return result.exit_code;
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
    }
}

int main(int argc, char** argv) {
    // Sandbox children re-exec this binary (see run_execute: options
    // point at self_executable()). Handle before subcommand dispatch —
    // this path never parses .jky, never touches the registry.
    if (argc > 1 && std::string(argv[1]) == "--jocky-sandbox-child") {
        return jocky::runtime_detail::sandbox_child_main(argc, argv);
    }
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
    if (argc == 3 && std::string(argv[1]) == "verify") {
        return run_verify(argv[2]);
    }
    if ((argc == 3 || (argc == 5 && std::string(argv[3]) == "--registry")) &&
        std::string(argv[1]) == "console") {
        const std::string registry =
            (argc == 5) ? argv[4] : "stat_scripts/";
        return run_console(argv[2], registry);
    }
    // Bare `jocky <file.jky>` (Phase 8 interpreter). Any first argument
    // that is not a known subcommand is a program path; remaining
    // arguments are run flags parsed by run_execute. (A file literally
    // named check/resolve/gate/shake keeps its old meaning — passing it
    // bare still prints usage, as before.)
    if (argc >= 2 && std::string(argv[1]) != "check" &&
        std::string(argv[1]) != "resolve" &&
        std::string(argv[1]) != "gate" &&
        std::string(argv[1]) != "shake" &&
        std::string(argv[1]) != "verify" &&
        std::string(argv[1]) != "console" &&
        std::string(argv[1]).rfind("-", 0) != 0) {
        std::vector<std::string> run_args;
        for (int i = 2; i < argc; ++i) {
            run_args.push_back(argv[i]);
        }
        return run_execute(argv[1], run_args);
    }
    std::cerr << "usage: jocky check <file.jky>\n"
                 "       jocky resolve <file.jky> [--registry <dir>]\n"
                 "       jocky gate <file.jky> [--registry <dir>]\n"
                 "       jocky shake <file.jky> [--registry <dir>]\n"
                 "       jocky verify <manifest.json>\n"
                 "       jocky console <case-dir> [--registry <dir>]\n"
                 "       jocky <file.jky> [--registry <dir>]\n"
                 "             [--output-root <dir>] [--manifest <path>]\n"
                 "             [--max-executions <n>] [--max-iterations <n>]\n";
    return 1;
}
