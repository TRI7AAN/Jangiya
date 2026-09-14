#pragma once
// JOCKY execution-time control-flow interpreter — Phase 7.5.
//
// Root cause it fixes (audit GAP-1/GAP-2, wiki/18-phase6-7-audit.md
// §2.3): the Phase 7 dispatcher executed a flattened authorized-call
// list with no regard for IfStmt/ForStmt/WhileStmt structure — both
// branches ran, loops ran once, and no iteration ceiling existed
// anywhere despite the wiki/17 §3 MUST.
//
// This header walks the program's REAL statement tree during execution
// (rules then investigations, mirroring resolve_program order):
// - IfStmt: the condition is evaluated via runtime/predicate_evaluator.hpp
//   and ONLY the taken branch dispatches. Untaken-branch calls never
//   spawn, never appear in the manifest — not even as denials.
// - ForStmt: the body runs bound-start times (bound forms already
//   validated by Phase 5.5 bound_checker; here they are executed for
//   real). Every iteration dispatches independently, so one call in a
//   3-iteration body yields 3 manifest entries.
// - WhileStmt: the condition is re-evaluated before each iteration and a
//   HARD iteration ceiling is enforced (default 10000, overridable per
//   case via `max_while_iterations` and per run via
//   RuntimeOptions::max_while_iterations). Hitting it records a distinct
//   "while_ceiling" entry and fails the run — never silent truncation.
//
// Sharing, not forking: every attempt goes through
// runtime_detail::dispatch_single_call — the SAME capability re-check,
// integrity re-check, argument validation, sandbox, timeout, and
// manifest-before-spawn path as flat dispatch. Tree mode and flat mode
// cannot drift apart on any per-attempt property.
//
// Fail-closed aborts: unresolvable conditions, unmappable call sites,
// and unknowable bounds throw ExecAbort, which stops the run with a
// manifest errors[] entry and failed status. A branch is never chosen
// on a guess.
//
// Loop/budget interaction (documented, not incidental): each dispatched
// attempt consumes the shared max_executions budget. If the budget is
// exhausted INSIDE a loop, the run aborts with an error instead of
// emitting an unbounded stream of ceiling_denied entries (flat runs
// keep the legacy per-attempt ceiling_denied behavior: their call lists
// are finite, so it stays bounded there).

#include <cstdint>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "jocky/ast/ast.hpp"
#include "jocky/ast/walker.hpp"
#include "jocky/lexer/lexer.hpp"
#include "jocky/parser/parser.hpp"
#include "jocky/runtime/dispatcher.hpp"
#include "jocky/runtime/predicate_evaluator.hpp"

