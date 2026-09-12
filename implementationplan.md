# Implementation Plan — Phase 5: Dependency-Aware Tree-Shaking Resolver (Current Phase)

> Per `AGENTS.md`, this file describes ONLY the current phase. It is not a
> copy of `roadmap.md`.
> Prior phase: Phase 4 complete — `include/jocky/semantic/case_binder.hpp`
> binds every program to exactly one case (zero/duplicate/missing-field
> are `BindingError`s), `include/jocky/policy/capability_gate.hpp`
> fail-closed gates every resolved call against `CaseDecl::capabilities`
> (all denials collected, `GateResult::Denied` carries function +
> capability + case + line:col), `jocky gate` runs resolve → bind → gate
> with `ALLOWED`/`DENIED` verdicts; 6/6 `tests/phase4/` fixtures behave as
> specified on the real 7-function registry (binder refusals vs gate
> denials proven distinct code paths); `check` AST, 6/6 `tests/phase3/`
> outcomes, and 7/0/42 registry counts unchanged; see `logs.md` and
> `wiki/13-phase4-capability-gate.md`. This file will be rewritten to
> describe Phase 6 once Phase 5 is marked complete in logs.md.

## Phase: 5 — Dependency-Aware Tree-Shaking Resolver

## What Is Being Built

Phase 4 proves every call is *allowed*; Phase 5 computes exactly what must
be *shipped* for those allowed calls — nothing more. The tree-shaker
operates on `GateResult::authorized` (the gate's allowed-call list), so
denied calls can never pull scripts into a build: only calls that passed
binding + gating participate in the closure.

1. **Used-function closure**: for a given `.jky` file, resolve the exact
   set of stdlib scripts required — directly called functions (from the
   authorized list) plus transitive `depends_on` (already parsed, stored,
   and dependency-validated by the Phase 2 scanner, but never yet
   walked). Walk the graph to fixpoint; report the closure in a stable,
   deterministic order (matching the scanner's reproducibility rule —
   compiled/interpreted manifest parity depends on it).
2. **`--list-used` output**: a resolver mode (likely `jocky gate
   --list-used` or a adjacent CLI spelling — decide explicitly in the
   session, no silent choice) printing the used-function list with
   script paths, so compilation input is inspectable before anything is
   embedded.
3. **Tests**: direct-only closure, transitive closure (e.g. a program
   calling only `jky_netforensics_top_talkers` must pull
   `jky_netforensics_extract_flows` via `depends_on`), and diamond
   dependencies (two used functions sharing one dependency list it once)
   — all on the real 7-function registry, whose `depends_on` edges
   (`check_dns_anomalies`/`detect_beaconing`/`top_talkers` →
   extractors) exercise transitive + diamond shapes without mocks.
4. **Handoff note (documented, not built)**: Phase 6 embedding consumes
   exactly this closure; record what the resolver guarantees (closed,
   deduplicated, ordered set over authorized calls only) and what the
   embedder must verify (hashes of the listed script paths at embed
   time) in the wiki.

## Why

`jockyc` must embed the minimum necessary script set: embedding the whole
registry bloats the artifact and widens the audit surface, while embedding
too little breaks the binary. The closure must additionally be gated —
tree-shaking over raw resolved calls instead of the authorized list would
ship scripts for denied capabilities. The depends_on graph exists in the
index but no code walks it yet; that is the exact gap this phase closes.

## Definition of Done

- [ ] Closure computed over `GateResult::authorized` only (denied calls
      contribute nothing); transitive `depends_on` walked to fixpoint,
      deduplicated, deterministically ordered.
- [ ] `--list-used` (or explicitly-decided equivalent spelling) prints
      the used-function list with script paths; exit non-zero on
      bind/gate failure before any listing.
- [ ] CLI spelling decision documented explicitly (no silent choice).
- [ ] Phase 6 handoff (resolver guarantee vs embedder re-verification)
      written in the wiki.
- [ ] Tests green on the real registry (direct, transitive, diamond).
- [ ] `logs.md` records Phase 5 as complete, at which point this file
      (implementationplan.md) is rewritten to describe Phase 6.
