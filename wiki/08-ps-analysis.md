# SIH26148 Full-Text Analysis

> Companion to `07-research.md`. Where `07` was researched from the
> catalogue title alone, this file works from the **complete problem-
> statement text** (Description + Expected Solution, received 2026-09-12)
> and researches every technical element it names. Read `07` first for
> the landscape, standards, admissibility, and implementation-precedent
> base; nothing there is repeated here except where the full text changes
> the conclusion. No evasion technique below is documented beyond its
> public ATT&CK/vendor-doc behavior — what each mechanism *is*, what
> sensor visibility it removes, and why JOCKY refuses it. There are no
> bypass instructions in this file, by design.

## 1. What the Full Text Adds Beyond the Title

The title promised "scripts/functions" plus "without triggering security
solutions." The body is far more specific — and far more weaponizable.
New facts the title did not disclose:

1. **The framework is pre-named 'JOCKY'** with a **cross-platform
   compiler (Windows & Ubuntu)** requirement. (The project name is
   therefore PS-derived, not invented; the Windows+Ubuntu target becomes
   a harvested requirement in §7.)
2. **The adversary model is enumerated:** behavioral heuristics, static
   signature matching, standard compiler-output fingerprints (MSVC/GCC
   artifacts), API-call-sequence analysis, and kernel-level monitoring.
3. **CI/CD is named as the delivery mechanism** for polymorphism — every
   iteration re-obfuscated, re-encrypted, re-mutated.
4. **Expected Solution 1** demands a custom language/LLVM frontend that
   alters control-flow graphs, token generation, and binary structure to
   defeat signatures.
5. **Expected Solution 2** demands pipeline-driven polymorphism: unique
   hashes, modified entry points, altered import tables per deployment.
6. **Expected Solution 3** demands Living-off-the-Land + BYOVD execution
   explicitly scoped to **persistence, privilege escalation, and SOCKS5
   network routing**, via (A) five named in-memory techniques — process
   hollowing, reflective DLL injection, API unhooking, direct syscalls,
   thread execution hijacking — and (B) kernel subversion via vulnerable
   drivers to disable EDR callbacks and manipulate kernel structures.
7. **A central management interface** for simultaneous multi-system
   analysis, with management↔client traffic routed over trusted cloud /
   CDNs via **domain fronting or legitimate cloud APIs**.

Items 3–7 convert the title's vague "without triggering" into a parts
list for an evasion-first remote-access ecosystem. The rest of this file
takes each part in turn.

## 2. The Background Paragraph, Sentence by Sentence

- *"Modern antivirus solutions restrict proprietary software from
  executing or creating custom scripts"* — the grievance. Partially true
  as stated (application control, script-block logging, AMSI-gated
  scripting hosts do constrain unsigned automation), but the constraint
  is the point of those controls. JOCKY's answer is allowlisting and
  authorization, not subversion: a signed, declared, capability-scoped
  tool that an estate permits deliberately (§6.1 of `07`).
- *"Behavioral heuristics"* — runtime judgment on what code *does*
  (process injection, suspicious memory permissions, abnormal API
  sequences) rather than what it *looks like*. This is the layer every
  technique in §5.4–§5.5 exists to defeat, and the layer JOCKY never
  triggers because dispatched scripts perform only declared forensic
  reads with intact, attributable behavior.
- *"Static signature matching"* — byte-pattern and hash identification.
  Countered in the PS by polymorphism (§5.1, §5.5); preserved by JOCKY
  as its integrity currency (§8.1 of `07`).
- *"Common compiler outputs (like standard MSVC or GCC artifacts)"* —
  build-environment fingerprinting, researched in §5.2 below. Note the
  inversion: defenders fingerprint *attacker* toolchains; the PS wants
  the framework's own binaries to shed such fingerprints. JOCKY takes
  the opposite stance — `jockyc` output should be a boring, standard,
  signed MSVC/GCC binary whose Rich header and import table are stable
  and inspectable.
- *"Typical API call sequences"* — the behavioral anchor for LOL-aware
  detection (§5.3). JOCKY scripts use a small, declared syscall surface
  (read evidence, write `out/`, exit), which is the quietest possible
  sequence without hiding anything.
- *"Kernel-level monitoring to intercept activities"* — the callback /
  ETW-TI / minifilter stack researched in §5.6. The PS frames it as an
  obstacle; JOCKY frames it as infrastructure to preserve: nothing in
  the runtime requires kernel cooperation, so nothing blinds it.
- *"A significant paradigm shift may occur when programmers adopt
  CI/CD"* — the PS's delivery insight, accepted and inverted: JOCKY's
  CI/CD pipeline manufactures *sameness proofs* (parity diffs, §6.4 of
  `07`), not unique-per-build mutants.

## 3. The Description Paragraph, Clause by Clause

- *"'Next-Gen' programming language framework, named as 'JOCKY', using
  cross-platform compiler (windows & ubuntu)"* — **ADOPT.** Name kept;
  Windows+Ubuntu CI becomes a requirement (§7). C++20/CMake already
  satisfies it; the parity harness must run on both.
