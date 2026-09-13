# Phase 8 Record: `jocky` Interpreter + Compiled/Interpreted Parity

> Completed: 2026-09-13. Platform scope: Linux/WSL (unchanged).
> Build: clean CMake Release, zero warnings/errors (g++ 15.2.0);
> `ctest` 10/10 (4 pre-existing + 6 new parity tests). All run
> artifacts lived in `/tmp`; digests and verdicts are persisted HERE.

## 1. Pre-build verification: ceiling logic is shared, not codegen-baked

The pre-Phase-8 checkpoint asked to verify this FIRST, before building
the interpreter — finding: **no refactor was needed.** Ceiling
ENFORCEMENT lives entirely in shared headers: the while-ceiling check
and `max_executions` budget check in
`include/jocky/runtime/control_flow_executor.hpp:146,271-274`, with
defaults in `include/jocky/runtime/dispatcher.hpp:116,121`
(`max_executions = 1000`, `max_while_iterations = 10000`, one struct,
one source). What is baked into codegen is only standalone argv
PARSING (`src/compiler/main.cpp:271-276`, emitted into each generated
binary) and plan-field emission. The interpreter therefore parses the
same flag names into the same `RuntimeOptions` fields — both paths call
the identical `execute_plan_tree` / `dispatch_single_call`. Default
equality is structural (single shared struct), not assumed: every
parity run below passes NO ceiling flags on either side, so any default
drift would surface as a count/outcome mismatch.

## 2. What was built

- `jocky <file.jky> [--registry <dir>] [--output-root <dir>]
  [--manifest <path>] [--max-executions <n>] [--max-iterations <n>]`
  (`src/cli/main.cpp`, `run_execute`): full pipeline lex → parse →
  resolve → bound-check → bind → gate → execute. Deliberately NO shake
  step — shaking decides what to EMBED, and with nothing embedded it is
  meaningless; the gate already limits execution to authorized calls and
  every attempt is still capability-rechecked at dispatch. This omission
  is design, not oversight.
- The ONLY structural difference from a compiled binary is call
  sourcing: each authorized `ResolvedCall` is materialized through the
  existing `load_registry_runtime_call` (bytes read from disk with the
  scanner digest preserved, so drift becomes a manifest-logged
  `integrity_denied`), with `line`/`col` copied across so the executor's
  call-site map resolves identically. Plan construction (case,
  capabilities, evidence, declared outputs via the same walker order,
  case ceiling, source text) mirrors the compiler's `build_plan`
  field-for-field.
- Sandbox children re-exec the `jocky` binary itself
  (`--jocky-sandbox-child` branch at the top of `main`, mirroring the
  generated runner); `executable_path` defaults to `self_executable()`,
  same as compiled.
- Output contract mirrors compiled `run`: `MANIFEST <path>
  status=<status>` plus a manifest-grounded summary,
  `EXECUTED <E> calls (<fn> x<n>, ...) [, N loop(s) capped],
  status=<status>` — E counts dispatched attempts (each loop trip
  dispatches independently, so trips are included), capped loops are
  called out from `while_ceiling` entries. Exit code mirrors run
  status. Denied gates print the `jocky gate` verdict, exit 1; all other
  failures use `file:line:col` diagnostics, exit 1.

## 3. Parity comparison rules (locked)

Implemented in `tests/phase8/parity_diff.py`, driven per fixture by
`tests/phase8/run_parity.sh` (build → run compiled with explicit
`--manifest` → run interpreted with explicit `--manifest` → diff;
`$?` captured immediately after every step; exit codes compared too).
Compared EXACTLY: every execution's function, capability,
script_sha256, args, exit_code, outcome, timed_out, stdout,
stdout_sha256, stderr; total count AND order of executions; top-level
case_id, program_sha256, status, authorization, inputs, outputs,
errors, manifest_version, runtime_version. EXCLUDED (timing only):
run_start_utc, run_end_utc, per-entry start_utc, end_utc,
duration_ms. script_sha256 equality is a SEPARATE explicit assertion
(index-wise, plus 64-hex well-formedness on both sides for every real
script entry) — the concrete proof that tree-shaking embeds an
unmodified copy. The differ was negative-controlled: a one-field
tamper (`exit_code` 0→7) yields `PARITY FAIL` with an 11-line unified
diff naming the field.

## 4. Fixture runs (all six green; what "match" looks like)

All six use the hermetic `tests/phase7.5/registry` (3/0/0;
`true_branch 507a50de…`, `false_branch` poison exit 7 `87a9a140…`,
`emit_lines 99bff8cf…` — hashes re-verified live this session), so
parity measures control-flow semantics, never evidence or network.
Fixture 6 replays the Phase 5.5 fixture-6 if→for→if structure on that
registry (bound 3, as in the original) for the same reason.

