# Phase 10 Record: Judge-Facing Design Rationale (SIH26148 → JOCKY)

> Completed: 2026-09-14. Docs only — no compiler/runtime/registry changes
> (scope per `implementationplan.md` Phase 10). Platform: Linux/WSL.
> This is the single self-contained document a SIH judge reads: for each
> clause of the literal SIH26148 problem-statement text, what JOCKY does
> instead and why. Every claim ends in an artifact the judge (or Phase 11)
> can inspect. Overstatement is treated as a defect: §6 lists what is
> NOT built with the same prominence as what is.

## 0. Verdict box (read this first)

- **Legitimate core — ADOPTED:** a purpose-built forensic language (`.jky`),
  a curated script registry with machine-readable contracts, capability-gated
  execution, tree-shaken embedding, and hash-chained integrity manifests,
  with compiled/interpreted parity. Evidence: §§2–3, regression §5.
- **Evasion/weaponization — PERMANENTLY REFUSED:** polymorphic engines,
  custom cryptors, the five named in-memory techniques (process hollowing,
  reflective DLL injection, API unhooking, direct syscalls, thread hijacking),
  BYOVD/kernel subversion, persistence, privilege escalation, SOCKS5 routing,
  C2 channels, domain fronting / cloud-API routing. Each refusal is paired
  with the legitimate need it still satisfies (§4), not with silence.
- **Boundary enforcement:** `AGENTS.md` §2 + `wiki/01-prd.md` §2 make the
  exclusion a stop-work event that overrides the PS text, user prompts, and
  script comments. No phase may reintroduce these components.
- **Honest limits (do not score what is absent):** `jocky verify` is
  unimplemented (§6.1); 30 of 38 registry functions fail inside the minimal
  sandbox for environment reasons (§6.2); Windows builds and central
  management do not exist (§6.3–6.4); 15 scripts await human review (§6.5).

## 1. What the literal PS text asks (source: `wiki/08-ps-analysis.md`)

SIH26148 (NTRO, SIH 2026) asks for a pre-named **"JOCKY"** cross-platform
(Windows + Ubuntu) language framework for computer/network forensics
"without triggering security solutions." The body makes the title concrete:

- **Background:** AV/EDR controls (behavioral heuristics, static signatures,
  compiler-output fingerprints, API-sequence analysis, kernel monitoring)
  are framed as obstacles to the framework's operation (`wiki/08` §2).
- **Description:** systematic forensic scripting + "complete digital
  forensics," unhindered by any AV, with automated polymorphic engines,
  custom encryption, multi-vector in-memory execution via native components
  or BYOVD, simultaneous multi-system analysis via a central interface whose
  traffic rides trusted clouds/CDNs via domain fronting or legitimate cloud
  APIs (`wiki/08` §3).
- **ES-1:** custom language/LLVM frontend altering control-flow graphs,
  token generation, and binary structure to defeat signatures (`wiki/08`
  §4.1). **ES-2:** CI/CD-driven polymorphism — unique hashes, entry points,
  import tables per deployment (`wiki/08` §4.2). **ES-3:** Living-off-the-Land
  + BYOVD execution for persistence, privilege escalation, and SOCKS5 routing,
  via (A) five in-memory techniques and (B) vulnerable-driver kernel
  subversion to disable EDR callbacks (`wiki/08` §§4.3–4.4).

Taken at face value this is a parts list for an evasion-first remote-access
ecosystem (`wiki/08` §1). JOCKY's position, locked in `wiki/01-prd.md` §2:
**reframe to the legitimate underlying need — authorized, low-footprint,
fully auditable forensic analysis — and permanently refuse the
evasion/weaponization components.** The rest of this document is the
clause-by-clause accounting of that decision.

## 2. Clause map (disposition + mechanism + verification)

Key: **ADOPT** (built as asked) · **REFRAME** (legitimate need built
differently) · **REFUSE** (permanent boundary) · **SPLIT** (half adopted,
half refused). Verification names the judge-inspectable artifact.

