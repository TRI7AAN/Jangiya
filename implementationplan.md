# Implementation Plan — Track A Complete (No Active Phase)

> Per `AGENTS.md`, this file describes ONLY the current phase. There is
> no current phase: **Track A (Phases 0–11) closed 2026-09-14** with the
> Phase 11 final coverage audit (`wiki/25-final-ps-coverage.md`), signed
> off in `logs.md`. `roadmap.md` carries the closure marker.
> Phase 10 delivered the judge-facing rationale (`wiki/24`); Phase 11
> re-verified the full build from scratch (zero deviations), resolved
> the four open items (`jocky verify` → documented backlog;
> E3j → specified-but-unwritten backlog; pipeline operators → proven
> parse-level; SIGABRT → reproduced, low severity), and issued the final
> requirement verdict with a post-Track-A corrective backlog (`wiki/25`
> §6).

## What Happens Next (Not a Phase)

- Any future Track A work is **corrective and backlog-driven** (items in
  `wiki/25` §6: `jocky verify`, Windows build, registry honesty/growth,
  manifest-path preflight, pipeline-operator spec honesty) — not
  roadmap-driven. Open a session against a specific backlog item, not a
  phase number.
- **Track B (Phases 12–16, Java IDE shell) is the active track** going
  forward if development continues. It invokes `jocky`/`jockyc` as
  subprocesses; all `AGENTS.md` §2 rules still apply.

## Standing Orders (Unchanged)

- Linux/WSL remains the execution target until the Windows backlog item
  says otherwise.
- Every session: read `AGENTS.md` in full, append to `logs.md`, persist
  computed hashes/counts into a `wiki/` file at computation time.
- `jocky verify` is UNIMPLEMENTED (backlog `wiki/25` §6.1) — do not claim
  otherwise; use the `wiki/24` §5.4 manual procedure.
