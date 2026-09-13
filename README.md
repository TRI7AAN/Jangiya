# Jangiya

# SIH WINNER 2026

## JOCKY — Phases 1–8 Complete, Phase 9 Current

**JOCKY** is a C++20 compiler/runtime for a forensic domain-specific
language (`.jky` files), built for **SIH26148 (NTRO, SIH 2026,
Blockchain & Cybersecurity)**: authorized, low-footprint, fully
auditable computer & network forensic analysis. Every agent session
starts at `AGENTS.md`, then `implementationplan.md` (current phase
only), and ends by appending to `logs.md`.

### Repo map

| Path | What it is |
| ---- | ---------- |
| `AGENTS.md` | Session entry point: identity, hard safety rules, conventions, toolchain, protocol |
| `roadmap.md` | Full plan, Phases 0–11 Track A + Track B IDE shell (Phases 12–16, changes rarely) |
| `implementationplan.md` | Current phase detail only (Phase 9: real registry finalization) |
| `logs.md` | Session log, one row per session |
| `wiki/00-index.md` | Wiki table of contents |
| `wiki/01-prd.md` | Product requirements + the permanent no-evasion boundary |
| `wiki/02-architecture.md` | Compiler/runtime pipeline, `jockyc` vs `jocky` paths |
| `wiki/03-graph-schema.md` | Evidence entities + query-time correlation model |
| `wiki/04-ingestion-pipeline.md` | Read-only adapters (pcap, eventlog, directory) |
| `wiki/05-frontend-design.md` | Static report MVP, dashboard stretch, demo narrative |
| `wiki/06-api-contracts.md` | CLI, FIR/manifest/`@jocky:` schemas, `.jky` EBNF |
| `wiki/grammar.md` | Authoritative `.jky` syntax reference with usage (lexical rules, declarations, heads, predicates, operators, types, control flow, verified examples) |
| `wiki/keywords.md` | Reserved-word roll: all 150 keywords (42 `.jky` + C/C++/Java sets), usage table, reservation record |
| `wiki/07-research.md` | Research base: PS provenance, NTRO, SIH scoring, tool landscape, formats, ISO/NIST/SLSA, 65B admissibility, exclusions, precedents |
| `wiki/08-ps-analysis.md` | Full-PS clause analysis, gap-check matrix, CERT-In/DPDP/D3FEND closings, residual backlog |
| `wiki/09-kalki-inventory.md` | Phase 1 audit: script triage, src/ verdict, prune/rename/netforensics/quarantine records, Phase 2 handoff |
| `wiki/10-phase1-verification.md` | Read-only checkpoint: build evidence, counts, audits, root-file inspection |
| `wiki/11-phase2-drift-check.md` | Read-only drift checkpoint + first quarantine SHA-256 baseline |
| `wiki/12-phase3-resolution.md` | Phase 3 record: `= default` decision, resolver rules, six fixture runs |
| `wiki/13-phase4-capability-gate.md` | Phase 4 record: case binder, capability gate, `jocky gate`, six fixture runs |
| `wiki/14-phase3-verification.md` | Retroactive persistence + live re-run of the chat-only verification (build, AST, 7/0/42, 6/6 fixtures, quarantine) |
| `wiki/16-phase5-tree-shaking.md` | Phase 5 record: `shake` naming decision, dependency-closure resolver, throwaway 12-script registry, five fixture runs, Phase 6 handoff |
| `wiki/17-control-flow-proposal.md` | PROPOSAL (undecided): C-style control flow in `.jky`, bounded vs unbounded paths, recommendation awaiting confirm |
| `wiki/17-control-flow.md` | Phase 5.5 record: C-style `if`/`for`/`while`, three boundable `for` forms, shared AST walker, runtime-ceiling commitment, six fixture runs |
| `wiki/18-project-alignment-audit.md` | Whole-project SIH26148 alignment audit with verified status, gaps, and recommended order |
| `wiki/19-phase6-embedding.md` | Phase 6 record: SHA-256 freshness, exact embedding, `jockyc`, standalone artifacts, and verification |
| `wiki/20-phase7-runtime.md` | Phase 7 record: isolated dispatcher, runtime rechecks, evidence/output staging, complete manifests, hashes, and verification |
| `include/jocky/` | C++20 frontend + Phase 6 embedding + Phase 7 runtime + Phase 7.5 control-flow execution: `lexer/`, `ast/` (+`ast/walker.hpp` shared call collector), `parser/` + `stdlib/script_metadata.hpp` + `semantic/call_resolver.hpp` + `semantic/case_binder.hpp` + `semantic/bound_checker.hpp` + `policy/capability_gate.hpp` + `fir/script_resolver.hpp` + `runtime/dispatcher.hpp` + `runtime/predicate_evaluator.hpp` + `runtime/control_flow_executor.hpp` |
| `src/cli/main.cpp` | `jocky check` (lex → parse → AST) + `jocky resolve` (parse → resolve against registry) + `jocky gate` (resolve → bind → gate verdict) + `jocky shake` (gate → dependency-ordered script list) |
| `src/compiler/main.cpp` | `jockyc` static chain, generated standalone source, and shell-free host-compiler invocation |
| `src/stdlib/metadata_scanner.cpp` | `scan_registry` engine (header parse, validation, JSON index, scan-time SHA-256) |
| `tools/scan_registry.cpp` | `scan_registry <dir>` CLI driver |
| `CMakeLists.txt` | CMake 3.20+, C++20, `jocky`, `jockyc`, `scan_registry`, and Phase 6/7 CTest targets |
| `samples/sample.jky` | Sample exercising every grammar construct (parse-level fixture; intentionally unresolved — see `wiki/12`) |
| `tests/phase3/` | Six `call` resolution fixtures (unknown/missing/default-ok/mismatch/extra/correct) for `jocky resolve` |
| `tests/phase4/` | Six gate fixtures (zero/duplicate cases, field-absent vs field-empty, exact-allow, partial-allow with both denials) for `jocky gate` |
| `tests/phase5/` | Five shake fixtures (direct/chain/diamond/cycle/mixed) + throwaway 12-script `fixtures/registry/` for `jocky shake` |
| `tests/phase5.5/` | Six control-flow fixtures (if-else/for-literal/for-const/for-callbound/while/nested) for all four commands |
| `tests/phase6/` | SHA-256 drift unit test and `jockyc` integration test (exact set/order, extraction, denied gate, registry independence, zero scripts) |
| `tests/phase7/` | Shared-runtime safety suite and registry-deleted standalone execution integration |
| `tests/phase8/` | Six parity fixtures (direct/if-data/for/while-natural/while-capped/nested) + `run_parity.sh`/`parity_diff.py` harness proving compiled and interpreted manifests agree |
| `stat_scripts/` | Registry: `recon/` (10), `compliance/` (24), `hostforensics/` (7), `netforensics/` (6), `shared/` (testssl wrapper + vendored `testssl.sh`) — 7 headers present, 41 pending Phase 9 |
| `_quarantine/` | Origin-unconfirmed files (CSV/JSON reference data), excluded from registry/scanner/docs pending owner confirmation |

