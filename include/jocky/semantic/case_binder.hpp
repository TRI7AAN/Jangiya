#pragma once
// JOCKY case binder — Phase 4 authorization, first half of the static gate.
//
// A resolved program is not yet an authorized program. The binder takes a
// parsed Program plus the ResolvedProgram produced by call_resolver and
// attaches every resolved call to its authorizing case, yielding a
// BoundProgram that capability_gate checks. Binding rule (locked for
// Phase 4): exactly one `case` block per Program. Zero or multiple case
// blocks is a semantic error — an investigation without a single declared
// authorization scope must not resolve, let alone run. Every rule and
// investigation is implicitly bound to that one case; no `for case X`
// syntax exists.
//
// The binder also enforces the PART 0 distinction: a case that never wrote
// `allowed_capabilities` (CaseDecl::capabilities_declared == false) fails
// HERE, at bind time, while a case that wrote `allowed_capabilities: []`
// binds fine and fails LATER, at gate time. Fixture 3 vs fixture 4 in
// tests/phase4/ prove these are different code paths, not different
// messages: BindingError (thrown here, pre-gate) vs GateResult::Denied
// (returned by capability_gate, post-binding).
//
// Layering: this pass consumes call_resolver's output and never re-resolves
// calls. call_resolver.hpp is untouched by Phase 4 (see wiki/13 for the
// rationale). All failures throw BindingError; line/col are 0 when the
// error is program-level (zero cases, duplicate cases, missing field) —
// there is no single token to point at. The CLI prints those as
// `error: <file>: <message> (case binding)`.

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "jocky/ast/ast.hpp"
#include "jocky/semantic/call_resolver.hpp"

namespace jocky {

// Thrown when a program cannot be bound to a single authorizing case.
// Mirrors SemanticError's style, minus guaranteed positions.
class BindingError : public std::runtime_error {
public:
    int line;
    int col;
    BindingError(int line, int col, const std::string& message)
        : std::runtime_error(message), line(line), col(col) {}
};

struct BoundProgram {
    CaseDecl bound_case;       // the single authorizing case
    ResolvedProgram resolved;  // every resolved call, implicitly bound to it
};

// Bind a parsed program and its resolved calls to the program's single
// case. Takes the resolver output by value (moved in). Never returns a
// partial result: any failure throws BindingError before gating runs.
inline BoundProgram bind_program(const Program& prog,
                                 ResolvedProgram resolved) {
    if (prog.cases.empty()) {
        throw BindingError(
            0, 0,
            "no case block: a program must declare exactly one case "
            "before any investigation can be authorized (found 0)");
    }
    if (prog.cases.size() > 1) {
        std::string names;
        for (std::size_t i = 0; i < prog.cases.size(); ++i) {
            if (i != 0) {
                names += ", ";
            }
            names += "'" + prog.cases[i].name + "'";
        }
        throw BindingError(
            0, 0,
            "multiple case blocks (" + names +
                "): a program must declare exactly one case so every "
                "call binds to a single authorization scope");
    }
    const CaseDecl& bound = prog.cases.front();
    if (!bound.capabilities_declared) {
        throw BindingError(
            0, 0,
            "case '" + bound.name +
                "' declares no allowed_capabilities field: an authorizing "
                "case must explicitly list its granted capabilities "
                "(write `allowed_capabilities: [...]`, even if empty)");
    }
    BoundProgram out;
    out.bound_case = bound;
    out.resolved = std::move(resolved);
    return out;
}

}  // namespace jocky
