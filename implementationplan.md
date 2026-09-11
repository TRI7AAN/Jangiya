# Implementation Plan — Phase 3: `call` Expression Resolution (Current Phase)

> Per `AGENTS.md`, this file describes ONLY the current phase. It is not a
> copy of `roadmap.md`.
> Prior phase: Phase 2 complete — `_quarantine/` holds the 3 unconfirmed
> root files, `scan_registry` indexes annotated scripts (7 registered /
> 0 rejected on the real tree) with deterministic output, and the
> registry corpus stands at 48 scripts with `timeline/` + `report/`
> legitimately empty; see `logs.md`. This file will be rewritten to
> describe Phase 4 once Phase 3 is marked complete in logs.md.

## Phase: 3 — `call` Expression Grammar Extension (Resolution & Checking)

## What Is Being Built

The Phase 1 parser already produces `call` AST nodes with named
arguments; Phase 3 gives them meaning against the Phase 2 registry:

1. **Call resolver** (new semantic unit over the AST + registry index):
   every `call f(...)` resolves `f` against `scan_registry` output.
   Unknown functions are hard errors with `file:line:col` diagnostics
   in the lexer's style. Depends on the registry index format — read,
   never reimplement, the scanner's JSON.
2. **Arity checking**: call args matched against `@jocky:inputs` by
   name; missing required args and unrecognized args are errors.
   `= default` inputs (recorded in headers, types stored default-free
   by the scanner) may be omitted — the resolver must re-read raw
   header defaults or treat all declared inputs as required and
   document which; decide explicitly, no silent choice.
3. **Input-type validation (parse-level)**: literal arg kinds
   (string/int/float/bool/list, field refs) checked against declared
   input types; mismatches rejected pre-execution. This is the static
   half of AGENTS.md §2.5 (runtime re-validation lands in Phase 7).
4. **Return-type plumbing**: the declared output type
   (`table<flow>`, `text`, …) becomes the type of the bound table for
   downstream pipeline operators; unknown output types are errors.
5. **Tests**: calls resolving to registry entries, unknown functions,
   bad arity, mistyped args — each with clean diagnostics. Fixture on
   the real 7-function registry, not mocks alone.

## Why

Parsing without resolution lets a `.jky` file name functions that do
not exist with arguments nothing accepts — a broken investigation that
`check` currently waves through. Closing that gap turns the registry
from an index into a contract, which Phases 4 (capability gate) and 5
(tree-shaking) both build on.

## Definition of Done

- [ ] Resolver consumes `scan_registry` JSON; no duplicated parsing.
- [ ] Unknown-function, arity, and type errors all carry
      `file:line:col` diagnostics; exit non-zero.
- [ ] `= default` input handling decided explicitly and documented.
- [ ] Tests green on the real registry (timeline/report emptiness must
      not break resolution of the other four domains).
- [ ] `logs.md` records Phase 3 as complete, at which point this file
      (implementationplan.md) is rewritten to describe Phase 4.
