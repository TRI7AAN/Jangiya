# Implementation Plan — Phase 4: Capability Gate Enforcement (Current Phase)

> Per `AGENTS.md`, this file describes ONLY the current phase. It is not a
> copy of `roadmap.md`.
> Prior phase: Phase 3 complete — `include/jocky/semantic/call_resolver.hpp`
> resolves every `call` against the registry index with arity checking
> (`= default` inputs stored on `InputParam` and filled at call sites),
> parse-level input-type validation, and return-type plumbing into
> `let`-binding types; `jocky resolve` runs it with `file:line:col`
> diagnostics; 6/6 fixtures behave as specified on the real 7-function
> registry with `timeline/` + `report/` legitimately empty; `check` AST
> and 7/0/42 registry counts unchanged; see `logs.md` and
> `wiki/12-phase3-resolution.md`. This file will be rewritten to
> describe Phase 5 once Phase 4 is marked complete in logs.md.

## Phase: 4 — Case-Level Authorization + Capability Gate (Static Check)

## What Is Being Built

Phase 3 proves a call is *well-formed*; Phase 4 proves it is *allowed*.
This stays a separate phase from resolution, not merged back in: the
resolver already records each call's declared capability
(`ResolvedCall::capability`) but enforces nothing against it.

1. **Case association**: every `investigate <name>` block binds to its
   `case <name>` block (matched by name). An investigation with no
   matching case block is a hard error — an investigation without a
   declared authorization scope must not resolve, let alone run.
2. **Gate check** (static half of AGENTS.md §2.2): each resolved call's
   declared capability must be a member of its case's
   `allowed_capabilities` list. Denials are hard errors with
   `file:line:col` diagnostics naming the capability, the function, and
   the case — e.g. a `compliance.tls.scan` call inside a case that only
   allows `netforensics.pcap.read` refuses before anything executes.
   The check runs wherever resolution runs (`jocky resolve`; decide in
   the session whether the gate lives inside `resolve_program` or as a
   separate pass over `ResolvedProgram` — decide explicitly, no silent
   choice).
3. **Runtime hook point (documented, not built)**: the dynamic half of
   the gate (refuse + manifest-log the denial at dispatch) lands in
   Phase 7; Phase 4 records the exact handoff (what the checker
   guarantees, what the dispatcher must re-verify) in the wiki so the
   static check is never mistaken for enforcement at run time.
4. **Tests**: allowed calls pass; denied capabilities fail with the
   denial naming capability + function + case; investigation without a
   case fails; fixture on the real 7-function registry (whose declared
   capabilities span `netforensics.*` and `compliance.tls.scan`, so both
   outcomes are exercisable without mocks alone).

## Why

A well-typed call to a real function is still unauthorized execution if
its case never granted the capability. Without the gate, `check`
waves through investigations that the safety contract (AGENTS.md §2.2,
"refuse to run and log the denial") forbids — the exact gap Phase 5
tree-shaking and Phase 7 dispatch both assume is closed.

## Definition of Done

- [ ] Investigate-to-case binding enforced; missing case is an error
      with `file:line:col` diagnostics; exit non-zero.
- [ ] Capability denials carry `file:line:col` diagnostics naming
      capability, function, and case; exit non-zero.
- [ ] Resolver-vs-gate layering decided explicitly and documented.
- [ ] Phase 7 handoff (static guarantee vs runtime re-verification)
      written in the wiki.
- [ ] Tests green on the real registry (both allow and deny outcomes).
- [ ] `logs.md` records Phase 4 as complete, at which point this file
      (implementationplan.md) is rewritten to describe Phase 5.
