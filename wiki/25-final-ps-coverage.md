# Phase 11 Record: Final SIH26148 Coverage Audit (Track A Close-Out)

> Completed: 2026-09-14. This session re-verified every prior claim from a
> clean rebuild (PART 1, zero deviations), resolved the four unclear items
> from `wiki/24` §6 (PART 2, with live probes), and issues the final
> requirement-by-requirement verdict below (PART 3). Method: prior sessions'
> results treated as claims, not facts; every number below was reproduced
> this session on build `/tmp/audit-build` (g++ 15.2.0, cmake 4.3.4, zero
> warnings) at commit `2225205`, except where a different commit is cited.
> Build/run artifacts lived in `/tmp` only; the repo tree stayed clean
> throughout (verified via `git status` after every stage).

## 0. Executive summary (90 seconds)

- **What JOCKY provably is:** a typed forensic DSL (`.jky`) with a
  parse→resolve→bound-check→bind→gate→shake→embed→execute pipeline, every
  stage fixture-verified; sandboxed call dispatch with
  capability/hash/type rechecks; complete hash-chained manifests
  (schema 0.2.0); compiled/interpreted parity 6/6; 38-function registry
  with every function dispatcher-invoked (8 pass, 30 environment-limited,
  zero unlogged).
- **The one architectural correction this audit makes:** pipeline query
  operators (`where`/`select`/`group_by`/`having`/`sort_by`/`limit`/
  `correlate`/`write`) are **parse-level only** — they type-check and
  gate, but no relational engine executes them at runtime (proven live,
  §5). JOCKY's proven capability is **capability-gated call dispatch with
  control flow**, not query execution. All PS-coverage claims below are
  cut to fit that fact.
- **Deliberately absent (permanent boundary, not debt):** polymorphic
  engines, cryptors, the five in-memory techniques, direct syscalls for
  evasion, BYOVD/kernel subversion, persistence, priv-esc, SOCKS5, C2,
  domain fronting / cloud-API routing — each with its enforcement pointer
  (§3). No such mechanism exists anywhere in `src/`/`include/`.
- **Genuine gaps (backlog, §6):** `jocky verify` unimplemented (contract
  docs corrected this session); Windows builds; central management;
  E3j driver-audit function + 15 human-review scripts; sandbox tooling
  (or honest scope marking); SIGABRT on manifest-outside-output-root
  (reproduced: exit 134, preflight-only, no corruption — low severity).
- **Verdict:** legitimate forensic requirements are satisfied to the
  extent built (§2); weaponizable requirements are absent by construction
  (§3); nothing in the literal PS text is left unaddressed (§7).
  Track A closes with this document.

## 1. From-scratch regression (PART 1) — verdict: ZERO DEVIATIONS

Clean slate confirmed first (`build/`, `/tmp/p10build`, `/tmp/j`,
`/tmp/jocky-phase7-test-5835` all absent/removed). `cmake` EXIT=0,
`cmake --build` EXIT=0, **0 warnings**.

- **§1.1 `jocky check samples/sample.jky`** EXIT=0; 13-line AST opening
  `Program / CaseDecl incident_01 capabilities=["netforensics.pcap.read",
  "timeline.correlate"]` — frozen shape intact (full output §1.7).
- **§1.2 `scan_registry stat_scripts/`** EXIT=0 →
  **38 registered / 0 rejected / 16 skipped**
  (recon 10, netforensics 6, hostforensics 2, timeline 3, compliance 15,
  report 2); all 16 SKIP lines read `no @jocky: header (not a registry
  function)` — exact set match with `wiki/23` §4.
- **§1.3 Static fixtures** (exit captured immediately per repo rule):
  Phase 3 — `call_default_ok:0, call_extra_arg:1,
  call_missing_required:1, call_ok:0, call_type_mismatch:1,
  call_unknown:1` (valid×2 pass, 4 refusals). Phase 4 —
  `allow_exact:0`, other five `:1` (binder vs DENIED layering intact).
  Phase 5 — only `fixture4_cycle:1`, rest `:0`. Phase 5.5 — only
  `fixture4_for_callbound:1`, rest `:0`. All match `wiki/12`/`wiki/13`/
  `wiki/16`/`wiki/17` and every prior session's patterns.