| Fixture | Compiled / interpreted executions | Verdict |
| ------- | --------------------------------- | ------- |
| 1 `direct.jky` | `true_branch/success` ×1 both | PARITY PASS, 1 execution |
| 2 `if_data.jky` (`contains "two"` from real output) | `emit_lines/success, true_branch/success` both; poison 0/0/0/0 by grep | PARITY PASS, 2 executions |
| 3 `for_literal.jky` (bound 4) | `true_branch/success` ×4 both, in order | PARITY PASS, 4 executions |
| 4 `while_natural.jky` (data-false cond) | `emit_lines/success` ×1 both; `while_ceiling` 0/0 by grep | PARITY PASS, 1 execution |
| 5 `while_capped.jky` (ceiling 4) | `true_branch/success` ×4 + identical `<while-loop>/while_ceiling` both; exits 1/1 | PARITY PASS, 5 executions |
| 6 `nested.jky` (if→for(3)→if) | `true_branch/success, emit_lines/success` ×3 both; poison 0/0 by grep | PARITY PASS, 4 executions |

Concrete match shape (fixture 1, interpreted side; the compiled twin
differs only in timing fields):

```json
{"manifest_version":"0.2.0","case_id":"t","program_sha256":"85552fe8ef7157fd52f8302c83feeb9dc91a3345eef92eeb57c8ba0706aac7b1","runtime_version":"jocky 0.1.0","run_start_utc":"2026-09-13T07:46:34Z","run_end_utc":"2026-09-13T07:46:34Z","status":"success","authorization":{"case":"t","allowed_capabilities":["recon.fixture.execute"]},"inputs":[],"outputs":[],"executions":[{"function":"jky_recon_true_branch","capability":"recon.fixture.execute","script_sha256":"507a50de2b117817f001456fa66e7708fe1e858bf97764ec57a6d45c63ffcd3c","args":{"target":"base"},"outcome":"success","exit_code":0,"timed_out":false,"duration_ms":4,"start_utc":"2026-09-13T07:46:34Z","end_utc":"2026-09-13T07:46:34Z","stdout":"branch-true\n","stdout_sha256":"953d00a64029233b6172e8dac5a65ef20187a03e4622679da7b931d43b74da27","stderr":""}],"errors":[]}
```

`program_sha256` equals `sha256sum tests/phase8/direct.jky`
(`85552fe8…`, verified live). Build-time `registry_version` for the
single-script closure: `7568266d…`, identical from `--registry-version`.
Capped marker (fixture 5, either side):

```json
{"function":"<while-loop>","capability":"","script_sha256":"","args":{},"outcome":"while_ceiling","exit_code":-1,"timed_out":false,"duration_ms":0,"start_utc":"2026-09-13T07:47:16Z","end_utc":"2026-09-13T07:47:16Z","stdout":"","stdout_sha256":"e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855","stderr":"while loop iteration ceiling (4) reached; loop aborted after 4 iterations"}
```

## 5. Regression evidence (same build)

- `jocky check samples/sample.jky`: byte-identical to `wiki/11` §1
  (frozen printer untouched — interpreter added no AST output).
- `scan_registry stat_scripts/`: 7/0/42, exit 0.
- Phase 3 `resolve`: `0,0,1,1,1,1`. Phase 4 `gate`: `1,1,1,1,0,1`.
  Phase 5 `shake`: `0,0,0,1,0`. Phase 5.5 `gate`: `0,0,0,1,0,0`.
- Phase 7.5 compiled runs: `0,0,0,1,0,0,0` (capped fixture exits 1).
- `ctest`: 10/10 (4 pre-existing + 6 `phase8_parity_*`).

## 6. Definition-of-Done mapping (brief PARTs)

- PART A: `jocky <file.jky>` with the specified flags (plus
  `--output-root`/`--manifest`, mirroring standalone `run`, so each
  side writes its own manifest); shared-struct defaults verified, not
  assumed; denied-gate/bad-flag/missing-file paths all exit 1 in
  existing diagnostic formats; sandbox-child delegation proven by
  every interpreted sandbox spawn in §4.
- PART B: bash driver + `parity_diff.py` (documented choice: JSON
  differencing without a C++ JSON dependency, following the existing
  `run_phase6/7.sh` + `BASH_PROGRAM` pattern); six `ctest` entries.
- PART C: 6/6 fixtures green with concrete per-entry projections
  above; poison absence grepped 0/0/0/0; natural-exit shows no
  `while_ceiling` entry 0/0 on either side.