- *"Systematic creation of scripts for analyzing malicious activities
  and also provide the complete digital forensics of the computer or
  network"* — **ADOPT.** This is the registry + report domains, verbatim
  the legitimate core.
- *"The framework will not be hindered by any of the existing
  anti-virus"* — **REFRAME.** Hindered-by-none is evasion; JOCKY's
  equivalent is *impeded-by-none-because-authorized*: signed binaries,
  declared capabilities, allowlisted paths, logged execution. A deployment
  where the AV fires on JOCKY is a deployment misconfiguration to fix in
  the open, not a detection to engineer around in secret.
- *"Automated polymorphic engines, custom encryption, and multi-vector
  in-memory execution via native components or BYOVD"* — **REFUSE**
  (§5.1, §5.4, §5.5, §8.1–§8.3 of `07`). Each noun is researched below
  precisely so the refusal cites mechanism, not squeamishness.
- *"Handle multiple system analysis simultaneously using central
  management interface"* — **REFRAME** as an offline multi-case console:
  many evidence bundles triaged from one analyst workstation, results
  correlated across cases (the persistent-store open question), with the
  Mythic-style *audit log* kept and the Mythic-style *network* deleted
  (§5.7).
- *"Traffic routed through trusted cloud infrastructure or CDNs using
  domain fronting or legitimate cloud APIs"* — **REFUSE** (§5.8, §8.4 of
  `07`). No management↔client protocol exists to route anywhere.

## 4. The Expected Solution, Requirement by Requirement

### 4.1 ES-1: custom language / LLVM frontend altering CFGs, tokens, binaries

The PS wants frontend ownership *as an evasion primitive*. JOCKY takes
frontend ownership *as a language primitive* and declines the evasion
half: the lexer/parser/AST exist so investigations are typed and
checkable (Phase 1, §9.1 of `07`), not so binaries dodge signatures.
"Token generation" altered for stealth is refused; token definitions
published in EBNF (`06-api-contracts.md`) is adopted. On LLVM
specifically: JOCKY's IR is the FIR, not LLVM IR, and no LLVM dependency
enters the MVP — researched justification in §5.1 (LLVM-level tooling is
the obfuscation industry's home turf; importing it buys complexity and
suspicion, not forensic value).

### 4.2 ES-2: CI/CD-driven polymorphism (unique hashes, entry points, import tables)

**REFUSE** in full, with the reframe that CI/CD still enters the project
— as the parity/smoke-test pipeline (§6.4 of `07`). Per-deployment
uniqueness of hashes is the exact negation of the manifest's promise;
modified entry points and shuffled import tables are triage-evasion
against imphash/Rich-header analysis (§5.2, §5.5). The compliance-domain
mirror is adopted instead: JOCKY *detects* packing/polymorphism-shaped
artifacts in evidence (entropy + section anomalies + Rich-header
inconsistencies) without ever *producing* them.

### 4.3 ES-3A: the five in-memory techniques

| Named technique | ATT&CK | Disposition |
| --------------- | ------ | ----------- |
| Process hollowing | T1055.012 | Refuse (§8.2 of `07`) |
| Reflective DLL injection | T1055.001 family | Refuse (§5.4) |
| API unhooking | T1562.001 (impair defenses) | Refuse (§5.4/§5.6) |
| Direct system calls | Defense-evasion primitive | Refuse (§5.4/§5.6) |
| Thread execution hijacking | T1055.003 | Refuse (§5.4) |

