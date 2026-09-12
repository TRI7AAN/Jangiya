#pragma once
// JOCKY call resolver — Phase 3 semantic unit over the AST + registry index.
//
// Every `call f(...)` in a parsed program is checked against the
// ScriptMetadata produced by the Phase 2 scanner (`scan_registry_dir` /
// `registry_to_json`): the resolver consumes that index and never
// re-parses `@jocky:` headers (duplicated parsing is a Phase 3 DoD
// violation). All failures throw SemanticError carrying 1-based
// line/col in the lexer's `file:line:col` style; the CLI prints them as
// `error: <file>:<line>:<col>: <message>` and exits non-zero.
//
// Checked, in order, per call:
//   1. Function lookup — unknown names are hard errors, never silent.
//   2. Arity — unknown argument names rejected; missing required inputs
//      rejected UNLESS the schema declares `= default` (Phase 3 decision:
//      defaulted inputs are filled from InputParam::default_value).
//   3. Parse-level types — literal arg kinds checked against declared
//      input types (String accepts `string`|`path` since paths travel as
//      string literals; Int->int; Float->float; Bool->bool). Non-literal
//      references (field/source/correlate/nested-call) are dynamically
//      typed and accepted here; runtime re-validation lands in Phase 7
//      per AGENTS.md §2.5.
//   4. Return-type plumbing — the declared output type becomes a TypeRef
//      attached to the `let` binding, so later pipeline statements can be
//      type-checked without re-deriving it. `table<X>` stays as-is; a
//      bare format word (`text`/`json`/`csv`) normalizes to `table<W>`;
//      anything else is an "unknown output type" error.
//
// Deliberately NOT here (later phases): capability gating (Phase 4),
// tree-shaking over depends_on (Phase 5).

#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include "jocky/ast/ast.hpp"
#include "jocky/ast/walker.hpp"
#include "jocky/stdlib/script_metadata.hpp"

