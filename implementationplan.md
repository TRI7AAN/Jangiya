# Implementation Plan — Phase 8: Interpreter + Parity (Current Phase)

> Per `AGENTS.md`, this file describes ONLY the current phase. It is not a
> copy of `roadmap.md`.
> Phase 7.5 is complete: the shared dispatcher now walks the real
> statement tree (taken-branch-only `if`, real `for` trips with
> independent per-iteration entries, re-evaluated `while` under a hard
> configurable ceiling with a distinct `while_ceiling` outcome), every
> attempt still flowing through the one shared dispatch path, and the
> manifest is schema 0.2.0 (`program_sha256`, per-entry
> `start/end_utc` + `stdout_sha256`, `registry_version` flag). Hostile
> fixtures green (`tests/phase7.5/`). Full record:
> `wiki/21-phase7-5-control-flow.md`.

## Phase: 8 — `jocky` Interpreter + Compiled/Interpreted Parity Testing

## What Is Being Built

1. `jocky <file.jky>` runs the full static chain and executes directly from
   the filesystem registry through the shared dispatcher.
2. The interpreted path reuses the Phase 7.5 tree executor (same branch
   selection, same loop iteration, same while ceiling, same
   `dispatch_single_call`) — the interpreter differs from the compiled
   path only in call sourcing (filesystem registry via
   `load_registry_runtime_call` vs embedded bytes), never in
   control-flow semantics.
3. Control-flow-correct on both paths: the `tests/phase7.5/` hostile
   fixtures (untaken-branch silence, per-iteration entries, ceiling
   behavior) must produce matching outcomes compiled and interpreted.
4. A parity harness runs both paths against identical evidence and compares
   normalized manifests (schema 0.2.0), excluding only documented
   nondeterministic fields.

## Scope

Linux/WSL remains the execution target. Real-registry bulk annotation and the
full forensic smoke-test corpus remain Phase 9.

## Definition of Done

- [ ] `jocky <file.jky>` executes through `load_registry_runtime_call`.
- [ ] Registry source drift is refused before dispatch and manifest-logged.
- [ ] Compiled and interpreted paths evaluate the same straight-line and
      control-flow fixtures (including all `tests/phase7.5/` hostile cases).
- [ ] Both paths produce matching normalized manifests for identical inputs.
- [ ] Capability denial, timeout, evidence protection, and output promotion
      remain identical across both paths.
- [ ] Phase 1–7.5 regression remains green.
