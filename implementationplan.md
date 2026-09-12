# Implementation Plan — Phase 7: Sandboxed Runtime Dispatcher + Evidence Manifest Logging (Current Phase)

> Per `AGENTS.md`, this file describes ONLY the current phase. It is not a
> copy of `roadmap.md`.
> Phase 6 is complete: `jockyc` runs parse → resolve → bound-check → bind →
> gate → shake → embed → link, scan-time script SHA-256 values are rechecked
> at embed time, and the generated Linux artifact contains only the authorized
> dependency closure. Full record: `wiki/19-phase6-embedding.md`.

## Phase: 7 — Sandboxed Runtime Dispatcher + Evidence Manifest Logging

## What Is Being Built

Phase 7 turns the Phase 6 standalone package into an execution system while
preserving every hard rule in `AGENTS.md`.

1. A shared dispatcher for embedded and filesystem-registry scripts that
   passes validated arguments as an argv vector, never through untyped shell
   interpolation.
2. A runtime capability re-check immediately before every dispatch, independent
   of the compile-time gate.
3. Canonical evidence/output path policy: declared evidence remains read-only,
   outputs are restricted to authorized destinations, and aliases/symlink
   escapes are rejected.
4. Timeout and control-flow ceilings, with process-group termination and
   captured stdout/stderr/exit status.
5. An append-complete integrity manifest entry for every attempt: success,
   failure, timeout, or authorization denial. A run is failed if the manifest
   cannot be completed.
6. SHA-256 records for the `.jky` source, evidence inputs, embedded/registry
   script bytes, and declared outputs.

## Scope

Linux/WSL remains the execution target for this phase. The dispatcher must be
usable by the Phase 6 generated artifact and the future interpreted path, but
Phase 8 parity work is not pulled into this phase.

## Definition of Done

- [ ] Capability denial is rechecked and manifest-logged without spawning.
- [ ] Validated arguments reach scripts without command-string interpolation.
- [ ] Evidence-path writes and canonical-path escapes are refused and logged.
- [ ] Success, non-zero exit, timeout, and denial each produce a complete
      manifest execution entry.
- [ ] Every started child is bounded by timeout and process-group cleanup.
- [ ] Manifest input/script/output hashes are reproducible and verifiable.
- [ ] Phase 6 standalone artifacts dispatch from embedded bytes without a live
      registry.
- [ ] Tests cover all hard-rule failure paths and leave no unlogged attempt.