- **§1.4 Phase 7.5 execution** (committed 7-file set, interpreted):
  `extra_for_count:0` (4 entries), `extra_for_ident:0` (2),
  `for_three:0` (3), `if_false:0` (**0 entries**),
  `if_true_else:0` (1), `nested:0` (3), `while_capped:1` (5×success +
  `while_ceiling`, status failed — by design). All manifests
  `manifest_version` 0.2.0. (Note: older log rows cite a different
  7-run pattern from uncommitted hostile probes; the committed set above
  is the standing record.)
- **§1.5 `ctest` 10/10**, named: `phase6_embedder_stale_hash`,
  `phase6_jockyc_integration`, `phase7_runtime_safety`,
  `phase7_embedded_runtime`, `phase8_parity_direct`,
  `phase8_parity_if_data`, `phase8_parity_for_literal`,
  `phase8_parity_while_natural`, `phase8_parity_while_capped`,
  `phase8_parity_nested` — CTEST_EXIT=0.
- **§1.6 Parity negative control re-proven:** untampered parity EXIT=0;
  manifest with `exit_code` 0→99 → `parity_diff.py` FAIL with an
  11-line diff naming the field — matches `wiki/22`.
- **§1.7 Phase 9 smoke re-run:** harness EXIT=0,
  **PASS=8 / FAIL=30, 0 functions without a manifest entry** — same 8
  (`jky_timeline_*` ×3, `jky_report_*` ×2, `password_policy`,
  `session_fixation`, `discover_admin` graceful) and same 30
  environment failures (`sed`/`head`/`tail` absent, `tshark`/`python3`/
  `nmap`/`curl` absent, no egress, no `/dev/null`). Byte-for-byte the
  `wiki/23` §4 outcome.
- **§1.8 Quarantine:** `a5c04271…dd91`, `5d237007…88cc`,
  `16ae2468…683e5` — match `wiki/11` §3 exactly. NO DRIFT.
- **§1.9 History:** 15 commits `88b5bd5`→`2225205`, phase order
  0-2 → 3 → 4 → 5+5.5 → 6 → 7 → 7.5 → 8 → 9 → 10 matches `roadmap.md`.
- **§1.10 Manifest integrity:** `for_three` `program_sha256`
  `edf89345b6df74f84291c5e9a8b1ebc66381fd648f376839b583508f7d6ced36`
  == live `sha256sum` of the source — reproduces the `wiki/24` §5.4
  value, cross-session stable.
- **Deviations: NONE.** Every prior-session claim reproduced exactly;
  this is stated explicitly per the audit brief.

## 2. Re-verified 30-row gap-check matrix (source: `wiki/08` §6)

Disposition key: ADOPT · REFRAME · REFUSE · SPLIT · PRESERVE/INVERT
(defensive postures). "Proof" cites this session's evidence or the
pinned record re-verified this session.

