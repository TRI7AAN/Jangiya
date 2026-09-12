#pragma once
// JOCKY capability gate — Phase 4 authorization, second half of the static
// gate (static half of AGENTS.md §2.2).
//
// The gate consumes a BoundProgram (single case + all resolved calls) and
// checks every resolved call's recorded capability against the case's
// CaseDecl::capabilities. It is a SEPARATE pass over call_resolver output,
// not merged into resolve_program: resolution proves a call is
// well-formed, binding proves it has exactly one authorization scope, and
// gating proves it is allowed. Each stage has its own header, error type,
// and tests (see wiki/13 for the layering rationale, matching the Phase
// 2/3 isolated-testability pattern).
//
// Fail-closed with full reporting: if ANY resolved call's capability is not
// a member of the case list, the whole compilation fails — but ALL denials
// are collected before reporting, never just the first, so one run shows
// the complete authorization gap. Denials are returned, not thrown: a
// GateResult is either Allowed (full authorized-call list, the input Phase
// 5 tree-shaking operates on) or Denied (full violation list). Each
// violation names the function, the required capability, the case, and the
// call-site line:col.
//
// What this gate does NOT do (deliberate, see wiki/13 "Why this isn't
// sufficient alone"): enforce anything at run time. Phase 7's dispatcher
// must independently re-check capability before executing as defense in
// depth; a static pass can never substitute for the dispatch check.

#include <string>
#include <vector>

#include "jocky/semantic/call_resolver.hpp"
#include "jocky/semantic/case_binder.hpp"

namespace jocky {

struct GateViolation {
    std::string function;    // dotted registry name of the denied call
    std::string capability;  // required capability the case did not grant
    std::string case_name;   // the single bound case
    int line = 0;            // call-site position (from ResolvedCall)
    int col = 0;
};

struct GateResult {
    std::string case_name;
    bool allowed = false;  // true iff violations is empty (fail-closed)
    std::vector<ResolvedCall> authorized;  // granted calls, source order
    std::vector<GateViolation> violations;  // denied calls, source order
};

// Check every bound call against the bound case's capability list.
// Pure: collects all violations, never throws on denials.
inline GateResult check_gate(const BoundProgram& bound) {
    GateResult result;
    result.case_name = bound.bound_case.name;
    for (const ResolvedCall& call : bound.resolved.calls) {
        bool granted = false;
        for (const std::string& allowed : bound.bound_case.capabilities) {
            if (allowed == call.capability) {
                granted = true;
                break;
            }
        }
        if (granted) {
            result.authorized.push_back(call);
        } else {
            GateViolation violation;
            violation.function = call.function;
            violation.capability = call.capability;
            violation.case_name = bound.bound_case.name;
            violation.line = call.line;
            violation.col = call.col;
            result.violations.push_back(std::move(violation));
        }
    }
    result.allowed = result.violations.empty();
    return result;
}

}  // namespace jocky
