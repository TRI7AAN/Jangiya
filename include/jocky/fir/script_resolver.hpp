#pragma once
// JOCKY script resolver (tree-shaker) — Phase 5 build planning.
//
// Phase 4 proves every call is *allowed*; this pass computes exactly what
// must be *shipped* for those allowed calls — directly called functions
// plus transitive `depends_on` — so Phase 6 (`jockyc`) embeds the minimum
// necessary set and nothing more. It operates on `GateResult::authorized`
// (never on raw resolved calls: denied calls contribute nothing, so a
// denied capability can never pull a script into a build) plus the full
// `ScriptMetadata` registry. `depends_on` has been parsed, stored, and
// dependency-validated since Phase 2 but never walked until now.
//
// Ordering contract (reproducibility matters for compiled/interpreted
// manifest parity): output is topological — every dependency appears
// before anything that depends on it (post-order DFS). Roots are visited
// in authorized-call (source) order, preserving program intent; edges out
// of each node are visited in ALPHABETICAL order by function name. The
// alphabetical edge tiebreak is the documented deterministic rule: the
// registry's storage order is an index detail the resolver must not
// depend on (same lesson as Phase 2's sorted-traversal fix).
//
// Failure modes (both throw ShakeError, never silent): a dependency cycle
// reports the FULL cycle path (`a -> b -> c -> a`), not just the repeated
// name; a `depends_on` reference naming a function absent from the
// registry errors clearly. The latter is defensive — the Phase 2 scanner
// rejects unresolvable `depends_on` at scan time — but the resolver must
// not silently skip a reference it cannot find. Likewise, shaking a
// non-allowed gate is refused outright (fail-closed): there is no
// meaningful closure over denied calls.

#include <algorithm>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "jocky/policy/capability_gate.hpp"
#include "jocky/stdlib/script_metadata.hpp"

namespace jocky {

// Thrown on cycle, unresolvable reference, denied gate, or unknown root.
// Registry-level errors have no single source token, so line/col are 0
// (same convention as BindingError); the CLI prints
// `error: <file>: <message> (shake)`.
class ShakeError : public std::runtime_error {
public:
    int line;
    int col;
    ShakeError(int line, int col, const std::string& message)
        : std::runtime_error(message), line(line), col(col) {}
};

struct ResolvedScript {
    ScriptMetadata metadata;
    // Why this script is in the closure: "direct call" for roots, or
    // "transitive via <parent function>" for dependencies (the parent
    // through which it was FIRST discovered; diamonds keep first path).
    std::string reason;
};

struct ShakeResult {
    // Deduplicated (diamonds appear exactly once), topological
    // (dependencies before dependents), deterministically ordered.
    std::vector<ResolvedScript> scripts;
};

namespace detail {

enum class VisitState { Unvisited, Visiting, Done };

inline const ScriptMetadata* find_script(
    const std::map<std::string, const ScriptMetadata*>& index,
    const std::string& function) {
    auto found = index.find(function);
    return (found == index.end()) ? nullptr : found->second;
}

inline void visit_script(const std::string& function,
                         const std::string& reason,
                         const std::map<std::string, const ScriptMetadata*>& index,
                         std::map<std::string, VisitState>& states,
                         std::vector<std::string>& stack,
                         ShakeResult& out) {
    VisitState& state = states[function];  // default-constructs Unvisited
    if (state == VisitState::Done) {
        return;  // diamond/shared dep: already emitted exactly once
    }
    if (state == VisitState::Visiting) {
        // Cycle: report the full loop from first occurrence, closed.
        std::string path;
        bool inside = false;
        for (const std::string& frame : stack) {
            if (frame == function) {
                inside = true;
            }
            if (inside) {
                if (!path.empty()) {
                    path += " -> ";
                }
                path += frame;
            }
        }
        path += " -> " + function;
        throw ShakeError(0, 0,
                         "dependency cycle detected: " + path +
                             " (tree-shaking cannot order a cycle)");
    }
    const ScriptMetadata* meta = find_script(index, function);
    if (meta == nullptr) {
        // Defensive: the scanner rejects these at scan time, so reaching
        // here means the index changed under us — fail loudly, never skip.
        const std::string parent =
            stack.empty() ? "<program>" : stack.back();
        throw ShakeError(0, 0,
                         "unresolvable depends_on '" + function +
                             "' required by '" + parent +
                             "' (no registry entry; rebuild the index)");
    }
    state = VisitState::Visiting;
    stack.push_back(function);
    // Alphabetical edge order: the documented deterministic tiebreak.
    std::vector<std::string> deps = meta->depends_on;
    std::sort(deps.begin(), deps.end());
    for (const std::string& dep : deps) {
        visit_script(dep, "transitive via " + function, index, states,
                     stack, out);
    }
    stack.pop_back();
    state = VisitState::Done;
    ResolvedScript entry;
    entry.metadata = *meta;
    entry.reason = reason;
    out.scripts.push_back(std::move(entry));
}

}  // namespace detail

// Resolve the used-script closure over an ALLOWED gate's authorized calls.
// Never returns a partial result: any failure throws ShakeError.
inline ShakeResult resolve_scripts(
    const GateResult& gate, const std::vector<ScriptMetadata>& registry) {
    if (!gate.allowed) {
        throw ShakeError(0, 0,
                         "cannot resolve scripts for a denied gate under "
                         "case '" +
                             gate.case_name +
                             "' (shake only runs on allowed calls)");
    }
    std::map<std::string, const ScriptMetadata*> index;
    for (const ScriptMetadata& meta : registry) {
        index[meta.function] = &meta;
    }
    ShakeResult out;
    std::map<std::string, detail::VisitState> states;
    std::vector<std::string> stack;
    // Roots in authorized (source) order — program intent first.
    for (const ResolvedCall& call : gate.authorized) {
        if (detail::find_script(index, call.function) == nullptr) {
            throw ShakeError(call.line, call.col,
                             "authorized call '" + call.function +
                                 "' has no registry entry (index changed "
                                 "after resolution; rebuild and re-run)");
        }
        detail::visit_script(call.function, "direct call", index, states,
                             stack, out);
    }
    return out;
}

}  // namespace jocky
