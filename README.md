# Jangiya

# SIH WINNER 2026

## JOCKY — Phases 1–3 Complete, Phase 4 Current

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
| `roadmap.md` | Full plan, Phases 0–11 (changes rarely) |
| `implementationplan.md` | Current phase detail only (Phase 4: capability gate enforcement) |
| `logs.md` | Session log, one row per session |
| `wiki/00-index.md` | Wiki table of contents |
| `wiki/01-prd.md` | Product requirements + the permanent no-evasion boundary |
| `wiki/02-architecture.md` | Compiler/runtime pipeline, `jockyc` vs `jocky` paths |
| `wiki/03-graph-schema.md` | Evidence entities + query-time correlation model |
| `wiki/04-ingestion-pipeline.md` | Read-only adapters (pcap, eventlog, directory) |
| `wiki/05-frontend-design.md` | Static report MVP, dashboard stretch, demo narrative |
| `wiki/06-api-contracts.md` | CLI, FIR/manifest/`@jocky:` schemas, `.jky` EBNF |
| `wiki/07-research.md` | Research base: PS provenance, NTRO, SIH scoring, tool landscape, formats, ISO/NIST/SLSA, 65B admissibility, exclusions, precedents |
| `wiki/08-ps-analysis.md` | Full-PS clause analysis, gap-check matrix, CERT-In/DPDP/D3FEND closings, residual backlog |
| `wiki/09-kalki-inventory.md` | Phase 1 audit: script triage, src/ verdict, prune/rename/netforensics/quarantine records, Phase 2 handoff |
| `wiki/10-phase1-verification.md` | Read-only checkpoint: build evidence, counts, audits, root-file inspection |
| `wiki/11-phase2-drift-check.md` | Read-only drift checkpoint + first quarantine SHA-256 baseline |
| `wiki/12-phase3-resolution.md` | Phase 3 record: `= default` decision, resolver rules, six fixture runs |
| `include/jocky/` | C++20 frontend: `lexer/`, `ast/`, `parser/` + `stdlib/script_metadata.hpp` + `semantic/call_resolver.hpp` |
| `src/cli/main.cpp` | `jocky check` (lex → parse → AST) + `jocky resolve` (parse → resolve against registry) |
| `src/stdlib/metadata_scanner.cpp` | `scan_registry` engine (header parse, validation, JSON index) |
| `tools/scan_registry.cpp` | `scan_registry <dir>` CLI driver |
| `CMakeLists.txt` | CMake 3.20+, C++20, `jocky` + `scan_registry` targets |
| `samples/sample.jky` | Sample exercising every grammar construct (parse-level fixture; intentionally unresolved — see `wiki/12`) |
| `tests/phase3/` | Six `call` resolution fixtures (unknown/missing/default-ok/mismatch/extra/correct) for `jocky resolve` |
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
- [ ] Phase 4 (current): case-level authorization + capability gate
  enforcement over resolved calls — see `implementationplan.md`

### Safety boundary (permanent, see `AGENTS.md` §2)

No evasion/weaponization (polymorphic engines, injection/hollowing,
BYOVD, C2/domain-fronting, etc.). Every execution capability-gated,
manifest-logged, read-only over evidence, with type-validated arguments.
