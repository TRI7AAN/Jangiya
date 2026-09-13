# Implementation Plan — Phase 10: Judge-Facing Design Rationale (Current Phase)

> Per `AGENTS.md`, this file describes ONLY the current phase. It is not a
> copy of `roadmap.md`.
> Phase 9 is complete: 26 of 41 headerless scripts annotated from full
> source reads (15 honestly flagged for human review and left
> headerless), 5 pure-bash timeline/report starters written and verified,
> registry at 38 registered / 0 rejected / 16 skipped, and every
> registered function smoke-invoked through the dispatcher (8 pass,
> 30 environment-limited failures, all manifest-logged, none omitted).
> Full record: `wiki/23-phase9-registry-finalization.md`.

## Phase: 10 — Judge-Facing Design Rationale Document

## What Is Being Built

A single self-contained document a SIH judge reads: for each clause of
the literal SIH26148 problem-statement text, what JOCKY does instead
and why — showing how the legitimate underlying need (authorized,
low-footprint, fully auditable forensic analysis) is met while the
evasion/weaponization components (polymorphic engine, BYOVD, process
hollowing, domain-fronted C2) are permanently out of scope with
explicit justification. Evidence pointers throughout: registry
(`wiki/23` annotation + smoke tables), pipeline (`wiki/02`,
`wiki/13`), manifests (`wiki/20`, `wiki/21`, `wiki/22`), and the
permanent product boundary (`wiki/01-prd.md`, AGENTS.md §2).

## Scope

Docs only — no compiler/runtime/registry changes unless the writing
exposes a factual gap in an existing wiki record (fix the record,
log it, do not expand scope). Linux/WSL remains the execution target.
The final gap-check audit (Phase 11) stays out of scope until the
rationale document exists to check against.

## Definition of Done

- [ ] One document mapping each literal-PS clause to JOCKY's
      reframed behavior with evidence pointers.
- [ ] Every evasion/weaponization exclusion justified against a
      legitimate underlying need it still satisfies.
- [ ] Passes a hostile read against the literal PS text.
- [ ] Phase 1–9 regression remains green.