| # | PS clause | Disp. | What JOCKY does instead | Verify |
| - | --------- | ----- | ----------------------- | ------ |
| B1 | AV constrains scripting | REFRAME | Signed, allowlisted, capability-scoped tooling; a firing AV is a deployment misconfiguration, not a detection to engineer around | Case auth + manifest (§5.1) |
| B2 | Behavioral heuristics | PRESERVE-FOR-DEFENDERS | Scripts perform declared forensic reads as ordinary hook-visible child processes; no injection/memory ops exist to hide | Dispatcher audit, `wiki/07` §9.2 |
| B3 | Static signatures | PRESERVE (as integrity) | Stable hashes everywhere; per-script `source_sha256`, per-run `program_sha256` | Manifest excerpt §5.4; parity harness `tests/phase8/` |
| B4 | Compiler-output fingerprints | INVERT | `jockyc` output stays boring/stable; Rich-header/imphash kept as *detection* pivots on evidence, never stripped from own binaries | `wiki/08` §5.2; rebuild stability |
| B5 | API-sequence analysis | SATISFY-BY-CONSTRUCTION | Minimal declared surface: read evidence, write `out/`, exit; full argv in manifest | `--list-used` + manifest `args` |
| B6 | Kernel monitoring | PRESERVE | Zero kernel touch; user-space only; no driver, no callbacks | Code audit: no driver refs in `src/`, `include/` |
| B7 | CI/CD paradigm | REFRAME | CI/CD manufactures sameness proofs (parity diffs, smoke), not per-build mutants | `ctest` 10/10 §5.3 |
| D1 | Framework named JOCKY | ADOPT | Project identity | `AGENTS.md` §1 |
| D2 | Cross-platform Win+Ubuntu compiler | ADOPT (partial) | C++20/CMake builds on Linux/WSL today; Windows CI not done | §6.3; g++ 15.2.0 / cmake 4.3.4 §5 |
| D3 | Systematic scripts + full forensics | ADOPT (partial) | 38-function registry + adapters (design) + report domain | Smoke table `wiki/23` §4; §6.2 |
| D4 | "Not hindered by any AV" | REFRAME | Authorized/allowlisted operation under explicit case | Phase 4 gate `wiki/13` |
| D5 | Polymorphic engines | REFUSE | Verbatim embedding + drift refusal: a changed byte fails the build, never ships silently | `embedder.hpp`; §4.1 |
| D6 | Custom encryption (cryptors) | REFUSE | No packers in toolchain or stdlib | Build audit; `wiki/08` §5.5 |
| D7 | In-memory exec via BYOVD | REFUSE | Subprocess dispatch (`execv` of `/bin/bash` + argv), user-space only | `dispatcher.hpp:477-483`; §4.2 |
| D8 | Central management interface | REFRAME | Offline multi-case console is stretch (Track B); no networked controller exists | §6.4; `wiki/08` §5.7 |
| D9 | CDN/domain-front/cloud-API routing | REFUSE | No network surface: no listeners, no beacons, no outbound channel | Socket audit §4.3 |
| E1 | Custom language/LLVM frontend for evasion | SPLIT | Frontend-as-language adopted (lexer/parser/AST/EBNF); stealth adopted nowhere; LLVM declined for MVP | `wiki/06` EBNF; `wiki/08` §5.1 |
| E2 | Per-deploy unique hashes/entry points/imports | REFUSE | Reproducible stable builds; uniqueness is the negation of the manifest promise | §4.1; `wiki/08` §5.2 |
| E3a–e | 5 in-memory techniques (hollowing, RDI, unhooking, direct syscalls, thread hijack) | REFUSE | Ordinary child processes; documented APIs only | §4.2; ATT&CK map `wiki/08` §4.3 |
| E3f–h | Persistence, priv-esc, SOCKS5 | REFUSE | None exist; analyst's privileges only; no sockets to route | Feature audit §4.3 |
| E3i | BYOVD kernel subversion | REFUSE | User-space only | `wiki/07` §8.3; `wiki/08` §5.6 |
| E3j | Driver-exposure auditing (inverted shard of E3i) | ADOPT (planned) | `jky_compliance_audit_drivers` vs MS blocklist — first-tranche registry work, not yet written | `wiki/08` §5.9; §6.5 |