| # | PS clause | Disp. | Verdict + proof |
| - | --------- | ----- | --------------- |
| B1 | AV constrains scripting | REFRAME | Satisfied as authorized-operation model: exactly-one-case binding + fail-closed gate re-proven §1.3 (Phase 4 matrix); runtime recheck denies pre-spawn (`phase7_runtime_safety` Passed §1.5). No evasion mechanism exists to audit (§3). |
| B2 | Behavioral heuristics | PRESERVE-FOR-DEFENDERS | Ordinary `fork`/`execv` child processes only (`dispatcher.hpp:646-683`, unchanged since `wiki/18` audit); injection-audit grep this session: zero `system`/`popen`/`sh -c` in `src/`+`include/` (only the words in comments/docs). |
| B3 | Static signatures | PRESERVE (integrity) | `source_sha256` per script, `program_sha256` per run, equality re-proven §1.10; tamper → precise FAIL §1.6. `verify` command itself missing — backlog §6.1, docs corrected §4.1. |
| B4 | Compiler-output fingerprints | INVERT | Stable reproducible builds (same source → same digest, §1.10 cross-session); no post-build mutation stage in `src/compiler/main.cpp` (470 lines, Session read). Rich/imphash-as-detection is doctrine (`wiki/08` §5.2), not yet a registry function — see E3j §6.4. |
| B5 | API-sequence analysis | SATISFY-BY-CONSTRUCTION | Minimal surface (read evidence, write `out/`, exit); full argv + exit + stdout hash per entry in every manifest inspected (§1.4, §5 probes). |
| B6 | Kernel monitoring | PRESERVE | Zero kernel touch: no driver code, no callback/syscall-evasion primitives in 6522 C++ lines; Linux namespaces used for *containment*, not evasion (`dispatcher.hpp`). |
| B7 | CI/CD paradigm | REFRAME | Sameness pipeline exists and passed: `ctest` 10/10 + smoke 8/30 + negative control §1.5–1.7. No mutation stage anywhere. |
| D1 | Framework named JOCKY | ADOPT | Identitykept (`AGENTS.md` §1, binary names `jocky`/`jockyc`). |
| D2 | Cross-platform Win+Ubuntu compiler | ADOPT (partial) | Linux/WSL only, proven (g++ 15.2.0 build §1). No MSVC build, no Windows CI — backlog §6.2. |
| D3 | Systematic scripts + full forensics | ADOPT (partial) | 38-function registry re-proven §1.2; every function dispatcher-invoked §1.7. "Full" bounded by §5 (no query execution) and §1.7 (8/38 pass in minimal sandbox) — backlog §6.3. |
| D4 | "Not hindered by any AV" | REFRAME | Authorization model standing in for evasion, fixture-proven §1.3. No stealth behavior to test — absence proven by §3 ledger. |
| D5 | Polymorphic engines | REFUSE | Absent by construction: verbatim embedding + drift refusal (`embedder.hpp:47-54`); tamper fails builds (`phase6_embedder_stale_hash` Passed). See §3. |
| D6 | Custom encryption (cryptors) | REFUSE | No packer/cryptor in toolchain or 54-script corpus (Session read + §3). |
| D7 | In-memory exec via BYOVD | REFUSE | Subprocess dispatch only; user-space; see §3. |
| D8 | Central management interface | REFRAME | No scheduler/store/client/protocol exists (Session read of all 19 C++ files). Offline-console reframe is Track B future, unstarted — backlog §6.2. |
| D9 | CDN/domain-front/cloud-API routing | REFUSE | No listeners/beacons/sockets in codebase (Session read); see §3. |
| E1 | Custom language/LLVM frontend for evasion | SPLIT | Frontend-as-language proven (§1.1, 809-line parser, published EBNF `wiki/06`+`grammar.md`); stealth half absent (§3); no LLVM dependency (`CMakeLists.txt` Session read). |
| E2 | Per-deploy unique hashes/entry points/imports | REFUSE | Reproducibility proven (§1.10); no uniqueness stage; see §3. |
| E3a–e | 5 in-memory techniques | REFUSE | Absent — see §3 ledger (per-technique ATT&CK mapping retained from `wiki/08` §4.3). |
| E3f–h | Persistence, priv-esc, SOCKS5 | REFUSE | Absent — see §3. No persistence path, no privilege boundary crossed, no sockets. |
| E3i | BYOVD kernel subversion | REFUSE | Absent — see §3. User-space only. |
| E3j | Driver-exposure auditing (inverted shard) | ADOPT (planned) | NOT WRITTEN: zero code/script references (repo-wide grep §4.2). Spec lives at `wiki/08` §5.9 — backlog §6.4. |

## 3. Explicit exclusions ledger (permanent boundary, not debt)

Each item: literal-PS requirement → disposition → rationale pointer →
absence evidence re-verified this session.

- **Polymorphic engine (D5/ES-2).** REFUSE — rationale `wiki/08` §§5.1,
  5.5 + `wiki/24` §4.1 (uniqueness negates admissibility).
  Absence: no IR mutation, no per-build randomization; `embed_scripts`
  throws on a single changed byte (test Passed §1.5).
- **Custom encryption / cryptors (D6/ES-2).** REFUSE — `wiki/08` §5.5.
  Absence: no encrypt/decrypt-stub machinery in toolchain or corpus.
- **Process hollowing T1055.012 (E3a).** REFUSE — `wiki/07` §8.2,
  `wiki/24` §4.2. Absence: no cross-process memory primitives; only
  `fork`+`execv` of own children.
- **Reflective DLL injection (E3b).** REFUSE — `wiki/08` §5.4.
  Absence: no loader re-implementation, no in-memory image mapping.