All five share one goal — executing while user-mode sensors (hooks) and
kernel-mode telemetry (ETW-TI's LOCAL/REMOTE memory/thread tasks) watch
elsewhere — and one conflict with JOCKY: unattributable execution cannot
produce a manifest entry naming what ran. The dispatcher runs scripts as
ordinary child processes instead; §5.4 documents the refused mechanics at
the level needed to defend the decision in Q&A.

### 4.4 ES-3B: BYOVD kernel subversion + persistence / priv-esc / SOCKS5 goals

**REFUSE** the mechanism (§8.3 of `07`); refuse the *goals* louder, since
goals matter more than mechanisms: **JOCKY has no persistence mechanism,
no privilege-escalation path, and no network routing of any kind**
(including SOCKS5, which the C2 literature confirms as the standard
implant pivot transport — Sliver ships `socks5 start` out of the box).
It runs with the analyst's existing privileges, touches only declared
evidence, writes only `out/`, and exits. The one adoptable shard is the
*detection* of BYOVD exposure: checking installed drivers against
Microsoft's vulnerable-driver blocklist as a `jky_compliance_*`
function (§5.9) — auditing the exact risk the PS wanted to weaponize.

## 5. Topic Research Capsules (New Material)

### 5.1 LLVM as an obfuscation substrate — why the FIR is not LLVM IR

- **Obfuscator-LLVM** (Junod et al., 2015) is the reference
  implementation of exactly what ES-1 asks for: passes over LLVM IR
  performing instruction substitution (semantically equivalent opcode
  swaps), bogus control flow (cloned blocks guarded by opaque
  predicates), and **control-flow flattening** (hierarchy destroyed,
  blocks dispatched through a `switch` loop — `-mllvm -fla`), plus
  procedure merging and checksum-based tamper-proofing. Follow-on work
  (**iOLLVM**) adds nested-switch flattening, bogus-block indegree
  obfuscation, and identifier obfuscation — and the counter-literature
  (IDA-script de-flatteners, indegree analysis, BinDiff similarity
  scoring, and now LLM-based deobfuscation with a measured resistance
  ladder: BCF < FLA < SUB/combined) proves this is a mature arms race,
  not a green field.
- Implication for JOCKY: adopting LLVM as the compilation substrate
  would drag that arms race — and its signature profile — into a
  forensic tool for zero forensic gain. The FIR stays a small,
  JSON-serializable, human-reviewable plan; `jockyc` stays a
  straightforward C++20 binary. LLVM is recorded as **declined for MVP**,
  revisit only if a future backend (e.g., eBPF-free query JIT) needs it
  for performance rather than stealth.

### 5.2 Compiler-output fingerprinting — why `jockyc` binaries stay boring

- **Rich header** (Microsoft linker metadata: per-tool product/version/
  count tuples, XOR-masked after the DOS stub): present in ~71% of
  malicious PEs in one SANS-corpus study; basis for Rich/RichPV
  similarity hashes that beat ssdeep/imphash at family clustering in
  evaluation (350 samples, zero cross-family false positives), scale to
  ~1M-sample triage in milliseconds (TUM study, 964,816 samples), and
  detect post-build tampering via header anomalies. It is forgeable
  (2018 Lazarus→Olympic-Destroyer false flag copied a Rich header) and
  absent from non-MSVC toolchains — both facts the PS's ES-2 exploits
  and JOCKY's compliance checks must account for.
- **imphash** (Mandiant, 2013/14): MD5 over ordered, lowercased,
  extension-stripped `lib:func` import lists; IAT order follows source
  order and link order, so equal imphash implies common build lineage
  (APT1: 356 samples → 11 family-covering hashes). Consumable via
  `pe.imphash()` in YARA, searchable on VirusTotal/MalwareBazaar,
  limited on packed/small-import samples — Mandiant's own caveat, and
  precisely the limitation ES-2's "altered import tables" targets.
- JOCKY posture, both directions: `jockyc` output keeps a stable Rich
  header and import table across builds (reproducibility aids
  allowlisting and parity); evidence-side, `jky_recon_*`/`jky_compliance_*`
  functions should compute imphash/RichPV over triaged PEs as standard
  pivots — the defensive use of the exact signals ES-2 tries to strip.

### 5.3 Living-off-the-Land — detect the technique, never embody it

- The **LOLBAS project** (241 tracked Windows binaries/scripts/libraries
  plus GTFOBins for Unix and loldrivers.io for drivers) catalogues only
  Microsoft-signed files with *unexpected* capability useful to an
  operator: proxy execution, code compile, download/upload, persistence,
  UAC bypass, credential theft, memory dumping, surveillance, log
  evasion. ATT&CK's **T1218** (System Binary Proxy Execution, incl.
  T1218.011 `Advpack.dll`) is the technique head; Volt Typhoon and
  Lazarus are cited enterprise-scale users.
- The distinction that saves JOCKY's scope: LOLBin *detection content*
  (Sigma rules flagging `msbuild.exe -*` oddities, parent/child
  anomalies) is core host-forensics material for `rule_decl` libraries;
  LOLBin *execution* (proxying JOCKY's own actions through signed
  binaries) is refused — it exists to inherit trust the actor hasn't
  earned, which is incompatible with capability-gated, manifest-logged
  operation. Note for Phase 9: several Hayabusa/Chainsaw Sigma rules are
  LOLBin-shaped, so the eventlog adapter ingests this coverage for free.

### 5.4 The in-memory execution family — refused mechanics, kept signals

- **Reflective DLL injection** (Stephen Fewer, 2011; Metasploit's
  workhorse via `reflective_dll_inject` and the Meterpreter RDI loader):
  a self-loading `ReflectiveLoader` export re-implements the OS loader
  in memory — locate own image via backward `MZ` scan, resolve
  `LoadLibraryA`/`GetProcAddress`/`VirtualAlloc` by walking the PEB
  module list, allocate `MEM_PRIVATE` (typically RWX), copy
  headers/sections, resolve imports, apply relocations, call `DllMain` —
  never touching `LdrLoadDll`, hence never appearing in the PEB module
  list. Detection signals fall out directly: executable `MEM_PRIVATE`
  with no backing file, the PEB/memory-view gap, RWX permissions; sRDI
  later removed the source-code and entry-point-only limitations.
- **The family shares primitives**, which is why one refusal covers
  five techniques: remote allocation/write/thread-creation
  (`VirtualAllocEx`/`WriteProcessMemory`/`CreateRemoteThread`),
  context/APC manipulation (`SetThreadContext`, `QueueUserAPC` — the
  thread-hijacking pair), and hook removal (API unhooking) plus direct
  syscalls to dodge user-mode sensor hooks. Windows 10 1809 answered
  with the **ETW Threat Intelligence** feed (`Microsoft-Windows-
  Threat-Intelligence`, PPL-Antimalware-gated): 14 LOCAL/REMOTE tasks
  over AllocVM/ProtectVM/MapView/QueueUserAPC/SetThreadContext/ReadVM/
  WriteVM — the exact visibility layer each refused technique is
  designed to evade.
