#pragma once
// JOCKY AST walker — Phase 5.5 shared recursive traversal.
//
// Exactly one place in the codebase knows how to find every CallExpr in
// a program: this header. Before control flow existed, call_resolver
// owned an ad-hoc traversal (resolve_value / resolve_predicate /
// resolve_op / resolve_stmt); adding If/For/While bodies to three
// separate passes would have tripled the recursion and guaranteed they
// drift apart. Instead every pass that collects calls uses these
// `for_each_call_in_*` visitors, which recurse into pipeline heads,
// predicate operands (including nested calls and lists), and — since
// Phase 5.5 — IfStmt then/else blocks, ForStmt bodies, and WhileStmt
// bodies at ANY depth. A call buried three levels deep (if -> for -> if)
// is visited exactly as a top-level call; see tests/phase5.5/ fixture 6.
//
// Visitors take `Fn&&` invoked as `fn(const CallExpr&)` in source order.
// The walker never resolves, binds, or gates — it only FINDS calls. What
// each pass does with them stays in that pass.

#include <utility>
#include <variant>

#include "jocky/ast/ast.hpp"

namespace jocky {
namespace walker {

template <typename Fn>
void for_each_call_in_value(const ExprValue& value, Fn&& fn);

template <typename Fn>
void for_each_call_in_predicate(const Predicate& pred, Fn&& fn) {
    switch (pred.kind) {
        case Predicate::Kind::Or:
        case Predicate::Kind::And:
            for (const auto& operand : pred.operands) {
                for_each_call_in_predicate(*operand, fn);
            }
            break;
        case Predicate::Kind::Not:
            for_each_call_in_predicate(*pred.inner, fn);
            break;
        case Predicate::Kind::Compare:
            for_each_call_in_value(*pred.left, fn);
            for_each_call_in_value(*pred.right, fn);
            break;
        case Predicate::Kind::Atom:
            for_each_call_in_value(*pred.atom, fn);
            break;
    }
}

template <typename Fn>
void for_each_call_in_value(const ExprValue& value, Fn&& fn) {
    switch (value.kind) {
        case ExprValue::Kind::Call: fn(*value.call); break;
        case ExprValue::Kind::List:
            for (const auto& item : value.list) {
                for_each_call_in_value(*item, fn);
            }
            break;
        default: break;
    }
}

// True when a predicate embeds at least one `call` operand. The
// while-loop executor uses this to honor a zero iteration ceiling
// without spawning: a call-free condition can always be evaluated
// (so natural exit needs no dispatch), while a call-bearing one
// cannot be decided without dispatching — so the ceiling wins.
inline bool predicate_has_calls(const Predicate& pred) {
    bool found = false;
    for_each_call_in_predicate(
        pred, [&](const CallExpr&) { found = true; });
    return found;
}

template <typename Fn>
void for_each_call_in_pipeline(const PipelineExpr& expr, Fn&& fn) {
    for_each_call_in_value(*expr.head, fn);
    for (const PipelineStep& step : expr.steps) {
        switch (step.op.kind) {
            case PipelineOp::Kind::Filter:
            case PipelineOp::Kind::Where:
            case PipelineOp::Kind::Having:
                for_each_call_in_predicate(*step.op.predicate, fn);
                break;
            default: break;
        }
    }
}

template <typename Fn>
void for_each_call_in_block(const Block& block, Fn&& fn);

template <typename Fn>
void for_each_call_in_stmt(const Stmt& stmt, Fn&& fn) {
    if (const auto* pipe = std::get_if<PipelineStmt>(&stmt.node)) {
        for_each_call_in_pipeline(pipe->expr, fn);
    } else if (const auto* branch = std::get_if<IfStmt>(&stmt.node)) {
        // Conservative static analysis: BOTH branches are reachable as
        // far as any semantic pass is concerned (the runtime choice is
        // unknowable at compile time) — so both are walked. The gate
        // therefore authorizes both branches' capabilities; see
        // wiki/17-control-flow.md.
        for_each_call_in_predicate(branch->cond, fn);
        for_each_call_in_block(branch->then_block, fn);
        if (branch->else_block.has_value()) {
            for_each_call_in_block(*branch->else_block, fn);
        }
    } else if (const auto* loop = std::get_if<ForStmt>(&stmt.node)) {
        // One static presence per enclosed call regardless of trip count:
        // tree-shaking cares WHICH functions, not how many invocations.
        for_each_call_in_block(loop->body, fn);
    } else if (const auto* loop = std::get_if<WhileStmt>(&stmt.node)) {
        for_each_call_in_predicate(loop->cond, fn);
        for_each_call_in_block(loop->body, fn);
    }
}

template <typename Fn>
void for_each_call_in_block(const Block& block, Fn&& fn) {
    for (const auto& stmt : block) {
        for_each_call_in_stmt(stmt, fn);
    }
}

// Statement-level sibling of the call collector above: visits every
// PipelineStmt in source order, descending into If/For/While bodies.
// Passes that need per-statement context (call_resolver's let-binding
// plumbing) use this; passes that only need calls use
// for_each_call_in_block. The recursive SHAPE (which blocks exist under
// which node) is defined once here — the two entry points differ only in
// payload, and fixture 6 (tests/phase5.5/) proves they agree.
template <typename Fn>
void for_each_pipeline_stmt_in_block(const Block& block, Fn&& fn) {
    for (const auto& stmt : block) {
        if (const auto* pipe = std::get_if<PipelineStmt>(&stmt.node)) {
            fn(*pipe);
        } else if (const auto* branch = std::get_if<IfStmt>(&stmt.node)) {
            for_each_pipeline_stmt_in_block(branch->then_block, fn);
            if (branch->else_block.has_value()) {
                for_each_pipeline_stmt_in_block(*branch->else_block, fn);
            }
        } else if (const auto* loop = std::get_if<ForStmt>(&stmt.node)) {
            for_each_pipeline_stmt_in_block(loop->body, fn);
        } else if (const auto* loop = std::get_if<WhileStmt>(&stmt.node)) {
            for_each_pipeline_stmt_in_block(loop->body, fn);
        }
    }
}

}  // namespace walker
}  // namespace jocky