### Status

- [x] Phase 0: wiki bootstrap + full-PS research — complete, logged in `logs.md`
- [x] Phase 1: Kalki audit (`wiki/09`), fresh C++20 skeleton (`jocky check`
  builds clean, sample exits 0), repo prune (66 files), registry
  canonicalization (41 scripts renamed to `jky_*`), netforensics
  greenfield batch (6 tested scripts) — complete, logged in `logs.md`
- [x] Phase 2: `@jocky:` metadata convention + `scan_registry` (7 registered /
  0 rejected on the real tree, hostile fixtures correctly rejected);
  unconfirmed reference files quarantined in `_quarantine/` — complete,
  logged in `logs.md`
- [x] Phase 3: `call` resolution + arity/type checking + return-type
  plumbing (`include/jocky/semantic/call_resolver.hpp`, `jocky resolve`,
  `= default` stored on registry inputs, 6/6 fixtures green) — complete,
  logged in `logs.md`
- [x] Phase 4: case binding (exactly-one-case, `capabilities_declared`)
  + capability gate (`include/jocky/semantic/case_binder.hpp`,
  `include/jocky/policy/capability_gate.hpp`, `jocky gate`, 6/6 fixtures
  green: binder refusals vs gate denials) — complete, logged in `logs.md`
- [x] Phase 5: dependency-aware tree-shaking resolver
  (`include/jocky/fir/script_resolver.hpp`, `jocky shake`, closure over
  `GateResult::authorized` only, 5/5 fixtures green on a throwaway
  12-script registry: direct/chain/diamond/cycle/mixed) — complete,
  logged in `logs.md`
