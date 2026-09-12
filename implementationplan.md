# Implementation Plan — Phase 6: Script Embedding + jockyc Binary Generation (Current Phase)

> Per `AGENTS.md`, this file describes ONLY the current phase. It is not a
> copy of `roadmap.md`.
> Prior phases: Phase 5 complete — `fir/script_resolver.hpp` closes the
> used-script set over `GateResult::authorized` only (`jocky shake`,
> 5/5 fixtures on a throwaway registry; full record:
> `wiki/16-phase5-tree-shaking.md`); then Phase 5.5 control-flow
> extension complete — `if`/`else`, C-shape `for` with bound-checked
> bounds, `while` flagged `requires_runtime_ceiling`; shared
> `ast/walker.hpp` recursion adopted by the resolver (binder/gate/shaker
> provably needed no changes); `check` AST, 6/6 `tests/phase3/`, 6/6
> `tests/phase4/`, 5/5 `tests/phase5/`, and 7/0/42 registry counts
> unchanged; 6/6 `tests/phase5.5/` green; see `logs.md` and
> `wiki/17-control-flow.md`. This file will be rewritten to
> describe Phase 7 once Phase 6 is marked complete in logs.md.

## Phase: 6 — Script Embedding + jockyc Binary Generation

## What Is Being Built

Phase 5 proves exactly what must be *shipped*; Phase 6 *ships* it: the
`jockyc` ahead-of-time path embeds the tree-shaken script closure into a
compiled artifact and links a standalone binary that no longer needs the
filesystem registry at run time.

1. **Embedding input**: consume Phase 5's deduplicated, topologically
   ordered `ScriptMetadata` list directly — one embedded unit per entry,
   in that exact order. Do NOT re-implement deduplication or cycle
   detection (solved once in `resolve_scripts`); DO hash every listed
   `script_path` at embed time and refuse on mismatch (the resolver
   guarantees set/order, not file freshness — see `wiki/16` §6).
   Control-flow programs need no special handling here: Phase 5.5's
   shared `ast/walker.hpp` already folds `if`/`for`/`while` bodies into
   the same resolved-call stream the shaker consumes, so the closure
   arriving at the embedder is complete by construction (both branches
   included — the documented conservative union, `wiki/17-control-flow.md`
   §5).
2. **`jockyc <file.jky> [-o output]`**: full chain (check → resolve →
   bind → gate → shake → embed → link) producing a runnable standalone
   binary with embedded scripts verifiable by hash. Default output
   `./a.out` per `wiki/06` §1.
3. **Scope**: Linux/WSL only. The stdlib is bash, the verified toolchain
   is Debian g++ on Kali, and parity scope stays same-OS (per `wiki/08`
   R5). No Windows target this phase; the cross-platform compiler stays
   a PS-level adopted requirement (`wiki/08` D2), out of scope here.
   (Note: no explicit Linux/WSL-only decision was found in the wiki when
   this plan was written — searched; the closest records are D2 and R5
   above. This plan records the scope as carried forward from the
   session brief; Phase 6 may cite the original design doc if located.)

## Why

Without embedding, every run depends on a live filesystem registry —
unacceptable for a verifiable artifact (the registry can change under
the binary). Embedding exactly the shaken closure keeps the artifact
minimal (audit surface) and self-contained (no registry at run time),
with embedded-script hashes making the contents verifiable.

## Definition of Done

- [ ] `jockyc <file.jky> [-o output]` produces a runnable standalone
      binary with exactly the shaken closure embedded (hash-verifiable).
- [ ] Embed-time hashing of every `script_path`; mismatch refuses the
      build (exit non-zero).
- [ ] Denied gates never reach embedding (fail-closed chain: shake
      refuses first).
- [ ] Linux/WSL scope documented (no Windows target claimed or tested).
- [ ] Tests green: embedded set matches `jocky shake` listing for the
      same program; binary runs without the filesystem registry.
- [ ] `logs.md` records Phase 6 as complete, at which point this file
      (implementationplan.md) is rewritten to describe Phase 7.
