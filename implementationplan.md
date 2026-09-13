# Implementation Plan — Phase 9: Real Registry Finalization (Current Phase)

> Per `AGENTS.md`, this file describes ONLY the current phase. It is not a
> copy of `roadmap.md`.
> Phase 8 is complete: `jocky <file.jky>` interprets directly against the
> filesystem registry through the shared Phase 7.5 tree executor (the only
> structural difference from compiled runs is call sourcing), and the
> parity harness proves both paths agree exactly minus timing fields on
> all six control-flow fixtures. Full record:
> `wiki/22-phase8-interpreter-parity.md`.

## Phase: 9 — Real Registry Finalization

## What Is Being Built

1. `@jocky:` headers across the ~48-script corpus: close the 41 header
   gaps (7 present today), keeping `scan_registry` at zero rejections.
2. Timeline/report domain starter scripts (both domains are empty in the
   registry today), following the `jky_<domain>_<verb>_<object>`
   convention with full headers.
3. End-to-end smoke-test harness proving both execution paths are
   provably equivalent on REAL evidence: compile + interpret the same
   investigation, diff manifests with the Phase 8 parity rules, against
   sample pcap/eventlog/directory evidence.

## Scope

Phase 9 can now build with full confidence that both execution paths
are provably equivalent (Phase 8 parity green on straight-line,
branching, looping, capped, and nested programs). Linux/WSL remains
the execution target. Judge-facing rationale (Phase 10) and the final
gap-check audit (Phase 11) stay out of scope until the registry is real.

## Definition of Done

- [ ] Every `stat_scripts/` script carries a valid `@jocky:` header.
- [ ] Timeline and report domains each have at least one working starter
      script with a valid header.
- [ ] Smoke harness runs check → compile + interpret → parity diff over
      sample evidence, green on both paths.
- [ ] Phase 1–8 regression remains green.
