#pragma once
// JOCKY AST — node structs for .jky programs (Phase 1 skeleton).
// Pure data: no parsing or semantic behavior lives here. The pretty
// printer in src/cli/main.cpp renders these nodes for `jocky check`.

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace jocky {

// A dotted field path such as `src_ip` or `flow.bytes`.
struct FieldRef {
    std::vector<std::string> path;
};

struct CallExpr;  // forward: mutually recursive with ExprValue.

// `correlate(a, b) within <n><unit> on <fields>` — a table-valued
// join usable both as an expression head (`let t = correlate(...) ...`)
// and as a pipeline operator, per wiki/06-api-contracts.md.
struct CorrelateSpec {
    std::string left;
    std::string right;
    std::int64_t within_value = 0;
    std::string within_unit;  // ms | s | m | h | d
    std::vector<FieldRef> on_fields;
};

// A value-level expression: literal, field reference, call result,
// list, correlate join, or explicit evidence-source reference.
struct ExprValue {
    enum class Kind {
        String,
        Int,
        Float,
        Bool,
        Field,
        Call,
        List,
        Source,
        Correlate,
    };
    Kind kind = Kind::String;
    std::string str;
    std::int64_t integer = 0;
    double floating = 0.0;
    bool boolean = false;
    FieldRef field;
    std::unique_ptr<CallExpr> call;
    std::vector<std::unique_ptr<ExprValue>> list;
    std::string source;  // evidence name for Kind::Source
    CorrelateSpec correlate;  // join spec for Kind::Correlate
};

// A registry-function invocation: `call a.b.c(x: <expr>, ...)`.
// line/col locate the `call` keyword (Phase 3: semantic errors reuse the
// lexer's file:line:col diagnostic style); each NamedArg carries the
// position of its argument name for arity/type errors.
struct CallExpr {
    struct NamedArg {
        std::string name;
        std::unique_ptr<ExprValue> value;
        int line = 0;
        int col = 0;
    };
    std::vector<std::string> function;
    std::vector<NamedArg> args;
    int line = 0;
    int col = 0;
};

// A boolean predicate over rows (`filter`/`where`/`having`).
struct Predicate {
    enum class Kind {
        Or,
        And,
        Not,
        Compare,
        Atom,
    };
    Kind kind = Kind::Atom;
    std::vector<std::unique_ptr<Predicate>> operands;  // Or / And
    std::unique_ptr<Predicate> inner;                  // Not
    std::string op;  // Compare: one of == != < <= > >= in contains
                     // contains_any
    std::unique_ptr<ExprValue> left;   // Compare
    std::unique_ptr<ExprValue> right;  // Compare
    std::unique_ptr<ExprValue> atom;   // Atom
};

struct SelectItem {
    FieldRef field;
    std::string alias;
    bool has_alias = false;
};

// One `| operator` pipeline stage.
struct PipelineOp {
    enum class Kind {
        Filter,
        Select,
        Correlate,
        Write,
        Where,
        GroupBy,
        Having,
        SortBy,
        Limit,
        Emit,
    };
    Kind kind = Kind::Filter;
    std::unique_ptr<Predicate> predicate;  // Filter / Where / Having
    std::vector<SelectItem> select_items;  // Select
    CorrelateSpec correlate;               // Correlate
    // Write: plain path, or report(path).
    std::string write_path;
    bool write_is_report = false;
    std::vector<FieldRef> group_fields;  // GroupBy
    std::vector<FieldRef> sort_fields;   // SortBy
    std::string sort_dir;                // "" | asc | desc
    std::int64_t limit = 0;              // Limit
    std::string emit_target;             // Emit
    bool emit_is_string = false;
};

struct PipelineStep {
    PipelineOp op;
};

// A full `head | op | op ...` chain.
struct PipelineExpr {
    std::unique_ptr<ExprValue> head;
    std::vector<PipelineStep> steps;
};

// `[let <name> =] <pipeline> ;`
struct PipelineStmt {
    std::string binding;
    bool has_binding = false;
    PipelineExpr expr;
};

struct TypeRef {
    std::string name;
    std::vector<TypeRef> args;  // e.g. table<flow>
};

struct Param {
    std::string name;
    TypeRef type;
};

struct RuleDecl {
    std::string name;
    std::vector<Param> params;
    TypeRef returns;
    std::vector<PipelineStmt> body;
};

struct InvestigationDecl {
    std::string name;
    std::vector<PipelineStmt> body;
};

struct CaseDecl {
    std::string name;
    std::vector<std::string> capabilities;
};

struct EvidenceDecl {
    std::string name;
    std::string adapter;  // pcap | eventlog | directory
    std::string path;
};

struct Program {
    std::vector<CaseDecl> cases;
    std::vector<EvidenceDecl> evidence;
    std::vector<RuleDecl> rules;
    std::vector<InvestigationDecl> investigations;
};

}  // namespace jocky