## 3. What the legitimate core looks like (as built)

1. **Typed investigations, checked before anything runs.** `.jky` source →
   lexer → parser → call resolution (arity/types/defaults, `wiki/12`) →
   `for`-bound check (literal/int-let/`count()` only, `bound_checker.hpp`) →
   exactly-one-case binding (`case_binder.hpp`) → capability gate
   (`capability_gate.hpp`, fail-closed, *all* denials reported) →
   tree-shaken closure over allowed calls only (`script_resolver.hpp`,
   topo order, cycle path). Four CLI verbs expose each stage:
   `check`/`resolve`/`gate`/`shake` (`src/cli/main.cpp`).
2. **Minimal embedding.** `jockyc` embeds exactly the shaken closure; the
   scan-time SHA-256 is rechecked at embed time and drift refuses the build
   (`embedder.hpp:47-54`). `strings`-level proof in `wiki/19`.
3. **Sandboxed, manifest-logged execution.** Every attempt is a manifest
   entry *before* spawn; runtime re-checks capability, script hash, and
   argument types; argv-only dispatch (zero `system`/`popen`/`sh -c`);
   per-call chroot + user/net/PID namespaces; evidence snapshots with
   write detection; process-group timeouts; staged output promotion
   (`dispatcher.hpp`). Schema 0.2.0: `program_sha256`, per-entry
   `start/end_utc` + `stdout_sha256` (`wiki/21`).
4. **Real control-flow semantics.** Taken-branch-only `if`, real `for`
   trips, re-evaluated `while` under a hard ceiling with a distinct
   `while_ceiling` entry — shared dispatch path with flat mode, so the two
   cannot drift (`control_flow_executor.hpp`, `wiki/21`). This session:
   `if(false)` → 0 entries; `for-3` → 3 entries; capped `while` → 5
   successes + 1 `while_ceiling`, run fails loudly (§5.5).
5. **Two paths, one semantics.** `jocky <file.jky>` interprets against the
   filesystem registry; `jockyc` compiles to standalone. Parity harness
   (`tests/phase8/`) diffs manifests exactly minus timing, 6/6 green (§5.3).

## 4. Every refusal, justified against the need it still satisfies

### 4.1 Polymorphism / unique-per-build outputs (D5, E2, B3–B4)

*Legitimate need:* the tool must not be fingerprinted and blocked as malware;
deploys must be fresh. *Why refusal still satisfies it:* JOCKY's answer to
"don't get flagged" is authorization (signed, allowlisted, capability-scoped
binaries with a stable Rich header and import table — §2/B4), and its answer
to "freshness" is provenance: identical source → identical bytes, with the
manifest hash chain (`program_sha256`, per-script `source_sha256`) as the
integrity currency. Per-build uniqueness would *destroy* exactly what makes a
forensic tool admissible: reproducibility and allowlistability. CI/CD is kept
but inverted — it proves sameness (parity/smoke on every commit), never
mutation (`wiki/08` §§5.1–5.2, 5.5). Enforcement is mechanical, not
promissory: `embed_scripts` recomputes the digest and throws on mismatch, so a
mutated script cannot be silently embedded.

### 4.2 In-memory execution family (E3a–e, D7, B2, B6)