- **API unhooking (E3c).** REFUSE — `wiki/08` §§5.4, 5.6. Absence: no
  hook manipulation; child processes run hook-visible.
- **Direct syscalls for evasion (E3d).** REFUSE — `wiki/08` §5.6.
  Absence: documented libc APIs only; sandboxes use `unshare` for
  containment.
- **Thread execution hijacking T1055.003 (E3e).** REFUSE — `wiki/08`
  §5.4. Absence: no remote-thread/context/APC primitives.
- **BYOVD / kernel driver access (D7/E3i/ES-3B).** REFUSE — `wiki/07`
  §8.3, `wiki/08` §5.6. Absence: no driver, no vulnerable-driver
  loading, no callback manipulation.
- **Persistence (E3f).** REFUSE — no autostart/service/cron mechanism
  anywhere (the three `audit_cron`-named scripts are headerless
  self-probing stubs flagged in `wiki/23` §2, never registered).
- **Privilege escalation (E3g).** REFUSE — sandbox maps root *inside*
  the namespace only; no host privesc path.
- **SOCKS5 routing (E3h).** REFUSE — `wiki/08` §§5.7–5.8; no sockets,
  nothing to route.
- **C2 channels + domain fronting / cloud-API routing (D8/D9).**
  REFUSE — `wiki/07` §8.4, `wiki/08` §§5.7–5.8; no listeners, no
  beacons, no outbound channel.
- **Enforcement:** `AGENTS.md` §2 (stop-work override) + `wiki/01`
  §2. This ledger is the Phase 11 proof that each weaponizable
  requirement is demonstrably absent with its rationale pointer.

## 4. PART 2 resolutions (unclear items from `wiki/24` §6)

- **§4.1 `jocky verify` — RESOLVED as missing-feature-with-contract.**
  Advertisement sites found: `AGENTS.md:74`, `wiki/06:14`+`:24` (full
  behavior contract), `wiki/02:87`, `wiki/01:63`, `wiki/05:55` (demo
  beat), `roadmap.md:138`, research notes `wiki/07`/`wiki/08`-B3, plus
  recorded GAP-6 (`wiki/18-phase6-7-audit.md:306`). The CLI usage string
  (`src/cli/main.cpp`) never lists it — the binary makes no false
  claim; the docs did. Fix applied this session: contract sources now
  read UNIMPLEMENTED with backlog pointer (`AGENTS.md`, `wiki/01`,
  `wiki/02`, `wiki/05`, `wiki/06`); research/history mentions left as
  intent records. Implementation is backlog §6.1 (spec already written
  at `wiki/06:24`), explicitly post-Track-A — not built now, by decision.