namespace jocky {
namespace runtime_detail {

// Fail-closed run abort from tree execution. The message is already
// manifest-logged by execute_plan_tree; nothing else to do with it.
class ExecAbort : public std::runtime_error {
public:
    explicit ExecAbort(const std::string& message)
        : std::runtime_error(message) {}
};

namespace cf_detail {

inline std::string call_key(int line, int col) {
    return std::to_string(line) + ":" + std::to_string(col);
}

struct ExecContext {
    DispatchState state;
    // (line, col) of every `call` keyword -> index into plan.calls.
    std::map<std::pair<int, int>, std::size_t> call_index;
    // Predicate operand values (see predicate_evaluator.hpp).
    ExecValues values;
    // let-bound names known (any head kind) for count() targeting.
    std::map<std::string, bool> known_bindings;
    // Integer-let constants with lexical scope stack (mirrors
    // bound_checker: one root scope per body, fresh scopes for
    // if-branches and loop bodies, sequential lets share a scope).
    std::vector<std::map<std::string, std::int64_t>> int_scopes;
};

inline const std::int64_t* find_int_const(
    const std::vector<std::map<std::string, std::int64_t>>& scopes,
    const std::string& name) {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        const auto found = it->find(name);
        if (found != it->end()) return &found->second;
    }
    return nullptr;
}

// Dispatch one planned call and record its observable output under key.
// Returns the terminal outcome string for callers that branch on it.
inline std::string dispatch_tracked(const RuntimeCall& call,
                                    const std::string& output_key,
                                    bool is_binding,
                                    const std::string& binding_name,
                                    ExecContext& ctx) {
    dispatch_single_call(call, ctx.state);
    const ExecutionRecord& entry =
        ctx.state.manifest->executions.back();
    if (!output_key.empty()) {
        ctx.values.call_outputs[output_key] = entry.stdout_text;
    }
    if (is_binding) {
        ctx.values.bindings[binding_name] = entry.stdout_text;
    }
    return entry.outcome;
}

inline const RuntimeCall& lookup_call(const CallExpr& node,
                                      const ExecContext& ctx) {
    const auto found = ctx.call_index.find({node.line, node.col});
    if (found == ctx.call_index.end()) {
        throw ExecAbort("no runtime call for source call at " +
                        call_key(node.line, node.col) +
                        " (plan and program disagree; refusing to guess)");
    }
    return ctx.state.plan->calls[found->second];
}

// Dispatch every call embedded in a condition predicate, in walker
// order, recording outputs for later evaluation. The condition itself
// is evaluated afterwards by the caller.
inline void dispatch_condition_calls(const Predicate& cond,
                                     ExecContext& ctx) {
    walker::for_each_call_in_predicate(
        cond, [&](const CallExpr& found) {
            const RuntimeCall& call = lookup_call(found, ctx);
            dispatch_tracked(call, call_key(found.line, found.col),
                             false, "", ctx);
        });
}

// Abort the run if the shared execution budget is exhausted. Called at
// loop-iteration boundaries so loops cannot emit an unbounded stream of
// per-attempt entries (flat runs keep their finite legacy behavior).
inline void check_loop_budget(const ExecContext& ctx, const char* loop) {
    if (*ctx.state.execution_count >= ctx.state.options->max_executions) {
        throw ExecAbort(std::string(loop) +
                        " exceeded the runtime execution budget (" +
                        std::to_string(ctx.state.options->max_executions) +
                        " attempts); run aborted inside the loop");
    }
}

inline bool eval_condition(const Predicate& cond, const ExecContext& ctx,
                           const char* where) {
    try {
        return evaluate_predicate(cond, ctx.values);
    } catch (const PredicateError& ex) {
        throw ExecAbort(std::string("cannot decide ") + where +
                        " condition: " + ex.what());
    }
}

// Count the rows of a call-produced table for count(name) bounds: one
// per line of captured stdout (a trailing newline starts no phantom
// row). Non-call bindings have no materialized rows — the static check
// only guarantees visibility, so an invisible table aborts fail-closed
// instead of silently looping zero times.
inline std::int64_t count_binding_rows(const std::string& name,
                                       const ExecContext& ctx) {
    const auto produced = ctx.values.bindings.find(name);
    if (produced == ctx.values.bindings.end()) {
        throw ExecAbort("cannot evaluate count(" + name +
                        ") (no call-produced value bound under that name; "
                        "table contents are not materialized at dispatch)");
    }
    std::int64_t rows = 0;
    std::istringstream text(produced->second);
    std::string line;
    while (std::getline(text, line)) ++rows;
    return rows;
}

inline std::int64_t resolve_bound(const BoundExpr& bound,
                                  const ExecContext& ctx) {
    switch (bound.kind) {
        case BoundExpr::Kind::Literal:
            return bound.literal;
        case BoundExpr::Kind::Ident: {
            const std::int64_t* known =
                find_int_const(ctx.int_scopes, bound.ident);
            if (known == nullptr) {
                throw ExecAbort(
                    "loop bound '" + bound.ident +
                    "' has no recorded integer value (static check passed "
                    "but the binding never executed; refusing to guess)");
            }
            return *known;
        }
        case BoundExpr::Kind::Count:
            return count_binding_rows(bound.ident, ctx);
    }
    throw ExecAbort("loop bound has an unknown form");
}

inline void execute_block(const Block& block, ExecContext& ctx);

inline void execute_pipeline_stmt(const PipelineStmt& stmt,
                                  ExecContext& ctx) {
    // Walker order: head call first, then predicate-embedded calls.
    // The head (when it is a call) is the statement's main dispatch and
    // owns the `let` binding; embedded calls only feed the evaluator.
    const CallExpr* head_call = nullptr;
    if (stmt.expr.head->kind == ExprValue::Kind::Call) {
        head_call = stmt.expr.head->call.get();
    }
    walker::for_each_call_in_pipeline(
        stmt.expr, [&](const CallExpr& found) {
            const RuntimeCall& call = lookup_call(found, ctx);
            const bool is_head = (head_call != nullptr && &found == head_call);
            if (is_head) {
                dispatch_tracked(call, call_key(found.line, found.col),
                                 stmt.has_binding, stmt.binding, ctx);
            } else {
                dispatch_tracked(call, call_key(found.line, found.col),
                                 false, "", ctx);
            }
        });
    // Record let bindings for later bounds/conditions. Integer heads
    // become scope constants (mirrors bound_checker); every other head
    // kind only marks the name known (count() over it aborts, field
    // references to it abort — fail-closed, see header docs).
    if (stmt.has_binding) {
        ctx.known_bindings[stmt.binding] = true;
        if (stmt.expr.head->kind == ExprValue::Kind::Int) {
            ctx.int_scopes.back()[stmt.binding] =
                stmt.expr.head->integer;
        }
    }
}

inline void execute_if_stmt(const IfStmt& branch, ExecContext& ctx) {
    dispatch_condition_calls(branch.cond, ctx);
    const bool taken = eval_condition(branch.cond, ctx, "if");
    // Fresh scope per taken branch (mirrors bound_checker); the untaken
    // branch dispatches nothing and leaves no manifest trace.
    ctx.int_scopes.emplace_back();
    if (taken) {
        execute_block(branch.then_block, ctx);
    } else if (branch.else_block.has_value()) {
        execute_block(*branch.else_block, ctx);
    }
    ctx.int_scopes.pop_back();
}

inline void execute_for_stmt(const ForStmt& loop, ExecContext& ctx) {
    const std::int64_t bound = resolve_bound(loop.bound, ctx);
    std::int64_t trips = bound - loop.start;
    if (trips < 0) trips = 0;
    // One scope for the whole body (mirrors bound_checker); the loop
    // variable itself is never recorded as a constant.
    ctx.int_scopes.emplace_back();
    for (std::int64_t iter = 0; iter < trips; ++iter) {
        check_loop_budget(ctx, "for loop");
        execute_block(loop.body, ctx);
    }
    ctx.int_scopes.pop_back();
}

inline void execute_while_stmt(const WhileStmt& loop, ExecContext& ctx) {
    const std::size_t configured = ctx.state.plan->max_while_iterations;
    const std::size_t ceiling =
        (configured > 0) ? configured
                         : ctx.state.options->max_while_iterations;
    const bool cond_has_calls = walker::predicate_has_calls(loop.cond);
    ctx.int_scopes.emplace_back();
    for (std::size_t iter = 0;; ++iter) {
        if (iter >= ceiling && cond_has_calls) {
            // A call-bearing condition cannot be decided without
            // dispatching, so at/past the ceiling the ceiling wins:
            // record the marker instead of spawning for a loop whose
            // body is disabled (notably ceiling 0 dispatches nothing).
            // Call-free conditions fall through and evaluate below,
            // so a naturally-false condition exits cleanly even at
            // the boundary.
            // Distinct, visible outcome — never silent truncation.
            // The run still fails overall (the program did not complete
            // as written), consistent with every other non-success
            // outcome flipping all_ok.
            ExecutionRecord marker;
            marker.function = "<while-loop>";
            marker.start_utc = utc_now();
            marker.outcome = "while_ceiling";
            marker.stderr_text =
                "while loop iteration ceiling (" +
                std::to_string(ceiling) +
                ") reached; loop aborted after " + std::to_string(iter) +
                " iterations";
            finalize_entry(marker);
            ctx.state.manifest->executions.push_back(marker);
            *ctx.state.all_ok = false;
            persist_manifest(ctx.state.manifest_path, *ctx.state.manifest);
            break;
        }
        check_loop_budget(ctx, "while loop");
        dispatch_condition_calls(loop.cond, ctx);
        if (!eval_condition(loop.cond, ctx, "while")) break;
        if (iter >= ceiling) {
            // Call-free condition evaluated true at/past the ceiling:
            // same distinct marker as above (the loop did not complete
            // as written), reached without any dispatch.
            ExecutionRecord marker;
            marker.function = "<while-loop>";
            marker.start_utc = utc_now();
            marker.outcome = "while_ceiling";
            marker.stderr_text =
                "while loop iteration ceiling (" +
                std::to_string(ceiling) +
                ") reached; loop aborted after " + std::to_string(iter) +
                " iterations";
            finalize_entry(marker);
            ctx.state.manifest->executions.push_back(marker);
            *ctx.state.all_ok = false;
            persist_manifest(ctx.state.manifest_path, *ctx.state.manifest);
            break;
        }
        execute_block(loop.body, ctx);
    }
    ctx.int_scopes.pop_back();
}

inline void execute_block(const Block& block, ExecContext& ctx) {
    for (const Stmt& stmt : block) {
        if (const auto* pipe = std::get_if<PipelineStmt>(&stmt.node)) {
            execute_pipeline_stmt(*pipe, ctx);
        } else if (const auto* branch = std::get_if<IfStmt>(&stmt.node)) {
            execute_if_stmt(*branch, ctx);
        } else if (const auto* loop = std::get_if<ForStmt>(&stmt.node)) {
            execute_for_stmt(*loop, ctx);
        } else if (const auto* loop = std::get_if<WhileStmt>(&stmt.node)) {
            execute_while_stmt(*loop, ctx);
        }
    }
}

}  // namespace cf_detail

// Walk the embedded program tree against the planned calls. Returns
// true on full completion, false on fail-closed abort (already
// manifest-logged, all_ok already false). Declared here in the header
// the dispatcher forward-declares it from.
inline bool execute_plan_tree(const RuntimePlan& plan,
                              DispatchState& state) {
    Program prog;
    try {
        Lexer lexer(plan.program_source);
        Parser parser(lexer.tokenize());
        prog = parser.parse_program();
    } catch (const std::exception& ex) {
        state.manifest->errors.push_back(
            std::string("control-flow re-parse failed: ") + ex.what());
        *state.all_ok = false;
        persist_manifest(state.manifest_path, *state.manifest);
        return false;
    }
    cf_detail::ExecContext ctx;
    ctx.state = state;
    for (std::size_t i = 0; i < plan.calls.size(); ++i) {
        ctx.call_index[{plan.calls[i].line, plan.calls[i].col}] = i;
    }
    try {
        // Rules then investigations: the exact resolve_program order,
        // so bindings become visible in the same sequence the static
        // passes assumed.
        for (const RuleDecl& rule : prog.rules) {
            ctx.int_scopes.emplace_back();
            cf_detail::execute_block(rule.body, ctx);
            ctx.int_scopes.pop_back();
        }
        for (const InvestigationDecl& inv : prog.investigations) {
            ctx.int_scopes.emplace_back();
            cf_detail::execute_block(inv.body, ctx);
            ctx.int_scopes.pop_back();
        }
    } catch (const ExecAbort& ex) {
        state.manifest->errors.push_back(
            std::string("control-flow abort: ") + ex.what());
        *state.all_ok = false;
        persist_manifest(state.manifest_path, *state.manifest);
        return false;
    }
    return true;
}

}  // namespace runtime_detail
}  // namespace jocky
