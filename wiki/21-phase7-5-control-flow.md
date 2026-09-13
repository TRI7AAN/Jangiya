# Phase 7.5 Record: Execution-Time Control-Flow Interpreter + Manifest Corrections

> Corrective phase inserted before Phase 8 (see roadmap.md note).
> Root cause (audit GAP-1/GAP-2, `wiki/18-phase6-7-audit.md` §2.3): the
> Phase 7 dispatcher executed the flattened authorized-call list with no
> regard for IfStmt/ForStmt/WhileStmt structure — both branches ran,
> loops ran once, and no iteration ceiling existed anywhere despite the
> `wiki/17` §3 MUST. Parity testing on top of that would have compared
> two equally broken paths, so this phase lands first.
>
> Completed: 2026-09-13. Platform scope: Linux/WSL (unchanged).
> Build: `cmake -S . -B /tmp/jocky-75`, `cmake --build` — clean, zero
> warnings/errors (g++ 15.2.0, cmake 4.3.4). All probe artifacts lived in
> `/tmp/75run`; digests and verdicts are persisted HERE.

## 1. Why this was needed (citing the audit)

- `grep WhileStmt|requires_runtime_ceiling|iteration_cap|max_iter` over
  `include/jocky/runtime/` + `src/compiler/` returned zero matches: the
  flag was set by the parser and read by nothing.
- `while (r == r)` over an `exit 0` script finished in 0.009s with one
  entry: no iteration, no cap — the run "succeeded" for the wrong reason.
- An `if/else` fixture executed BOTH authorized calls although only one
  branch could run: untaken-branch calls EXECUTED.

## 2. What was built

- `include/jocky/runtime/predicate_evaluator.hpp` (new): execution-time
  `evaluate_predicate` over literals, lists, and recorded call outputs.
  Field references resolve only to `let`-bound call stdout; anything
  unknowable (unbound names, non-call pipelines, source/correlate,
  mixed-kind comparisons) throws `PredicateError` — never a silent
  default. Static passes never include this header (opposite direction
  from the walker, by design).
- `include/jocky/runtime/control_flow_executor.hpp` (new): walks the
  real statement tree (rules then investigations, mirroring
  `resolve_program`). If takes one branch (other leaves no trace); for
  loops `bound - start` times with independent per-iteration entries;
  while re-evaluates per iteration under a hard ceiling. Every attempt
  goes through the extracted `dispatch_single_call` — the SAME
  capability/integrity/validation/sandbox/timeout/manifest path as flat
  dispatch, so the two modes cannot drift on per-attempt properties.
- `max_while_iterations`: case-level property (`case t { ...
  max_while_iterations: 5; }`, duplicate/negative rejected at parse),
  carried as `RuntimePlan::max_while_iterations` (0 = unset) with
  fallback to `RuntimeOptions::max_while_iterations` (default **10000**,
  also settable per run via `standalone run --max-iterations N`). The
  default bounds the worst case while case files opt into tight,
  auditable ceilings; the shared `max_executions` budget remains the
  outer bound (loops abort with an error instead of emitting unbounded
  `ceiling_denied` entries — flat runs keep the finite legacy behavior).
- Ceiling breach records `outcome: "while_ceiling"` on a `<while-loop>`
  marker entry and fails the run — distinct and visible, never silent
  truncation.
- Manifest schema 0.1.0 → **0.2.0**: top-level `script_sha256` renamed
  to **`program_sha256`** (it always held the .jky hash; per-execution
  `script_sha256` keeps its correct meaning); per-execution
  **`start_utc`/`end_utc`** added (`duration_ms` and raw
  stdout/stderr kept); per-execution **`stdout_sha256`** added.
- `registry_version`: SHA-256 over the ordered embedded closure
  (`function\0sha256\n` per unit), printed by `jockyc` at build
  (`registry_version <hex>`) and queryable via
  `standalone --registry-version` (audit GAP-4 closed).
- Static layer untouched by design: gate still authorizes both branches
  (7.5 fixtures gate ALLOWED exactly as 5.5 did), shake still takes the
  union. `jocky check` output is byte-identical for programs without the
  new case field (verified against `wiki/11` §1).

## 3. Runtime value semantics (locked for Phase 8 to inherit)

- `let name = call ...` records the call's captured stdout under `name`
  (even on failure/denial — whatever was captured).
- Non-call pipeline bindings are name-known only; field references and
  `count()` over them abort fail-closed (tables are not materialized at
  dispatch; Phase 8 changes nothing here unless it evaluates pipelines).
- `count(t)` over a call-produced binding = newline count of its stdout
  (`"one\ntwo\nthree\n"` → 3; empty → 0).
- Integer `let` constants scope exactly like `bound_checker` (root scope
  per body, fresh scopes for branches/loop bodies, loop var never a
  constant).