- **§4.2 E3j — RESOLVED as real planned function, not a typo.**
  Repo-wide grep: only `wiki/08` (§5.9 spec, §6-E3j matrix row) and
  `wiki/24` mention `jky_compliance_audit_drivers`. It is the specified
  defensive inversion (enumerate third-party drivers vs MS blocklist),
  slated for "Phase 9 tranche 1" but never written — Phase 9 shipped
  annotations + starters instead. `wiki/24` §6.5 wording ("not yet
  written") was already accurate; no correction needed. Backlog §6.4.
- **§4.3 Pipeline operators — RESOLVED by live probe, finding: PARSE-ONLY.**
  Three end-to-end interpreter probes (`/tmp`, repo untouched):
  (1) `source ev | where | select | sort_by | limit` + `write`, zero
  calls → **status success, 0 manifest entries**, no errors, evidence
  hashed, no output file created;
  (2) `call … | where <impossible> | select | limit` + `write` →
  **1 entry (the call, success)**, filter never evaluated, `write`
  created **no file**;
  (3) `where call … == "branch-true"` → call dispatched (1 entry) but
  the comparison result discarded — filter operators are call-carriers
  only.
  Mechanism (code): `execute_pipeline_stmt` walks calls via
  `ast/walker.hpp`; no relational engine exists behind any operator.
  Consequence: JOCKY's proven capability is **capability-gated call
  dispatch with control flow** — static checking (arity/types/gating/
  shaking) sees the full pipeline, runtime executes the calls. §2 rows
  D3/B5 are cut to fit this fact; `wiki/24` §6's one-line flag is now
  this full finding.
- **§4.4 SIGABRT — RESOLVED by live reproduction, LOW severity.**
  Trigger: `--manifest <outside --output-root>` only (defaults can
  never hit it — the default manifest is always inside the root).
  Observed: uncaught `std::runtime_error("manifest path must be inside
  output root")`, **exit 134 (SIGABRT)**, no MANIFEST line. State
  check: no manifest written anywhere, zero scripts executed (throw is
  preflight, pre-dispatch), evidence untouched; only side effect is two
  empty dirs (`<root>/`, `<root>/.jocky-tmp/`). No corruption, no hang,
  no partial manifest. Same path shared by compiled binaries
  (codegen reuses `run_runtime_plan`). Backlog §6.5 (catch → clean
  `file:line:col` diagnostic, exit 1).

## 5. Verified Working Capability (plain statement)

- **Compiler pipeline, stage by stage:** lex → parse (`check`, §1.1) →
  resolve with arity/default/type errors (§1.3/Ph3) → `for`-bound check
  (call-derived refused, §1.3/Ph5.5) → exactly-one-case bind → fail-closed
  gate with all denials (§1.3/Ph4) → tree-shaken closure, cycles refused
  (§1.3/Ph5) → drift-refusing embed (§1.5/Ph6) → sandboxed execute with
  per-attempt manifest entries (§1.4–1.5) → interpreted twin with 6/6
  parity (§1.5) + tamper FAIL (§1.6).
- **Pipeline-operator truth (§4.3):** operators check and gate; they do
  not execute. Any demo or scorecard claiming query execution is wrong;
  the honest claim is gated dispatch + control flow + manifests.
- **Registry truth:** 38 registered / 0 rejected / 16 skipped (§1.2);
  8/38 pass in the minimal sandbox, 30 fail on missing tools/network/
  `/dev/null`, 0 unlogged (§1.7). The 8 are the operational core today.
- **SIGABRT truth:** preflight-only crash on explicit misuse, exit 134,
  no corruption (§4.4). Robustness bug, not integrity bug.
- **Verify truth:** format verifiable by hand (§1.10), command absent,
  docs corrected (§4.1).

## 6. Post-Track-A corrective backlog (not roadmap phases)

- **§6.1 `jocky verify` subcommand** (spec: `wiki/06:24`; procedure
  stand-in: `wiki/24` §5.4). Top gap: closes the integrity loop.
- **§6.2 Platform/management:** Windows/MSVC build + CI (D2); offline
  multi-case console or explicit descoping (D8); Track B IDE (Ph12–16)
  untouched.
- **§6.3 Registry honesty:** provision sandbox tooling for the 30
  env-failing functions, or mark their scope (demo vs operational);
  the 8-pass core is the shippable claim.
- **§6.4 Registry growth:** write `jky_compliance_audit_drivers`
  (spec `wiki/08` §5.9); human-review the 15 headerless scripts
  (`wiki/23` §2) — keep-or-drop each, no bulk annotation.
- **§6.5 Robustness:** catch manifest-path preflight → diagnostic exit 1
  (replaces SIGABRT, §4.4).
- **§6.6 Capability honesty:** either implement runtime relational
  operators or re-spec `.jky` pipelines as dispatch-with-annotations
  (§4.3 finding); docs must not imply query execution.

## 7. Unaddressed literal-PS text — sweep result

Checked each Background sentence, Description clause, and ES-1–ES-3
requirement (`wiki/08` §§2–4) against §§2–3 above: every clause lands
in exactly one of (a) adopted/reframed with proof, (b) refused with
rationale + absence evidence, or (c) backlog §6. **Nothing found
unaddressed.** The nearest near-miss, B4's "Rich/imphash as detection
pivots," is doctrine without a registry function yet — covered under
E3j/backlog §6.4, not unaddressed.

## 8. Audit artifacts index

Build `/tmp/audit-build` (removed at close); logs
`/tmp/audit-{cmake,build,check.*,scan.*,matrix.txt,ctest.log,neg-*,smoke.log,pipe/*}`
(removed at close). Standing records: fixtures in `tests/`
(§1.3–1.4 matrices), harness scripts `tests/phase8/*` + `tests/phase9/*`,
baselines `wiki/11` §3, smoke table `wiki/23` §4, rationale `wiki/24`,
this document. Per `AGENTS.md` §6, every hash/count above is persisted
here at computation time — no chat-only evidence.