- JOCKY keeps the *signals* as detection content (`MEM_PRIVATE`-RX
  regions, PEB-gap indicators, RWX findings feed the `indicator` entity
  and report domain) and refuses the *mechanics*: no remote allocation,
  no cross-process writes, no loader re-implementation, no hook removal,
  no raw syscalls in the runtime — all process execution goes through
  ordinary, hook-visible child processes.

### 5.5 Packers, cryptors, entry points, import tables — ES-2's toolkit

- "Custom encryption" in this context means cryptors/packers: encrypted
  payload + decryptor stub, variable keys per build — the static form of
  the polymorphic engines refused in §8.1 of `07`. "Modified entry
  points" (TLS callbacks, shifted `AddressOfEntryPoint`, stolen bytes)
  and "altered import tables" (reordered/minimized IATs, dynamic API
  resolution) are anti-triage measures aimed squarely at §5.2's pivots.
- Mandiant's imphash writeup concedes the fragility honestly (reorder
  source or link order and the hash changes) — the PS industrializes
  that concession into CI/CD. JOCKY's compliance answer is layered
  detection (entropy + section-permission anomalies + Rich-header
  inconsistency + imphash deviation *together*, never one signal), per
  YARA production-rule practice in §4.5 of `07`.

### 5.6 EDR internals — what the refused techniques remove

- **Kernel callbacks:** drivers register process/thread/image/registry
  notification routines (`PsSetCreateProcessNotifyRoutine` and Ex/Ex2
  variants, up to 64 process-creation slots; Defender's `WdFilter`
  visible in the `PspCreateProcessNotifyRoutine` array in published
  analyses). Callback removal or higher-altitude hijack requires kernel
  code — i.e., BYOVD — which is why ES-3B pairs the two asks.
- **ETW-TI** (§5.4), **AMSI** (`AmsiScanBuffer` script/content scanning
  with hash-verified integrity checks in published sensor designs),
  **minifilters** (file I/O visibility): together the standard sensor
  stack. Published evasion-detection catalogs monitor exactly this
  surface — callback health polling, ETW/AMSI patch detection,
  handle-stripping and log-tamper checks — confirming the industry
  treats sensor integrity as the asset and its subversion as the attack.
- JOCKY's relationship to this stack is *zero-touch*: no driver, no
  callback registration or removal, no ETW provider manipulation, no
  AMSI interaction beyond what the OS does for any normal process.
  Forensic reads via documented user-mode APIs keep every sensor's view
  intact — the literal opposite of "will not be hindered."

### 5.7 C2 architecture — what a "central management interface" is refused to be