- Predicate-embedded calls dispatch in walker order before evaluation;
  their outputs are keyed by call-site `line:col`.
- Undecidable conditions, unmappable call sites, and budget exhaustion
  inside loops abort the run with a manifest `errors[]` entry and
  failed status — a branch is never chosen on a guess.

## 4. Fixture runs (verbatim verdicts; `$?` captured immediately)

Registry `tests/phase7.5/registry/` scans **3/0/0** (all
`recon.fixture.execute`): `jky_recon_true_branch` (`exit 0`,
`507a50de...`), `jky_recon_false_branch` (`exit 7` poison,
`87a9a140...`), `jky_recon_emit_lines` (three lines, `99bff8cf...`) —
hashes match the embedded values below byte-for-byte.

| Fixture | Build | Run | Manifest entries | Verdict |
| ------- | ----- | --- | ---------------- | ------- |
| 1 `if_false` | 0, 1 script, `registry_version b383b65c...` | 0, success | **0** (`false_branch` absent by grep) | untaken call never spawned |
| 2 `if_true_else` | 0, 2 scripts, `d3d1d376...` | 0, success | **1** (`true_branch` success; `false_branch` absent) | taken once, else zero |
| 3 `for_three` | 0, 1 script, `7568266d...` | 0, success | **3** (`true_branch` ×3) | not 1, not 0 |
| 4 `while_capped` (ceiling 5) | 0, 1 script, `7568266d...` | 1, failed, **0.041s** | **5** success + **1** `while_ceiling` | capped, logged, fast |
| 5 `nested` (if→for(2)→if) | 0, 3 scripts, `15d258d7...` | 0, success | **3** (probe ×1, deep ×2; poison absent) | taken path only, loop count real |
| extra `for_ident` (N=2) | 0 | 0, success | **2** | ident form executes |
| extra `for_count` (3 lines) | 0 | 0, success | **4** (producer + 3) | count form executes |

Program hashes (`program_sha256`, truncated): if_false `10e2a89e...`,
if_true_else `b62a6678...`, for_three `edf89345...`, while_capped
`70d93ab6...`, nested `620696b8...`, ident `442f4cc6...`, count
`c6b19d34...`.

Cap marker, verbatim shape (fixture 4, last entry):

```json
{"function":"<while-loop>","capability":"","script_sha256":"","args":{},"outcome":"while_ceiling","exit_code":-1,"timed_out":false,"duration_ms":0,"start_utc":"2026-09-13T05:46:28Z","end_utc":"2026-09-13T05:46:28Z","stdout":"","stdout_sha256":"e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855","stderr":"while loop iteration ceiling (5) reached; loop aborted after 5 iterations"}
```

Hash self-check (fixture 3, first entry): stored `stdout_sha256`
`953d00a6...` recomputes exactly over `"branch-true\n"`.
`--registry-version` prints the build-time value (`7568266d...` for the
single-script closure). Gate layering re-confirmed on this build:
fixture 3 zero `DENIED`, fixture 4 exactly one.

## 5. Regression evidence (same build)

- `jocky check samples/sample.jky`: byte-identical to `wiki/11` §1.
- `scan_registry stat_scripts/`: 7/0/42, exit 0.
- Phase 3 `resolve`: `0,0,1,1,1,1`. Phase 4 `gate`: `1,1,1,1,0,1`.
  Phase 5 `shake`: `0,0,0,1,0`. Phase 5.5 `gate`: `0,0,0,1,0,0`.
- `ctest`: 4/4 passed (embedder stale-hash, jockyc integration,
  runtime safety incl. timeout-kill + capability re-check, embedded
  runtime) — the Phase 6/7 audit's non-control-flow checks re-verified
  against the refactored shared dispatch path.
- Parser edges: duplicate `max_while_iterations` → hard error;
  unknown case field still rejected; property renders in `check` output
  only when present.

## 6. Definition-of-Done mapping (brief PARTs)

- PART A: `predicate_evaluator.hpp` (none existed — confirmed by grep);
  fail-closed `PredicateError`, strict comparisons, documented semantics.
- PART B: `control_flow_executor.hpp` (taken-branch-only, real `for`
  trips with independent entries, re-evaluated `while` with hard
  ceiling, default 10000, case/run configurability); shared
  `dispatch_single_call` used by BOTH flat and tree paths; compiler
  embeds source + positions + case ceiling.
- PART C: `program_sha256` rename (source hash kept), per-entry
  `start_utc`/`end_utc` + `stdout_sha256` (raws kept),
  `registry_version` + `--registry-version` flag.
- PART D: 5/5 hostile fixtures green with manifest excerpts above, plus
  2 supplemental bound-form runs.