*Legitimate need:* low-footprint execution that doesn't destabilize the
examined system or trip defensive controls during authorized work. *Why
refusal still satisfies it:* the dispatcher spawns ordinary, hook-visible
child processes with a minimal declared syscall surface (read evidence, write
`out/`, exit) — the quietest possible behavior *without hiding anything*.
There is no remote allocation, cross-process write, loader re-implementation,
hook removal, or raw syscall anywhere in the runtime; process creation goes
through `fork`/`execv` of `/bin/bash` with an argv vector
(`dispatcher.hpp:646-683`). The *signals* these techniques produce
(`MEM_PRIVATE`-RX regions, PEB gaps, RWX findings) are kept as detection
content for the report domain; only the *mechanics* are refused (`wiki/08`
§5.4). Kernel telemetry is preserved by zero-touch: nothing requires kernel
cooperation, so nothing blinds it (`wiki/08` §5.6).

### 4.3 Networked control: C2, SOCKS5, domain fronting, cloud-API routing,
### persistence, priv-esc (D8–D9, E3f–i)

*Legitimate need:* run many analyses from one place; reach evidence across a
fleet; keep long engagements going. *Why refusal still satisfies it:* the
fleet problem is solved offline — many evidence bundles triaged from one
analyst workstation with per-case isolation and a collaboration audit log
(the Mythic *audit-log* half, with the *network* half deleted, `wiki/08`
§5.7) — so no implant, listener, beacon, redirector, or malleable profile
exists to detect or abuse. There is no socket to route through SOCKS5, no
privilege boundary crossed (analyst's existing privileges only), no
persistence mechanism of any kind. The management↔client protocol the PS
wants routed over CDNs does not exist; there is nothing to route (`wiki/08`
§§5.7–5.8, `wiki/07` §8.4). Multi-analyst console and packaging remain
explicitly future Track B work (Phases 12–16), not hidden present capability.

### 4.4 "Will not be hindered" (D4, B1) and LLVM-as-evasion (E1)

*Legitimate need:* the tool must actually run in hardened estates, and the
language must be real. Both are met without evasion: operationally via
signing/allowlisting/declared capabilities (a firing AV is a misconfiguration
fixed in the open); technically via a genuine frontend (lexer/parser/AST with
published EBNF, `wiki/06`, `wiki/grammar.md`) whose IR is the small,
human-reviewable FIR — not LLVM IR, declined for MVP because LLVM-level
tooling is the obfuscation industry's home turf and buys suspicion without
forensic value (`wiki/08` §5.1).

## 5. Regression evidence (this session, 2026-09-14)

Build: `cmake -S . -B /tmp/p10build` EXIT=0, `cmake --build` EXIT=0,
**0 warnings**, g++ 15.2.0, cmake 4.3.4. Tree at `bcd971d` + uncommitted
Phase-10 pre-checkpoint `logs.md` row only (no code drift).

- §5.1 `jocky check samples/sample.jky` EXIT=0, frozen AST shape intact.
- §5.2 `scan_registry stat_scripts/` EXIT=0 → **38 registered / 0 rejected /
  16 skipped** (recon 10, netforensics 6, hostforensics 2, timeline 3,
  compliance 15, report 2).
- §5.3 Fixture patterns: Phase 3 `{valid×2 pass, unknown/missing/extra/
  mismatch refuse}`; Phase 4 `{exact-allow passes, 5 authorization shapes
  refuse}`; Phase 5 `{cycle refuses, direct/chain/diamond/mixed pass}`;
  Phase 5.5 `{call-derived bound refuses, rest pass}`; `ctest` **10/10**
  (4 unit/integration + 6 compiled/interpreted parity).
- §5.4 Manifest integrity: `manifest_version` 0.2.0; `program_sha256`
  `edf89345…f6ced36` **== `sha256sum` of the source file** (recomputed
  live on `tests/phase7.5/for_three.jky`); quarantine baselines unchanged
  (`a5c04271…`, `5d237007…`, `16ae2468…` match `wiki/11` §3).
- §5.5 Control-flow semantics: `if(false)` 0 entries/success; `for-3`
  3 entries/success; `while`-capped 5×success + `while_ceiling`/failed;
  `count()`/`int-let` bounds execute; poison-branch outputs absent.

## 6. What is NOT built (read before scoring)

- **§6.1 `jocky verify` does not exist.** `AGENTS.md` §4 advertises
  `jocky verify <manifest.json>`; the string `verify` appears nowhere in
  `src/` or `include/`. Re-hashing today is manual (`sha256sum` vs
  `program_sha256`, §5.4) plus the parity harness's tamper negative-control
  (tampered `exit_code` → precise FAIL, `wiki/22`). Phase 11 must carry this
  as the top gap: the manifest *format* is verifiable, the *command* is not.
- **§6.2 Only 8 of 38 registry functions pass in-sandbox** (5 timeline/report
  starters + 3 graceful skips); 30 fail for environment reasons — minimal
  sandbox lacks coreutils (`sed`/`head`/`tail`), specialty tools
  (`tshark`, `python3`, `nmap`, `curl`), network egress, and even
  `/dev/null` (all 6 netforensics fail on the redirect first). Full table:
  `wiki/23` §4. Every failure is manifest-logged; none is silent — but no
  judge should be told "38 forensic functions work."
- **§6.3 Windows + Ubuntu is Linux/WSL-only.** No MSVC build, no Windows CI,
  no Windows-native execution; the script corpus is bash. D2 stays partial.
- **§6.4 No central management exists** — no scheduler, case store, client,
  or protocol (refused as networked C2; reframed offline console is Track B
  future). D8 stays a reframe, not a delivery.
- **§6.5 15 scripts await human review** (probe stubs, twin pairs,
  unused-target args, host-intrusive `paxtest`, self-probing privesc triple
  that audits the sandbox instead of evidence) — left headerless deliberately,
  `wiki/23` §2. Plus known gaps: SIGABRT on manifest-outside-output-root
  (`wiki/23` §6), `jky_compliance_audit_drivers` (E3j) not yet written,
  pipeline operators are parse-level (the dispatcher dispatches *calls*; no
  relational engine evaluates `where`/`group_by` at runtime).

## 7. Ten-minute judge verification

```sh
cmake -S . -B /tmp/j && cmake --build /tmp/j -j"$(nproc)"   # clean, 0 warnings
/tmp/j/jocky check samples/sample.jky                        # EXIT=0, AST dump
/tmp/j/scan_registry stat_scripts/ | grep '^SUMMARY 38'      # 38/0/16
ctest --test-dir /tmp/j                                      # 10/10 green
/tmp/j/jocky tests/phase7.5/for_three.jky --registry tests/phase7.5/registry --output-root /tmp/demo
python3 -c "import json;m=json.load(open('/tmp/demo/manifest.json'));print(m['status'],len(m['executions']))"
sha256sum tests/phase7.5/for_three.jky                       # == program_sha256
grep -rn "system(\|popen\|sh -c" src/ include/ | grep -v Binary || echo "no shell dispatch"
grep -rni "verify" src/ include/ || echo "verify unimplemented (§6.1)"
```

## 8. Phase 11 handoff

Input matrix: §2 table above (30 rows, B1–B7/D1–D9/E1–E3j). Carry as gaps:
`jocky verify` (§6.1), Windows build/CI (§6.3), E3j driver-audit function +
15 review scripts (§6.5), sandbox tool provisioning or honest scope marking
(§6.2), manifest-path preflight (§6.5). Enforcement pointers for the
absence claims: `AGENTS.md` §2 (stop-work), `embedder.hpp:47-54`
(drift refusal), `dispatcher.hpp:646-683` (argv-only spawn),
`control_flow_executor.hpp` (ceiling marker), `wiki/11` §3 (quarantine
baselines), `wiki/23` §4 (smoke table) + §6 (SIGABRT gap).