- Every modern framework reduces to **team server + listeners +
  implants**, with tasking queued server-side and fetched by
  interval-jittered beacons (async) or held sessions (interactive):
  Cobalt Strike (commercial; Beacon over HTTP(S)/DNS/SMB/TCP with
  **malleable C2 profiles** — `c2lint`-validated wire-shape configs that
  mimic jQuery CDNs or Office365 — plus Artifact Kit, Aggressor Script,
  BOFs; MITRE S0154), **Sliver** (open-source Go; mTLS/WireGuard/HTTP/
  DNS, per-binary keys, Armory modules, built-in `socks5` pivoting;
  S0633), **Mythic** (open-source plugin C2: Dockerized server +
  per-agent/per-profile containers — Apollo, Poseidon, etc. — with C2
  profiles spanning `http`/`smb`/`dns` to **`discord` and `github`**,
  i.e., the PS's "legitimate cloud APIs" verbatim), **Havoc**
  (open-source; Ekko sleep, indirect syscalls baked in). Redirectors
  (rewrite-fronted Nginx/Apache, or CDN fronting) hide the team server;
  JARM/JA3 fingerprinting, beacon-timing analysis, and memory scanning
  catch stock deployments.
- Adopted shards: the *collaboration/audit* half — Mythic's per-operator
  action history exported for the report is the model for JOCKY's
  multi-analyst case console (who ran what, when, against which
  evidence), and per-case isolation replaces per-implant keying.
  Refused: listeners, implants, tasking-over-wire, redirectors, profiles
  — there is no JOCKY component that accepts inbound connections or
  emits callbacks, so malleable-shape questions never arise.

### 5.8 Legitimate-cloud-API C2 — the PS's routing clause, mapped

- **T1102** covers the clause exactly: dead-drop resolvers (T1102.001 —
  APT41 via GitHub/Pastebin/TechNet, MiniDuke via Twitter, PlugX via
  Pastebin, Astaroth via AWS/Cloudflare/YouTube/Facebook) and
  bidirectional web-service C2 (T1102.002 — POLONIUM's parallel
  OneDrive+Dropbox channels, APT37's multi-cloud spread, RIFLESPINE's
  Google Drive command files, PowerStallion's `net use`-mapped OneDrive,
  HAMMERTOSS's algorithmic Twitter handles). Advantages documented for
  operators — blends with enterprise traffic, TLS by default, rarely
  blocked, CDN-resilient — are the PS's stated motives verbatim.
- Defenders answer with polling-shape detection (fixed intervals,
  consistent sizes, non-browser processes, programmatic user-agents),
  CASB API inspection, and OAuth-app/token auditing — all of which are
  *network/log* detection content JOCKY can express (`dns_event` +
  `flow` predicates over resolver/API domains) while refusing to
  *implement* either side of the channel.

### 5.9 The blocklist as compliance data — BYOVD inverted

- Microsoft publishes **recommended driver block rules** (quarterly
  refresh, `SiPolicy.p7b` via App Control, enforced with HVCI/S-mode/
  Smart App Control, on by default since Windows 11 22H2, auditable via
  CodeIntegrity event 3099), plus an ASR rule blocking vulnerable-driver
  *writes to disk* — the ecosystem's immune response to the Qilin/
  Warlock industrialization documented in `07` §4.3.
- This converts ES-3B's weapon into JOCKY's highest-value compliance
  check: `jky_compliance_audit_drivers` enumerating installed third-party
  drivers and joining against the pinned blocklist, emitting dated,
  manifest-logged findings. Detecting BYOVD exposure is authorized
  defensive work; *being* the BYOVD loader is refused. The function
  belongs in the Phase 9 registry bootstrap's first tranche.

## 6. Draft Gap-Check Matrix (Input to Phase 11)

Disposition key: **ADOPT** (build as asked) · **REFRAME** (build the
legitimate need behind the ask) · **REFUSE** (permanent boundary, with
the section justifying it). Verification names the artifact a judge can
inspect.

| # | PS clause | Disposition | JOCKY mechanism | Verification |
| - | --------- | ----------- | --------------- | ------------ |
| B1 | AV constrains script execution | REFRAME | Signed, allowlisted, capability-scoped tooling | Case authorization + manifest |
| B2 | Behavioral heuristics | PRESERVE-FOR-DEFENDERS | Declarative forensic reads only; no injection/memory ops | Dispatcher audit (§9.2 of `07`) |
| B3 | Static signatures | PRESERVE (as integrity) | Stable hashes everywhere; `verify` | Matching parity manifests |
| B4 | Compiler-output fingerprints | INVERT | Boring stable MSVC/GCC builds; Rich/imphash as *detection* pivots | Reproducible builds; §5.2 |
| B5 | API-sequence analysis | SATISFY-BY-CONSTRUCTION | Minimal declared syscall surface | `--list-used` + manifest args |
| B6 | Kernel monitoring | PRESERVE | Zero kernel touch; user-space only | No driver, no callbacks (code audit) |
| B7 | CI/CD paradigm | REFRAME | CI/CD proves sameness (parity/smoke) | Pipeline logs + parity diffs |
| D1 | Framework named JOCKY | ADOPT | Project identity | AGENTS.md §1 |
| D2 | Cross-platform Win+Ubuntu compiler | ADOPT | C++20/CMake; dual-OS CI | §7; build matrix in Phase 1 |
| D3 | Systematic script creation + full forensics | ADOPT | Registry + adapters + report | Smoke harness (Phase 9) |
| D4 | "Not hindered by any AV" | REFRAME | Authorized/allowlisted operation | Authorization model (Phase 4) |
| D5 | Polymorphic engines | REFUSE | Verbatim embedding + hashes | §8.1 of `07`, §5.1/§5.5 |
| D6 | Custom encryption (cryptors) | REFUSE | No packers in toolchain or stdlib | Build audit; §5.5 |
| D7 | In-memory via BYOVD | REFUSE | Subprocess dispatch; user-space only | §5.4, §8.2–§8.3 of `07` |
| D8 | Central management interface | REFRAME | Offline multi-case console + audit log | §5.7; stretch dashboard |
| D9 | CDN/domain-front/cloud-API routing | REFUSE | No network surface at all | §5.8, §8.4 of `07`; socket audit |
| E1 | Custom language/LLVM frontend for evasion | SPLIT | Adopt frontend-as-language; refuse evasion; decline LLVM for MVP | Grammar EBNF; §5.1, §4.1 |
| E2 | Per-deploy unique hashes/entry points/imports | REFUSE | Stable reproducible builds | Rebuild-hash equality |
| E3a | Process hollowing | REFUSE | — | T1055.012; §8.2 of `07` |
| E3b | Reflective DLL injection | REFUSE | — | §5.4 |
| E3c | API unhooking | REFUSE | — | §5.4, §5.6 |
| E3d | Direct syscalls (for evasion) | REFUSE | Ordinary documented APIs only | Syscall review; §5.6 |
| E3e | Thread execution hijacking | REFUSE | — | T1055.003; §5.4 |
| E3f | Persistence mechanisms | REFUSE | None exist | Feature audit |
| E3g | Privilege escalation | REFUSE | Analyst's existing privileges only | No elevated primitives in code |
| E3h | SOCKS5 routing | REFUSE | No sockets to route | §5.7–§5.8; socket audit |
| E3i | BYOVD kernel subversion | REFUSE | User-space only | §8.3 of `07`, §5.6 |
| E3j | Driver-exposure auditing (inverted shard of E3i) | ADOPT | `jky_compliance_audit_drivers` vs MS blocklist | §5.9; Phase 9 tranche 1 |

## 7. Harvested Requirement Changes

1. **Cross-platform Windows + Ubuntu** is now an explicit PS requirement:
   Phase 1 build matrix covers MSVC + GCC from day one; parity harness
   runs per-OS (hashes may differ across OS builds — parity is
   same-OS-compiled-vs-interpreted, recorded here to prevent a false
   Phase 8 failure).
2. **CI/CD is adopted as the sameness pipeline** (parity + smoke on
   every commit), not a mutation pipeline.
3. **LLVM is declined for MVP** (§5.1); revisit trigger defined
   (performance-driven backend need, never stealth).
4. **Driver-blocklist auditing** joins the Phase 9 first tranche (§5.9).
5. **LOLBin-shaped Sigma content** ships with the eventlog path by
   default via converter rule packs (§5.3).
6. **Compliance checks for packing/timestomping/Rich-anomalies** join
   the registry bootstrap (§4.2, §5.4 of `07`, §5.2/§5.5 here).
7. **Multi-case offline console** (operators' audit-log half of §5.7)
   becomes the named form of the stretch dashboard's case layer.

## 9. Late-Closing Research: D3FEND, CERT-In, DPDP

Three stones identified as unturned in the "any research left" audit and
closed in the same session:

### 9.1 D3FEND — the defensive taxonomy for detection content

- MITRE's **D3FEND** knowledge graph catalogues countermeasures the way
  ATT&CK catalogues offenses, with published ATT&CK-mitigation mappings
  and NIST 800-53 crosswalks. Directly relevant techniques: **D3-FA
  File Analysis** (subclasses: dynamic analysis, emulated analysis,
  **file hashing D3-FH**, **file content rules D3-FCR**, content
  analysis) and **D3-NTA Network Traffic Analysis** (21 subclasses incl.
  signature analysis and file carving); M1020 (SSL/TLS inspection) maps
  to D3-NTA as supporting infrastructure.
- JOCKY use, recorded as a Phase 9 registry work item: tag stdlib
  functions with **D3FEND IDs alongside ATT&CK tags** (extending the
  YARA/Sigma `meta` convention from `07` §4.5) — host/file triage under
  D3-FA, network functions under D3-NTA, compliance checks against the
  Harden branch. Every refused offensive technique in §6 then has a
  named defensive counterpart the tool *does* implement, which is the
  strongest possible Q&A posture: refusal paired with coverage.

### 9.2 CERT-In — the auditor regime JOCKY deployments live under

- CERT-In is the Section 70B IT Act nodal agency (collection/analysis/
  dissemination of incident information, alerts, emergency measures,
  coordination, guidelines). It **empanels auditing organisations** for
  VA/PT work, and its Comprehensive Audit Guidelines (v1.0, 2025) plus
  empanelment terms are binding: written consent and defined scope, NDAs,
  **audit data encrypted, stored only in India, never mirrored abroad,
  wiped unrecoverably after engagement**, audit reports + metadata to
  CERT-In within 5 days (including unfixed criticals), and termination —
  including for *unconsented* security testing — for violations. Its
  2024 audit-practice recommendations require **audit artefacts (hash
  values, versions, timestamps) captured in the audit certificate and
  reports**, comprehensive (not top-10) coverage per ISO/OSSTMM/OWASP
  WSTG, and highest-standard comprehensive reporting.
- Five consequences for JOCKY:
  1. The manifest is the artefact set CERT-In already demands — say so
     in Phase 10 explicitly.
  2. Report/manifest handling must follow auditor-data rules (encrypt,
     India-only, wipe) — deployment guidance owned by the report domain.
  3. Written-consent scoping is an empanelment condition, so the case
     authorization model (Phase 4) is regulatory alignment, not ethos.
  4. The 5-day submission + repeat-observation loop matches the retest
     model in `07` §4.6 — manifests must support follow-up audits.
  5. CERT-In itself lists evidence collection and cyber forensics among
     its functions: JOCKY is positioned as compatible analyst tooling
     for that ecosystem.

### 9.3 DPDP Act + the 6-hour regime — the clock that justifies speed

- Two parallel Indian breach duties: **CERT-In Directions (28 Apr 2022,
  operative now)** — Annexure-I incidents including breach/leak reported
  within **6 hours** on available facts (supplement later), 180-day
  India-resident log retention; and **DPDP Act 2023 §8(6) + 2025 Rules
  r.7 (from 13 May 2027)** — Board intimation without delay, detailed
  report within 72 hours, **every affected Data Principal notified
  without delay with no GDPR-style risk threshold**, penalties to
  ₹250cr (safeguards) / ₹200cr (notification). Forensic guidance for the
  regime stresses preservation first (chain-of-custody record,
  pre-change-state logging for live actions) and clock-start at
  *awareness*, not root-cause confirmation.
- Four consequences for JOCKY:
  1. The 6-hour initial-report pressure is the performance requirement
     behind fast, targeted triage — a demo-narrative point with a
     statutory citation.
  2. Breach-scope questions (which records, how many principals, CIA
     impact) are compliance-domain queries over `log_event`/`file` —
    先列 here so Phase 9 fixtures include a breach-scope scenario.
  3. Personal-data minimization in reports/timelines becomes a
     report-domain handling rule (with §9.2's storage/wipe discipline).
  4. Processor-escalation timelines mean the manifest's `authorization`
     block should record *whose authority* each run exercises.

## 10. Residual Backlog — What's Left, Honestly Graded

| # | Item | Status | Owner |
| - | ---- | ------ | ----- |
| R1 | Blockchain-theme non-requirement | CLOSED by reasoning: the PS text contains zero blockchain requirements — Blockchain & Cybersecurity is the theme bucket, not a deliverable. Decision recorded: **no DLT component**; manifest hash-chaining must never be marketed as blockchain (judges punish buzzword-cramming per `07` §3). | Phase 10 (state once, move on) |
| R2 | D3FEND / CERT-In / DPDP | CLOSED this session (§9). | Phase 9–10 work items as noted |
| R3 | Embedding + tree-shaking precedents | CLOSED this session: Go `//go:embed` (compile-time, read-only FS), ELF-section/objcopy-class approaches, codegen-then-compile tooling, linker `-X` metadata injection, GC-toolchain dead-code handling — the C++ analogs (objcopy `--input binary`, `xxd -i`, linker scripts via CMake custom commands) inherit the mechanism. Forensic addition (embed-time hashes) remains ours alone. | Phase 5–6 |
| R4 | Registry script hygiene | CLOSED this session: **ShellCheck** (GPLv3, CI-integrable exit codes, dialect/portability + dataflow checks; mandated by Google's shell styleguide) becomes the registry admission gate alongside `@jocky:` header validation — shellcheck-clean in a pinned dialect as Phase 2/9 criteria. | Phase 2, 9 |
| R5 | Cross-platform parity hazards | OPEN design note (no search required): CRLF/LF normalization before hashing embedded scripts, path-separator and case-sensitivity rules for manifest paths; parity scope stays same-OS (already `§7.1`). | Phase 1, 8 |
| R6 | Sample-evidence corpora | PARTIALLY OPEN: Hayabusa Sample EVTXs + EVTX-Attack-Samples verified (`07` research); pcap fixture corpora (Wireshark samples, CIC-IDS2017-class sets) unverified. | Phase 9 |
| R7 | T1055.003 / T1027.002 dedicated pages | MINOR OPEN: covered via family pages + ETW primitive names; dedicated reads deferred to Phase 10 prep. | Phase 10 |
| R8 | Sigma v2 correlation / Timesketch analyzer specifics for `correlate` growth | MINOR OPEN: deferred post-MVP by design (`07` §4.4). | Post-MVP |
| R9 | Bypass how-tos, live-C2 operation, cryptor construction | DELIBERATELY UNEXPLORED and to remain so: outcome-level ATT&CK/vendor knowledge suffices for detection content and Q&A; implementation detail would cross the refusal boundary. | Never |

## 11. Sources (New in This File; Shared Base in `07` §11)

**LLVM obfuscation**
- Obfuscator-LLVM (Junod et al., 2015) —
  `https://www.semanticscholar.org/paper/Obfuscator-LLVM-Software-Protection-for-the-Masses-Junod-Rinaldini/b529562ca19d954b2e1d46b3b3660a7e89edd1c1`
- iOLLVM enhancements (Li et al., 2022) —
  `https://aircconline.com/csit/papers/vol12/csit120409.pdf` and
  `https://arxiv.org/pdf/2203.03169`
- Control-flow-flattening pass docs (obfuscator-llvm wiki) —
  `https://github.com/obfuscator-llvm/obfuscator/wiki/Control-Flow-Flattening`

**Compiler-output fingerprinting**
- Rich-header static detection/linking (SANS/GREM, Corcoran) —
  `https://www.giac.org/paper/grem/6321/leveraging-pe-rich-header-static-malware-etection-linking/169729`
- Rich-header triage at scale (TUM, 964,816 samples) —
  `https://www.insec.cit.tum.de/i20/publications/finding-the-needle-a-study-of-the-pe32-rich-header-and-respective-malware-triage`
- RichPE metadata hash + spoof check —
  `https://github.com/RichHeaderResearch/RichPE`
- imphash (Mandiant/Google Cloud, 2014) —
  `https://cloud.google.com/blog/topics/threat-intelligence/tracking-malware-import-hashing/`
- PE-metadata pivoting incl. false-flag caveat (SSTIC 2021, Lunghi)

**Living-off-the-land**
- LOLBAS project — `https://lolbas-project.github.io/`
- LOLBAS corpus (GitHub) — `https://github.com/lolbas-project/lolbas`
- T1218 System Binary Proxy Execution —
  `https://attack.mitre.org/techniques/T1218/`

**In-memory execution**
- ReflectiveDLLInjection (Fewer) —
  `https://github.com/stephenfewer/ReflectiveDLLInjection`
- Meterpreter RDI internals (redforge, 2026-08-27) —
  `https://dev.to/redforge/meterpreter-internals-how-reflective-dll-injection-actually-works-2ccd`
- Metasploit `reflective_dll_inject` module + Meterpreter RDI loader
  (Rapid7) — `https://github.com/rapid7/metasploit-framework/blob/master/modules/post/windows/manage/reflective_dll_inject.rb`
- Payload-life / sRDI notes (attl4s) —
  `https://attl4s.github.io/assets/pdf/Understanding_a_Payloads_Life.pdf`
- ETW-based injection detection (redbluepurpl — callbacks, ETW-TI tasks,
  AMSI) —
  `https://blog.redbluepurpl.com/windows-security-research/kernel-tracing-injection-detection`

**EDR sensor stack**
- `PsSetCreateProcessNotifyRoutine` (Microsoft Learn) —
  `https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-pssetcreateprocessnotifyroutine`
- `PsSetCreateProcessNotifyRoutineEx2` (Microsoft Learn)
- Callback-abuse demonstration, outcome level (Altered Security,
  2024-06-20) —
  `https://www.alteredsecurity.com/post/when-the-hunter-becomes-the-hunted-using-custom-callbacks-to-disable-edrs`
- Evasion-detection catalog incl. callback health + patch checks
  (AuroraSOC, 2026-06-27) —
  `https://docs.aurorasoc.ahmeddwalid.me/docs/dev/edr-windows/evasion-detection`

**C2 architecture**
- C2 frameworks compared: Cobalt Strike/Sliver/Mythic (yunolay,
  2025-10-20) — `https://yunolay.com/c2-frameworks-explained/`
- Cobalt Strike/Havoc/Sliver operator comparison (genxcyber, 2026-07-14)
- Malleable C2 official docs (Fortra) —
  `https://hstechdocs.helpsystems.com/manuals/cobaltstrike/current/userguide/content/topics/malleable-c2_main.htm`
- Mythic plugin architecture (UncleSp1d3r, 2023, current to 2026) —
  `https://unclesp1d3r.github.io/posts/2023-04-22-mythic/`
- T1102.001 Dead Drop Resolver — `https://attack.mitre.org/techniques/T1102/001/`
- T1102.002 bidirectional cloud C2 — patterns per startupdefense.io
  technique survey
- T1102 parent — `https://attack.mitre.org/techniques/T1102/`

**Driver blocklist (compliance data)**
- Microsoft recommended driver block rules (Learn) —
  `https://learn.microsoft.com/en-us/windows/security/application-security/application-control/windows-defender-application-control/design/microsoft-recommended-driver-block-rules`
- Tamper resiliency / ASR vulnerable-driver rule (Defender docs) —
  `https://learn.microsoft.com/en-us/defender-endpoint/tamper-resiliency`
- Blocklist servicing (KB5020779, Microsoft Support)

**Defensive taxonomy, audit regime, breach law (§9)**
- D3FEND matrix — `https://d3fend.mitre.org/`
- D3-NTA Network Traffic Analysis — `https://d3fend.mitre.org/technique/d3f:NetworkTrafficAnalysis/`
- D3-FA File Analysis — `https://d3fend.mitre.org/technique/d3f:FileAnalysis/`
- ATT&CK-mitigations-to-D3FEND mappings —
  `https://d3fend.mitre.org/mappings/attack-mitigations/`
- CERT-In comprehensive audit policy guidelines (v1.0, 2025-07-25) —
  `https://www.cert-in.org.in/PDF/Comprehensive_Cyber_Security_Audit_Policy_Guidelines.pdf`
- CERT-In empanelled-auditor guidelines (v3.0) —
  `https://www.cert-in.org.in/PDF/Auditor_Guidelines.pdf`
- CERT-In empanelment terms and conditions —
  `https://www.cert-in.org.in/PDF/termscon.pdf`
- CERT-In audit-ecosystem recommendations (2024-10-01, artefacts in
  audit certificates) — `https://www.cert-in.org.in/` (CurrentActivity
  CICA-2024-3329)
- DPDP Act, 2023 (MeitY) — breach definition §2(u), duties §8,
  penalties Schedule
- DPDP breach-notification mechanics, §8(6)/r.7 dual track (2026-08-20)
- DPDP vs CERT-In vs GDPR comparison (Mondaq, 2026-01-02; ruleexpert,
  2026-08-24; myitmanager, 2026-06-24)

**Registry hygiene + embedding verification (§10 R3–R4)**
- ShellCheck (koalaman) — `https://github.com/koalaman/Shellcheck`
- Google shell styleguide, ShellCheck mandate —
  `https://github.com/google/styleguide/blob/5b8bc6b9477c0a96aea4dfebe8f91c0c2cbac67e/shellguide.md`
- Go `embed` package docs — `https://pkg.go.dev/embed`
- Go toolchain data-bundling tricks (syso/`.incbin`, `-X` link flags) —
  `https://go.dev/wiki/GcToolchainTricks`
- Binary-embedding approaches survey incl. ELF sections (itaysk, 2020) —
  `http://blog.itaysk.com/2020/04/30/embedding-files-in-binaries`
