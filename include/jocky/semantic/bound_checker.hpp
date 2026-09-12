#pragma once
// JOCKY for-loop bound checker — Phase 5.5 semantic validation.
//
// The parser accepts the full C-style `for` shape but deliberately does
// NOT judge whether the bound is safe (see parser.hpp). This pass does:
// every ForStmt bound must resolve to one of exactly three statically
// boundable forms, checked here:
//
//   1. Integer literal — `for (int i = 0; i < 5; i++)`.
//   2. A name bound to an integer literal in an enclosing scope —
//      `let N = 5; … for (int i = 0; i < N; i++)`. NOTE on terminology:
//      the session brief calls these "case-level declared constants,"
//      but case blocks grammatically hold ONLY allowed_capabilities
//      (any other field is "unknown case field"), so no such constants
//      can exist there. The real, implemented rule: an integer `let`
//      binding (`let NAME = <int literal>;`) declared in an enclosing
//      block BEFORE the loop. Lookup walks outward through nested
//      scopes; the loop variable itself is never a constant.
//   3. `count(<table>)` — `for (int i = 0; i < count(rows); i++)`, where
//      the target must name a `let` binding visible in an enclosing
//      scope (any bound name — the count is evaluated at run time, but
//      the REFERENCE is statically pinned, so the loop stays bounded by
//      a value fixed before the loop starts).
//
// Anything else fails with BoundCheckError naming the actual bound found
// and why it is not statically boundable — in particular a bound
// derived from a call result (`let r = call …; for (…; i < r; …)`)
// is rejected: a trip count flowing from unvalidated runtime data
// reintroduces unboundedness, the exact thing this gate exists to bar.
// Runs after resolution in the CLI chain (resolve → bound-check → bind):
// the calls a bad bound may reference must themselves resolve first so
// "unknown function" errors keep their existing precedence.

#include <map>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

#include "jocky/ast/ast.hpp"

namespace jocky {

// Thrown when a ForStmt bound is not one of the three allowed forms.
// Carries the bound's own line/col (recorded by the parser).
class BoundCheckError : public std::runtime_error {
public:
    int line;
    int col;
    BoundCheckError(int line, int col, const std::string& message)
        : std::runtime_error(message), line(line), col(col) {}
};

namespace detail {

struct BoundScope {
    // Integer constants: name -> literal value.
    std::map<std::string, std::int64_t> int_consts;
    // Every let-bound name (any head kind) visible for count() targets.
    std::map<std::string, bool> bound_names;
};

inline const std::int64_t* find_int_const(
    const std::vector<BoundScope>& scopes, const std::string& name) {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto found = it->int_consts.find(name);
        if (found != it->int_consts.end()) {
            return &found->second;
        }
    }
    return nullptr;
}

inline bool is_bound_name(const std::vector<BoundScope>& scopes,
                          const std::string& name) {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        if (it->bound_names.count(name) != 0) {
            return true;
        }
    }
    return false;
}

inline void check_block(const Block& block, std::vector<BoundScope>& scopes);

inline void check_stmt(const Stmt& stmt, std::vector<BoundScope>& scopes) {
    if (const auto* pipe = std::get_if<PipelineStmt>(&stmt.node)) {
        // Record let bindings for later bounds: integer literals become
        // constants; every other head kind registers the name as bound
        // (usable as a count() target, never as an integer bound).
        if (pipe->has_binding) {
            scopes.back().bound_names[pipe->binding] = true;
            if (pipe->expr.head->kind == ExprValue::Kind::Int) {
                scopes.back().int_consts[pipe->binding] =
                    pipe->expr.head->integer;
            }
        }
        return;
    }
    if (const auto* branch = std::get_if<IfStmt>(&stmt.node)) {
        scopes.emplace_back();
        check_block(branch->then_block, scopes);
        scopes.pop_back();
        if (branch->else_block.has_value()) {
            scopes.emplace_back();
            check_block(*branch->else_block, scopes);
            scopes.pop_back();
        }
        return;
    }
    if (const auto* loop = std::get_if<ForStmt>(&stmt.node)) {
        const BoundExpr& bound = loop->bound;
        switch (bound.kind) {
            case BoundExpr::Kind::Literal: break;  // always boundable
            case BoundExpr::Kind::Count:
                if (!is_bound_name(scopes, bound.ident)) {
                    throw BoundCheckError(
                        bound.line, bound.col,
                        "count() target '" + bound.ident +
                            "' is not a visible let binding (bind the "
                            "table before the loop)");
                }
                break;
            case BoundExpr::Kind::Ident: {
                const std::int64_t* known =
                    find_int_const(scopes, bound.ident);
                if (known == nullptr) {
                    if (is_bound_name(scopes, bound.ident)) {
                        throw BoundCheckError(
                            bound.line, bound.col,
                            "loop bound '" + bound.ident +
                                "' is bound to a non-constant value (a "
                                "trip count derived from call results or "
                                "other runtime data is not statically "
                                "boundable; use an integer literal, an "
                                "integer let-binding, or count(name))");
                    }
                    throw BoundCheckError(
                        bound.line, bound.col,
                        "loop bound '" + bound.ident +
                            "' is not a statically known bound (no "
                            "integer let-binding with that name is "
                            "visible; use an integer literal, an integer "
                            "let-binding, or count(name))");
                }
                break;
            }
        }
        // The loop variable is scoped to the body and is NOT a constant.
        scopes.emplace_back();
        check_block(loop->body, scopes);
        scopes.pop_back();
        return;
    }
    if (const auto* loop = std::get_if<WhileStmt>(&stmt.node)) {
        scopes.emplace_back();
        check_block(loop->body, scopes);
        scopes.pop_back();
        return;
    }
}

inline void check_block(const Block& block, std::vector<BoundScope>& scopes) {
    for (const auto& stmt : block) {
        check_stmt(stmt, scopes);
    }
}

}  // namespace detail

// Validate every ForStmt bound in all rules and investigations.
// Never returns a partial result: any failure throws BoundCheckError.
inline void check_bounds(const Program& prog) {
    for (const RuleDecl& rule : prog.rules) {
        std::vector<detail::BoundScope> scopes(1);
        detail::check_block(rule.body, scopes);
    }
    for (const InvestigationDecl& inv : prog.investigations) {
        std::vector<detail::BoundScope> scopes(1);
        detail::check_block(inv.body, scopes);
    }
}

}  // namespace jocky
