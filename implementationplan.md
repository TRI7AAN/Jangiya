# Implementation Plan — Phase 8: Interpreter + Parity (Current Phase)

> Per `AGENTS.md`, this file describes ONLY the current phase. It is not a
> copy of `roadmap.md`.
> Phase 7 is complete: the shared dispatcher rechecks capabilities and typed
> argv, executes embedded or registry bytes in an isolated root with bounded
> process groups, protects evidence through snapshots and output staging, and
> atomically records every outcome in the integrity manifest. Full record:
> `wiki/20-phase7-runtime.md`.

## Phase: 8 — `jocky` Interpreter + Compiled/Interpreted Parity Testing

## What Is Being Built

1. `jocky <file.jky>` runs the full static chain and executes directly from
   the filesystem registry through the Phase 7 dispatcher.
2. Compiled and interpreted paths consume the same runtime plan semantics,
   capability recheck, argument validation, isolation, ceilings, and manifest
   writer.
3. Runtime evaluation selects control-flow paths and executes bounded loops
   without exceeding the dispatcher ceiling.
4. A parity harness runs both paths against identical evidence and compares
   normalized manifests, excluding only documented nondeterministic fields.

## Scope

Linux/WSL remains the execution target. Real-registry bulk annotation and the
full forensic smoke-test corpus remain Phase 9.

## Definition of Done

- [ ] `jocky <file.jky>` executes through `load_registry_runtime_call`.
- [ ] Registry source drift is refused before dispatch and manifest-logged.
- [ ] Compiled and interpreted paths evaluate the same straight-line and
      control-flow fixtures.
- [ ] Both paths produce matching normalized manifests for identical inputs.
- [ ] Capability denial, timeout, evidence protection, and output promotion
      remain identical across both paths.
- [ ] Phase 1–7 regression remains green.
