# Whole-Project Alignment Audit — SIH26148

**Audit date:** 2026-09-12  
**Current phase:** Phase 6 — script embedding and `jockyc`

## Finding

JOCKY is a credible static front end and execution-plan prototype for an
authorized forensic DSL. It is not yet an end-to-end compiler/runtime. The
implemented path ends at `jocky shake`: parse, resolve, validate primitive
argument types, bind one case, check declared capabilities, and calculate a
dependency-ordered script closure.

The project correctly refuses the problem statement's weaponizable clauses:
polymorphism, antivirus/EDR evasion, process injection, API unhooking, direct
syscalls, BYOVD, persistence, privilege escalation, SOCKS routing, C2, and
domain fronting. Its safe interpretation is an authorized, predictable,
manifested forensic tool.

## Verified implementation state

| Area | State | Evidence and implication |
| --- | --- | --- |
| Language front end | Implemented | C++20 lexer, parser, AST, pipelines, calls, and control flow exist. `jocky check samples/sample.jky` exits 0. |
| Call resolution/types | Implemented for registry calls | Named arguments, missing/extra rejection, primitive type checks, defaults, and return types behave as documented. |
| Static capability gate | Implemented | Exactly one explicit case is required and all call capabilities are checked. The required runtime re-check does not exist. |
| Control-flow analysis | Partial | `if`/`else` and constrained `for` work. `while` is only marked `requires_runtime_ceiling`; no dispatcher enforces it. |
| Tree shaking | Implemented | Direct, transitive, diamond, mixed, and cycle cases behave as documented. Scripts are listed, not embedded. |
| Registry | Partial | Live scan: **7 registered, 0 rejected, 42 skipped**. Coverage is six net-forensics functions and one compliance wrapper. |
| `jockyc` and embedding | Missing | No target, embedder, embed-time hash verification, generated runner, or standalone artifact exists. |
| Runtime dispatcher | Missing | No process execution, timeout, output capture, sandbox, or runtime gate exists. |
| Manifest and verification | Missing | No manifest writer, evidence/output hashing, attempt record, or `jocky verify` command exists. |
| Read-only evidence enforcement | Design only | Evidence paths parse, but writes and script output paths are not checked against them. |
| Interpreter/parity | Missing | `jocky <file.jky>` does not execute, so compiled/interpreted manifest parity cannot be tested. |
| Adapters/correlation | Design only | PCAP, eventlog, directory, FIR evaluation, pipeline operations, and correlation are syntax or documentation, not runtime behavior. |
| Central management | Missing | No scheduler, case store, UI, client, or management protocol exists. |
| Windows + Ubuntu | Missing | Phase 6 is Linux/WSL-only and the script registry is Bash. |
| Test automation | Partial | Fixtures exist and pass manually; CMake has no CTest registration or CI workflow. |

## Live verification

The available Ubuntu environment had no CMake. Both current targets compiled
successfully in `/tmp` with Ubuntu g++ 13.3.0 and
`-std=c++20 -O2 -Iinclude`.

- Sample parse: exit 0.
- Registry scan: exit 0; **7 registered, 0 rejected, 42 skipped**;
  netforensics 6, compliance 1, every other declared domain 0.
- Phase 3: two valid fixtures pass; four invalid fixtures refuse with the
  expected unknown, missing, extra, or type diagnostics.
- Phase 4: exact allowance passes; five invalid authorization shapes refuse.
- Phase 5: direct, chain, diamond, and mixed pass; the dependency cycle refuses.
- Phase 5.5: five valid resolve fixtures pass; the call-derived loop bound
  refuses.
- `bash -n` over every `.sh` under `stat_scripts/`: exit 0.

These checks prove the existing static stages. They do not prove script
execution, forensic output, evidence protection, manifests, standalone
compilation, Windows behavior, or multi-host management.

## Alignment with the problem statement

| Theme | Assessment |
| --- | --- |
| Independent language | Partially met: real syntax and analysis exist; code generation and execution do not. |
| Computer/network forensics | Early partial: callable PCAP analysis is the strongest asset. Host scripts are unregistered; timeline and report implementations are absent. |
| Cross-platform compiler | Not met. |
| Central simultaneous analysis | Not met. |
| Security-product compatibility | Safely reframed as authorization, stable artifacts, logging, signing/allowlisting, and transparent behavior. |
| Polymorphism and binary mutation | Permanently refused because they defeat provenance and reproducibility. |
| Injection, unhooking, direct syscalls | Permanently refused as defense-evasion behavior. |
| BYOVD and EDR impairment | Permanently refused; defensive vulnerable-driver detection remains appropriate. |
| Persistence, privilege escalation, SOCKS5, domain fronting | Permanently refused; the safe design is local/offline and investigator-authorized. |

## Recommended build order

1. Complete Phase 6 narrowly: embed only the authorized shaken closure, hash
   every source at embed time, and produce a deterministic Linux artifact.
2. Add Phase 6 CTest coverage for exact embedded sets, stale-file refusal,
   denied-gate refusal, and operation without the live registry.
3. Make Phase 7's runtime boundary the core security feature: canonicalize
   paths, reject writes to evidence, re-check capabilities, avoid shell
   interpolation, enforce ceilings/timeouts, and log every attempt.
4. Implement manifest verification and compiled/interpreted parity before
   expanding the registry.
5. Audit skipped scripts individually. Several perform live network/cloud
   reconnaissance; several host scripts inspect `/` and invoke `sudo -l`.
   They need evidence-vs-live-host classification and least-privilege schemas,
   not bulk annotation.
6. Add timeline/report functions, eventlog/directory adapters, and then
   Windows-native execution and CI.
7. Build the offline multi-case console after runtime manifests and parity are
   stable.

## Readiness judgment

The repository is **architecturally aligned but operationally incomplete**.
Its strongest asset is the typed, capability-gated, minimal-script planning
chain. Until Phases 6–9 are complete, it should be presented as a static
compiler-front-end prototype. It should not claim complete digital forensics,
standalone compilation, enforced read-only evidence, manifests, Windows
support, or centralized analysis.


## Post-audit update

Phase 6 was completed after this point-in-time audit on 2026-09-12. The
`jockyc`, embedding, and standalone-artifact gaps above are now closed as
recorded in `wiki/19-phase6-embedding.md`. Runtime execution, evidence
enforcement, manifests, parity, UI, and Windows support remain open.