- [x] Phase 5.5: control-flow extension (`if`/`else`, C-shape `for` with
  bound-checked bounds, `while` flagged for runtime ceilings;
  `ast/walker.hpp` shared recursion, `semantic/bound_checker.hpp`, 6/6
  `tests/phase5.5/` green, no new CLI command) — complete, logged in
  `logs.md`
- [x] Phase 6: scan-time SHA-256 + embed-time freshness refusal, exact-closure
  embedding, and `jockyc` standalone Linux artifact generation — complete,
  logged in `wiki/19-phase6-embedding.md`
- [x] Phase 7: shared isolated runtime, runtime capability/type/hash rechecks,
  argv-only dispatch, evidence snapshots, staged output promotion, timeouts,
  execution ceilings, and complete integrity manifests — complete, logged in
  `wiki/20-phase7-runtime.md`
- [x] Phase 7.5: execution-time control-flow interpreter (taken-branch-only
  `if`, real `for` trips, re-evaluated `while` under a hard configurable
  ceiling), shared dispatch path, manifest schema 0.2.0, registry_version
  flag, hostile fixtures green — complete, logged in
  `wiki/21-phase7-5-control-flow.md`
- [x] Phase 8: `jocky <file.jky>` no-compile interpreter reusing the
  shared Phase 7.5 executor (filesystem sourcing via
  `load_registry_runtime_call` the only structural difference) +
  compiled/interpreted parity harness (exact match minus timing fields,
  script_sha256 asserted separately), 6/6 control-flow fixtures green —
  complete, logged in `wiki/22-phase8-interpreter-parity.md`
- [ ] Phase 9 (current): real registry finalization — see
  `implementationplan.md`

### Track plan (A: toolchain, B: Java IDE shell)

Track A is the forensic toolchain (Phases 5–8 complete, Phase 9 current
per Status above). Track B is a parallel Java IDE shell over the CLI —
it starts immediately on the existing `jocky`/`jockyc` commands and never
blocks Track A:

| Phase | Track | Focus | Depends on |
| ----- | ----- | ----- | ---------- |
| 5 | A | Tree-shaking resolver (in progress) | Phase 4 |
| 6 | A | Script embedding + jockyc binary generation | Phase 5 |
| 7 | A | Sandboxed runtime dispatcher + manifest logging | Phase 6 |
| 8 | A | jocky interpreter + compiled/interpreted parity test | Phase 7 |
| 9 | A | Real registry finalization (header gaps, timeline/report domains, smoke tests) | Phases 2, 7, 8 |
| 12 | B | Java shell + editor pane — window layout mirroring VS Code (left file tree, center editor, bottom terminal, right/bottom output panel), using JavaFX with an embedded Monaco editor (via WebView) for real .jky syntax highlighting without hand-rolling a highlighter | Nothing from Track A — can start immediately, using Phase 3–4's existing CLI |
| 13 | B | Integrated terminal pane — spawn jocky/jockyc as a subprocess, stream stdout/stderr live into a terminal-style widget, support interactive re-runs without leaving the app | Phase 12 |
| 14 | B | Structured output/monitor panel — parse jocky gate/jocky shake output (and later Phase 7's manifest JSON) into readable panels: a findings table, a dependency tree view for shaken scripts, colored ALLOWED/DENIED banners, and clickable file:line:col diagnostics that jump the editor cursor to the error | Phase 12; richer once Phase 7's manifest exists |
| 10 | A | Judge-facing rationale doc | Phase 6 (needs the binary to describe) |
| 11 | A | Final gap-check audit vs. SIH26148 | Everything in Track A |
| 15 | B | Manifest/report visualization — once Phase 7 exists, render the evidence-integrity manifest as a proper case report view (hash chain, execution log, timeline) instead of raw JSON | Phase 7 |
| 16 | B | Packaging — jpackage into a distributable installer/jar bundling the Java app, invoking either a system-installed jocky/jockyc or a bundled copy | Phase 6 (for jockyc), Phase 15 |

See `roadmap.md` (Phases 12–16) and `wiki/05-frontend-design.md` (§4) for
the same plan on the wiki side.

### Safety boundary (permanent, see `AGENTS.md` §2)

No evasion/weaponization (polymorphic engines, injection/hollowing,
BYOVD, C2/domain-fronting, etc.). Every execution capability-gated,
manifest-logged, read-only over evidence, with type-validated arguments.