namespace jocky {

// Thrown on any resolution failure. Mirrors LexError/ParseError style.
class SemanticError : public std::runtime_error {
public:
    int line;
    int col;
    SemanticError(int line, int col, const std::string& message)
        : std::runtime_error(message), line(line), col(col) {}
};

inline std::string join_function_name(
    const std::vector<std::string>& parts) {
    std::string name;
    for (std::size_t i = 0; i < parts.size(); ++i) {
        if (i != 0) {
            name += ".";
        }
        name += parts[i];
    }
    return name;
}

// Human-readable kind of an argument value for mismatch messages.
inline std::string expr_kind_name(const ExprValue& value) {
    switch (value.kind) {
        case ExprValue::Kind::String: return "string";
        case ExprValue::Kind::Int: return "int";
        case ExprValue::Kind::Float: return "float";
        case ExprValue::Kind::Bool: return "bool";
        case ExprValue::Kind::List: return "list";
        case ExprValue::Kind::Field: return "field reference";
        case ExprValue::Kind::Source: return "source reference";
        case ExprValue::Kind::Correlate: return "correlate result";
        case ExprValue::Kind::Call: return "call result";
    }
    return "?";
}

// True when a literal argument value satisfies a declared input type.
// Non-literal references are dynamically typed: always accepted here,
// re-validated at runtime (Phase 7).
inline bool arg_value_matches(const std::string& declared,
                              const ExprValue& value) {
    switch (value.kind) {
        case ExprValue::Kind::String:
            return declared == "string" || declared == "path";
        case ExprValue::Kind::Int: return declared == "int";
        case ExprValue::Kind::Float: return declared == "float";
        case ExprValue::Kind::Bool: return declared == "bool";
        case ExprValue::Kind::List: return false;
        case ExprValue::Kind::Field:
        case ExprValue::Kind::Source:
        case ExprValue::Kind::Correlate:
        case ExprValue::Kind::Call: return true;
    }
    return false;
}

inline std::string type_to_string(const TypeRef& type) {
    std::string out = type.name;
    if (!type.args.empty()) {
        out += "<";
        for (std::size_t i = 0; i < type.args.size(); ++i) {
            if (i != 0) {
                out += ", ";
            }
            out += type_to_string(type.args[i]);
        }
        out += ">";
    }
    return out;
}

namespace detail {

inline bool is_type_char(char c, bool first) {
    const unsigned char u = static_cast<unsigned char>(c);
    if ((u >= 'A' && u <= 'Z') || (u >= 'a' && u <= 'z') || c == '_') {
        return true;
    }
    return !first && (u >= '0' && u <= '9');
}

// Parses `name` or `name<arg, ...>`; throws SemanticError on anything
// else so unknown output spellings fail loudly at the call site.
inline TypeRef parse_type_text(const std::string& text, std::size_t& pos,
                               int line, int col,
                               const std::string& function) {
    TypeRef type;
    std::size_t start = pos;
    while (pos < text.size() &&
           is_type_char(text[pos], pos == start)) {
        ++pos;
    }
    if (pos == start) {
        throw SemanticError(line, col,
                            "unknown output type '" + text +
                                "' for function '" + function + "'");
    }
    type.name = text.substr(start, pos - start);
    if (pos < text.size() && text[pos] == '<') {
        ++pos;
        type.args.push_back(
            parse_type_text(text, pos, line, col, function));
        while (pos < text.size() && text[pos] == ',') {
            ++pos;
            type.args.push_back(
                parse_type_text(text, pos, line, col, function));
        }
        if (pos >= text.size() || text[pos] != '>') {
            throw SemanticError(line, col,
                                "unknown output type '" + text +
                                    "' for function '" + function + "'");
        }
        ++pos;
    }
    return type;
}

}  // namespace detail

// Normalize a registry output_type to the call's result TypeRef:
// `table<flow>` parses as-is; a bare word (`text`, `json`, `csv`)
// becomes `table<<word>>`, the table type pipeline sources already
// produce; anything else is an unknown-output-type error.
inline TypeRef normalize_output_type(const std::string& output_type, int line,
                                     int col, const std::string& function) {
    std::size_t pos = 0;
    TypeRef type =
        detail::parse_type_text(output_type, pos, line, col, function);
    if (pos != output_type.size()) {
        throw SemanticError(line, col,
                            "unknown output type '" + output_type +
                                "' for function '" + function + "'");
    }
    if (type.args.empty()) {
        TypeRef table;
        table.name = "table";
        table.args.push_back(type);
        return table;
    }
    return type;
}

struct ResolvedArg {
    std::string name;
    std::string declared_type;
    bool used_default = false;
    std::string default_value;  // meaningful only when used_default
};

struct ResolvedCall {
    std::string function;  // dotted registry name
    std::string capability;  // declared capability (gated in Phase 4)
    std::string output_type;  // declared output_type, verbatim
    TypeRef result_type;  // normalized result type for binding plumbing
    std::vector<ResolvedArg> args;  // schema order; defaults filled in
    bool has_binding = false;
    std::string binding;
    int line = 0;
    int col = 0;
};

// Resolve one CallExpr against the registry index. Never returns a
// partial result: any failure throws SemanticError.
inline ResolvedCall resolve_call(
    const CallExpr& call, const std::vector<ScriptMetadata>& registry) {
    const std::string name = join_function_name(call.function);
    const ScriptMetadata* target = nullptr;
    for (const ScriptMetadata& meta : registry) {
        if (meta.function == name) {
            target = &meta;
            break;
        }
    }
    if (target == nullptr) {
        throw SemanticError(call.line, call.col,
                            "unknown function '" + name +
                                "' (no registry entry; run scan_registry " +
                                "to rebuild the index)");
    }
    std::string declared_names;
    for (std::size_t i = 0; i < target->inputs.size(); ++i) {
        if (i != 0) {
            declared_names += ", ";
        }
        declared_names += target->inputs[i].name;
    }
    ResolvedCall resolved;
    resolved.function = name;
    resolved.capability = target->capability;
    resolved.output_type = target->output_type;
    resolved.line = call.line;
    resolved.col = call.col;
    for (const InputParam& param : target->inputs) {
        const CallExpr::NamedArg* provided = nullptr;
        for (const CallExpr::NamedArg& arg : call.args) {
            if (arg.name == param.name) {
                provided = &arg;
                break;
            }
        }
        ResolvedArg out;
        out.name = param.name;
        out.declared_type = param.type;
        if (provided == nullptr) {
            if (!param.has_default) {
                throw SemanticError(
                    call.line, call.col,
                    "missing required argument '" + param.name +
                        "' for function '" + name +
                        "' (no default declared)");
            }
            out.used_default = true;
            out.default_value = param.default_value;
            resolved.args.push_back(std::move(out));
            continue;
        }
        if (!arg_value_matches(param.type, *provided->value)) {
            throw SemanticError(
                provided->line, provided->col,
                "type mismatch for argument '" + param.name +
                    "' of function '" + name + "': declared '" + param.type +
                    "' but got " + expr_kind_name(*provided->value));
        }
        resolved.args.push_back(std::move(out));
    }
    for (const CallExpr::NamedArg& arg : call.args) {
        bool known = false;
        for (const InputParam& param : target->inputs) {
            if (param.name == arg.name) {
                known = true;
                break;
            }
        }
        if (!known) {
            throw SemanticError(arg.line, arg.col,
                                "unknown argument '" + arg.name +
                                    "' for function '" + name +
                                    "' (declared inputs: " + declared_names +
                                    ")");
        }
    }
    resolved.result_type =
        normalize_output_type(target->output_type, call.line, call.col, name);
    return resolved;
}

struct ResolvedProgram {
    std::vector<ResolvedCall> calls;  // source order across rules+probes
    // Every `let x = call ...` binding with its inferred result type, so
    // later pipeline statements (`x | where ...`) type-check against it
    // without re-deriving the type. Last binding wins on shadowing.
    std::map<std::string, TypeRef> binding_types;
};

namespace detail {

// Single traversal lives in ast/walker.hpp (the ONLY place that knows
// how to find every CallExpr — pipeline heads, predicate operands,
// lists, and If/For/While bodies at any depth). These wrappers resolve
// each found call and preserve the return-type plumbing: the FIRST call
// collected from a pipeline statement is its head call, so binding
// attachment keeps its exact Phase 3 semantics.

inline void resolve_stmt(const PipelineStmt& stmt, const std::string& scope,
                         const std::vector<ScriptMetadata>& registry,
                         ResolvedProgram& out) {
    (void)scope;
    const std::size_t before = out.calls.size();
    walker::for_each_call_in_pipeline(
        stmt.expr, [&](const CallExpr& found) {
            out.calls.push_back(resolve_call(found, registry));
        });
    // Attach the head call's result type to its `let` binding: this is
    // the return-type plumbing future phases check pipelines against.
    if (stmt.has_binding && out.calls.size() > before &&
        stmt.expr.head->kind == ExprValue::Kind::Call) {
        ResolvedCall& head = out.calls[before];
        head.has_binding = true;
        head.binding = stmt.binding;
        out.binding_types[stmt.binding] = head.result_type;
    }
}

inline void resolve_block(const Block& block, const std::string& scope,
                          const std::vector<ScriptMetadata>& registry,
                          ResolvedProgram& out) {
    // One static presence per enclosed call regardless of trip count or
    // taken branch; binding plumbing for lets inside control flow works
    // exactly as at top level because every PipelineStmt still flows
    // through resolve_stmt.
    walker::for_each_pipeline_stmt_in_block(
        block, [&](const PipelineStmt& pipe) {
            resolve_stmt(pipe, scope, registry, out);
        });
}

}  // namespace detail

// Resolve every call in all rules and investigations of a program.
inline ResolvedProgram resolve_program(
    const Program& prog, const std::vector<ScriptMetadata>& registry) {
    ResolvedProgram out;
    for (const RuleDecl& rule : prog.rules) {
        detail::resolve_block(rule.body, "rule " + rule.name, registry, out);
    }
    for (const InvestigationDecl& inv : prog.investigations) {
        detail::resolve_block(inv.body, "investigate " + inv.name, registry,
                              out);
    }
    return out;
}

}  // namespace jocky
