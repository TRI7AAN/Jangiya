# JOCKY Research Notes

> Evidence base behind the design decisions recorded in `01-prd.md`
> through `06-api-contracts.md`. Two research passes, 2026-09-11 to
> 2026-09-12, via public sources cited in §11. This file explains *why*
> JOCKY is shaped the way it is: what the problem statement literally
> says, who the customer is, how the contest is scored, what the existing
> tool landscape already covers, which evidence formats ground the entity
> schema, which standards govern integrity, what Indian law requires for
> admissibility, why each excluded component is excluded on technical —
> not just ethical — grounds, and which implementation precedents each
> phase builds on. Judges and future sessions should read this alongside
> `01-prd.md`.

## 1. Problem-Statement Provenance

- **Exact title:** "Creation of scripts/functions with new programming
  language to commence Computer & Network forensic analysis without
  triggering security solutions." **ID SIH26148**, organization **National
  Technical Research Organisation (NTRO)**, theme **Blockchain &
  Cybersecurity**, Smart India Hackathon 2026. The title, ID, org, and
  theme are confirmed across the SIH 2026 catalogue mirrors (NTRO
  organisation page, Blockchain & Cybersecurity theme page, the
  community problem-statement repository, and the 226-statement master
  catalogue).
- **NTRO forensics cluster:** the master catalogue lists neighboring NTRO
  software statements — SIH26147 (`.IQ`/`.wav` signal-parameter analysis),
  SIH26149 (secure data erasure + file recovery for forensics), SIH26150
  (multi-vendor DVR/NVR forensic acquisition). JOCKY therefore sits inside
  a coherent NTRO forensics cluster, and its scope (computer & network
  forensic analysis expressed as scripts/functions in a new language) is
  the defensible core shared with its sibling statements.
- **Phrase-by-phrase reading that drives the design:**
  - *"new programming language"* → a real DSL (`.jky`) with a lexer,
    parser, AST, and grammar — not a script folder with a README.
  - *"scripts/functions"* → the stdlib-as-registry model: the existing
    Kalki/Trinetra corpus promoted to typed, metadata-headed callable
    functions invoked via `call`.
  - *"Computer & Network forensic analysis"* → the domain taxonomy
    (`netforensics`, `hostforensics`, `timeline`, plus `recon`,
    `compliance`, `report`) and the three MVP adapters (pcap, eventlog,
    directory).
  - *"without triggering security solutions"* → the dangerous phrase. Read
    literally it specifies defense evasion, which is why `01-prd.md`
    reframes it: the legitimate need underneath is *authorized,
    low-footprint, declared-and-logged* analysis (allowlisted tooling that
    a defender's stack permits on purpose), never *covert* analysis that
    defeats the stack. Everything in §8 below exists to justify that
    reframing technically.

## 2. The Customer: NTRO — Why an Auditable Tool Fits

- **What NTRO is:** India's technical-intelligence (TECHINT) agency, set
  up in 2004, reporting to the National Security Advisor and the Prime
  Minister's Office, modelled on the NSA. Its disciplines include SIGINT,
  imagery/geospatial intelligence, cyber security, cryptology, and
  strategic hardware/software development; it houses the National Critical
  Information Infrastructure Protection Centre (NCIIPC, the national nodal
  agency for CII protection under Section 70A of the IT Act, 2000) and the
  National Institute of Cryptology Research and Development. Its origins
  trace to post-Kargil intelligence reform (Kargil Review Committee /
  Group of Ministers, a 2001 roadmap prepared under A.P.J. Abdul Kalam,
  Cabinet Committee on Security approval in October 2004).
- **Collection-and-processing, not operational:** NTRO is consistently
  described as a super-feeder agency — it collects and processes technical
  data and disseminates finished intelligence to consumer agencies
  (R&AW, IB, Defence Intelligence Agency) rather than acting on it
  itself. A tool built for this customer must therefore produce
  **transferable, verifiable work product**: findings another agency can
  rely on without re-running the collection. That is the manifest, and it
  is why the manifest — not the report alone — is JOCKY's primary output.
- **Oversight context strengthens the design:** NTRO has faced public
  scrutiny over unauthorized surveillance. Whether or not any specific
  allegation is credited, the lesson for a builder is durable: tooling
  offered to this ecosystem is most defensible when every action is
  authorized in advance (case + capabilities), bounded (timeouts, typed
  args), and reconstructible afterward (manifest). JOCKY's three hard
  rules are exactly that posture, compiled into software.

## 3. The Contest: How SIH Is Scored and What It Means for JOCKY

- **Published rubrics.** One college's published SIH 2026 internal-hackathon
  notice scores teams out of 100 as: idea pitch & problem understanding
  25, technical approach 30, feasibility & viability 20, impact &
  benefits 15, innovation/presentation/Q&A 10 — with technical approach
  the single heaviest weight. The SIH 2025 national evaluation criteria
  list problem understanding, novelty, technical depth and feasibility,
  presentation and communication, social relevance and impact, and
  preparedness/past work, with a prototype demo as bonus or mandatory
  criterion. SPOC guidance adds novelty, complexity, clarity, feasibility,
  practicability, sustainability, scale of impact, user experience, and
  future-work potential. Internal rounds are the real filter and are run
  with national-round rigor.
- **What judges reward, in practice:** problem understanding before tech
  stack; original thinking over buzzword count; a stable working MVP over
  an ambitious incomplete build; a live demo over slides; credible
  sourcing (government data, published research) over assumptions; and
  honest uncertainty over confident wrong answers in Q&A. Common judge
  questions include: why this statement, what existing solutions were
  evaluated and how yours differs, what assumptions were made, can it
  scale nationally, what would improve with more time, how it is
  maintained post-deployment.
- **JOCKY-to-rubric mapping (feeds the Phase 10 rationale document):**
  - *Problem understanding (25):* §1's phrase-by-phrase reading plus §8's
    reframing *is* the answer to "why this statement" — including the
    uncomfortable sentence about what the literal text asks for.
  - *Technical approach (30):* the FIR pipeline, dual toolchain with
    parity, tree-shaking, and capability gating give judges architecture
    to score instead of adjectives.
  - *Feasibility (20):* the phased roadmap (skeleton → registry →
    checker → resolver → embedding → runtime → parity) is each a demoable
    increment; the MVP deliberately excludes servers, agents, and kernel
    code.
  - *Impact (15):* authorized DFIR/audit teams plus the NTRO cluster
    (§2); the 65B-certificate angle (§7) makes the impact legible to
    non-technical judges.
  - *Innovation/Q&A (10):* the DSL + manifest-parity combination is the
    novelty claim; this research file is the Q&A briefing pack.
- **Demo strategy follows directly:** check → plan → run → verify
  (`05-frontend-design.md`) puts the heaviest rubric item (technical
  approach) on screen as artefacts (FIR, `--list-used`, manifest) rather
  than slides, and ends on integrity instead of features.

## 4. Landscape: Forensic Languages & Platforms

JOCKY is not the first to observe that investigators need a language
rather than a pile of one-off scripts. The precedents:

### 4.1 Velociraptor + VQL (closest language precedent)

- Velociraptor is an open-source endpoint monitoring, DFIR, and response
  platform built around the **Velociraptor Query Language (VQL)**. Its
  own docs state the motivation outright: endpoint tools must adapt to new
  IOCs faster than release cycles allow, so *"a query language can
  accelerate the time it takes to discover an IOC, design a rule to detect
  it, and then deploy the detection at scale"* — an investigator writes
  VQL, packages it in an **artifact**, and hunts across the deployment in
  minutes.
- Three VQL properties directly shaped JOCKY mechanisms:
  1. **Plugins as data sources.** In VQL, `FROM` clauses are backed by
     plugins — *"specific pieces of code which may accept arguments and
     generate a sequence of rows"* — and the data is generated
     dynamically, not read from stored tables. JOCKY `call` expressions
     are the same idea applied to shell scripts: a named function with
     typed arguments producing typed rows.
  2. **Environment-scoped parameters instead of string interpolation.**
     The VQL docs warn against building queries by string-concatenating
     user input and direct authors to place parameters in the query
     environment (*"You should always try to write VQL queries referring
     to parameters in the environment"*). JOCKY's hard rule — arguments
     type-validated against declared schemas before execution, never
     interpolated unvalidated into a shell command — is this lesson
     enforced at the type-checker level rather than left as guidance.
  3. **Stored queries (`LET`) as execution plans.** VQL authors chain
     `LET` stored queries to control execution order explicitly instead
     of relying on an optimizer. JOCKY's FIR plays the same role: an
     inspectable, ordered plan (`--list-used`, plan output) fixed before
     anything runs.
- Difference that justifies JOCKY's existence: VQL hunts live fleets from
  a server; JOCKY compiles a *verifiable, self-contained, manifest-logged*
  investigation against read-only evidence with no live infrastructure —
  the compiled/interpreted parity requirement (§9.1, Phase 8) has no VQL
  analog.

### 4.2 Plaso / log2timeline + Timesketch (timeline precedent)

- **Plaso** ("super timeline all the things") is the Python engine behind
  `log2timeline`: it extracts timestamped events from dozens of artifact
  types (filesystem, event logs, registry, browser history, prefetch…)
  into a `.plaso` storage file, which `psort` filters, sorts, and exports.
  **Timesketch** ingests `.plaso` files for collaborative, searchable,
  SIEM-like timeline analysis.
- Design debts JOCKY owes this stack: the `timeline_event` entity and the
  `correlate(a, b) within <duration> on <fields>` operator are a
  deliberately small, query-time version of the super-timeline idea
  (see `03-graph-schema.md`); the static-report MVP mirrors the
  `psort`-to-CSV/JSON export beat (produce a reviewable artifact, no
  server required); and the field's visible shift toward **targeted
  timelines** (parse only relevant artifacts via filter files instead of
  full super-timelines) supports JOCKY's scoped, case-declared evidence
  model over always-ingest-everything.
- Difference: Plaso/Timesketch assume a lab workstation and an analyst
  driving tools interactively. JOCKY assumes a *case file that must prove
  itself afterward* — hence the manifest as a first-class output, not an
  afterthought.

### 4.3 The Sleuth Kit / TSK body files (file-layer complement)

- TSK's `fls -m`-to-body-file-to-`mactime` pipeline remains the fast,
  granular file-system timeline path, and Plaso consumes TSK body files
  directly — evidence that layered tools beat monoliths. TSK's own wiki
  documents the two-step model (gather temporal data into body format,
  then sort/merge) and the per-filesystem MACB semantics its `mactime`
  output depends on (see §5.4). JOCKY's `directory` adapter occupies
  exactly this layer (metadata + hashes), designed to compose with, not
  replace, TSK-style tooling via `call`.

### 4.4 The Windows log-triage stack: Sigma, Hayabusa, Chainsaw (rule precedent)

- **Sigma** is the generic, open signature format for log detections:
  every rule declares a `logsource` triad (`category`/`product`/`service`
  — e.g. `category: process_creation, product: windows`), a `detection`
  block of named selections plus a boolean `condition` (AND/OR/NOT,
  `1 of`/`all of`, wildcards, brackets), optional `fields`, a `level`,
  and `tags` (including `attack.*` technique tags). Value modifiers
  (`|contains`, `|startswith`, `|endswith`, `|re`, `|cidr`, `|base64*`,
  `|windash`, `|exists`, comparisons) express match semantics
  portably; its stated purpose is to let analysts *"describe their once
  developed detection methods and make them [shareable] with others."*
  JOCKY's `rule_decl` is Sigma's thesis restated for a compiled DSL:
  logsource ≈ evidence/table binding, detection+condition ≈
  filter/correlate predicates, level/tags ≈ indicator confidence and
  ATT&CK references. Sigma's v2 correlation rules are the direct
  ancestor of anything `correlate` grows beyond MVP.
- **Hayabusa** (Yamato Security, Rust, AGPLv3) is a Sigma-based Windows
  event-log timeline generator and threat hunter: multi-threaded EVTX
  parsing into a single CSV/JSON/JSONL timeline, 4,000+ curated Sigma
  rules plus ~170 built-in rules, timeline-analysis commands
  (logon summaries, eid/computer metrics, base64 extraction, keyword
  pivots), output importable into Timesketch/Elastic/Timeline Explorer,
  and a Velociraptor artifact for enterprise hunts. Three of its
  engineering choices are load-bearing precedents for JOCKY:
  1. *Channel-based rule filtering* (only load rules whose channels
     appear in the EVTX at hand; 20%–10x speedups) is the same
     load-only-what-the-case-needs philosophy as tree-shaking.
  2. *XOR-encoded live-response packages* exist so endpoint AV does not
     false-positive on rule files **and** so fewer files are written
     (preserving USN-journal evidence) — the field's clearest statement
     that forensic tooling must budget its own footprint, which is what
     JOCKY's low-footprint claim cashes out as.
  3. *EVTX record carving and gap analysis* treat the evidence's own
     continuity as checkable — the ancestor of the compliance-domain
     checks proposed in §10.
- **Chainsaw** (WithSecure Labs, Rust) covers the same triage beat with
  Sigma rules plus a custom rule format, keyword/regex search, MFT /
  SRUM / Shimcache / Amcache / registry support, and JSON/CSV output —
  built explicitly for estates where no EDR was installed at compromise
  time. Its `analyse gaps` command (chronological/RecordID gaps as
  selective-deletion indicators) confirms that tamper-evidence over
  event streams is a first-class forensic primitive, not a nice-to-have.

### 4.5 YARA and YARA-X (file-triage precedent)

- YARA (Victor Alvarez / VirusTotal origin, now the industry's standard
  file-level detection format) expresses analyst knowledge as portable
  rules: `meta` (author, date, reference, hashes, ATT&CK tags — note the
  convention of carrying `mitre_attack` IDs *in the rule itself*),
  `strings` (text with `nocase`/`wide`/`fullword`/`xor` modifiers, hex,
  regex), and a boolean `condition` (`and`/`or`/`not`, `N of them`,
  `at`/`in` offsets, occurrence counts `#`, `filesize`, raw-byte reads
  like the canonical `uint16(0) == 0x5A4D` PE gate). Production practice
  — always gate on file type + size, require ≥3 independent strings,
  bound regex quantifiers, test against goodware/NSRL before shipping —
  is the quality bar any `jky_*_triage_*` stdlib function wrapping YARA
  must meet, and the `indicator(kind=yara)` row in `03-graph-schema.md`
  exists to carry YARA hits (rule name, matched strings, offset) into
  timelines.
- **YARA-X** (VirusTotal's Rust rewrite) matters twice over: as the
  memory-safe scanning engine JOCKY-era tooling should prefer, and as
  precedent that the field is actively re-implementing its load-bearing
  C tools in safer languages — the same modernization argument behind
  JOCKY's C++20-from-scratch front end.

### 4.6 The authorized-engagement model (target-user grounding)

- The PRD's target user — an authorized DFIR analyst or audit team doing
  Kryvasis-style engagements — reflects how the offensive-assurance
  market actually operates: scoped objectives, agreed rules of
  engagement, ATT&CK-aligned techniques, evidence-backed findings (PoC
  per finding), prioritized remediation, and retest/closure evidence.
  Kryvasis itself (Founder & CTO Nilotpal Guha, Kolkata) describes its
  practice as *"threat-modelled security assurance … evidence that the
  work made a difference"* — findings tied to real systems, realistic
  paths, accountable remediation, verifiable outcomes. JOCKY's case file
  is that engagement contract in executable form: scope (`evidence`,
  `allowed_capabilities`), method (FIR plan), findings (report), and
  proof of work (manifest) in one package.

### 4.7 Landscape gap table

| Tool | Has a language? | Execution model | Integrity story | What JOCKY adds over it |
| ---- | --------------- | --------------- | --------------- | ----------------------- |
| Velociraptor/VQL | Yes (VQL) | Live fleet hunts from a server | Server-side audit logs | Offline, compiled, manifest-proven runs; no infrastructure |
| Plaso + Timesketch | No (CLI + search syntax) | Lab workstation + web UI | Analyst-driven, ad hoc | Case file as executable proof; capability gating |
| Sleuth Kit | No (CLI) | Lab workstation | Practitioner discipline | Typed entities + correlation in-language |
| Hayabusa / Chainsaw + Sigma | Rules (YAML), not a language | Triage workstation / live response | Timelines + rule hits | Sigma-grade rules *inside* a verifiable compiled plan |
| YARA / YARA-X | Rules, not a language | Scanner over files | Match output | YARA hits typed as indicators feeding correlation |
| GRR-class agents | No (flows/APIs) | Deployed agents | Server logs | No agents, no persistence, no network surface |
| Incumbent lab suites (EnCase/FTK-class, cf. CFTT-tested tools) | No | Licensed lab workstation | Vendor procedures | Open, inspectable plan + manifest any third party can verify |
| Raw script folders (status quo ante) | No | Whatever the analyst typed | None | Grammar, types, registry, tree-shaking, manifest |

## 5. Evidence Formats Behind the Entity Schema

`03-graph-schema.md` fields were chosen to match what real tools emit —
field-by-field justification follows. Anything the schema cannot yet
represent is listed as a numbered gap feeding later phases.

### 5.1 Zeek logs → `flow` and `dns_event`

- **`conn.log` → `flow`.** Zeek's connection log (one row per
  connection, *"who is talking to whom, when, for how long, and with
  what protocol"*) carries `ts`, `uid`, `id.orig_h`, `id.orig_p`,
  `id.resp_h`, `id.resp_p`, `proto`, `service`, `duration`,
  `orig_bytes`, `resp_bytes`, `conn_state`, `missed_bytes`, `history`,
  `orig_pkts`, `orig_ip_bytes`, `resp_pkts`, `resp_ip_bytes`,
  `tunnel_parents`, `ip_proto`. The mapping is direct: `id.orig_h` →
  `src_ip`, `id.resp_h` → `dst_ip`, ports → ports, `proto` → `protocol`,
  `orig_bytes + resp_bytes` → `bytes`, `orig_pkts + resp_pkts` →
  `packets`, `ts`/`ts + duration` → `start_time`/`end_time`. Two
  field-guide facts shape analysis design: `service` is identified by
  content inspection, not port (HTTP on 8443 still logs `http` — so
  `filter` predicates should prefer service-derived protocol over raw
  ports), and `conn_state` encodes outcome (`SF` normal, `S0`
  unanswered — scan/sweep shape in bulk), which is the canonical
  `indicator` seed for the report domain.
- **`dns.log` → `dns_event`.** Zeek logs one row per query/response
  pair: `query`, `qtype_name`, `rcode_name`, `answers`, `TTLs`,
  `rejected`, plus the shared `ts`/`uid`/`id.*` tuple. Mapping: `query`
  → `query_name`, `qtype_name` → `query_type`, `id.orig_h` → `src_ip`,
  `ts` → `timestamp`, `rcode_name` → `rcode`. Known operational shapes
  worth pre-building as smoke fixtures (Phase 9): TXT/NULL-query bursts
  (tunneling shape), NXDOMAIN bursts (DGA shape — after ruling out
  search-suffix misconfiguration), `answers` pivoting back into
  `conn.log` addresses.
- **The `uid` lesson.** Zeek correlates across logs with a shared unique
  connection ID: the same `uid` joins `conn.log`, `dns.log`, `http.log`,
  `ssl.log`, `files.log`. JOCKY's MVP correlates on shared field values
  within time windows instead — but a synthetic join key emitted at
  ingestion (a `uid`-analog on `flow`) is the obvious post-MVP upgrade,
  recorded here so Phase 8+ can adopt it without schema churn.
- **Gaps (flow/dns_event):** no `conn_state`, `history`, `service`,
  `missed_bytes`, `answers`, or `uid` columns yet. Recommendation: add
  `service`, `conn_state`, `uid` first (all three drive the highest-value
  demo predicates); defer the rest.

### 5.2 Packet captures: pcap/pcapng and BPF

- **Format reality:** classic libpcap format is the open-source common
  denominator (`.pcap`, media type `vnd.tcpdump.pcap`); **pcapng** (IETF
  draft `draft-ietf-opsawg-pcapng`, block-structured around Section
  Header / Interface Description / Enhanced Packet Blocks) is what
  Wireshark writes by default and supports multi-interface captures,
  annotations, and nanosecond timestamps. libpcap ≥1.1 reads *some*
  pcapng but does not write it. Consequence for the `pcap` adapter: it
  must accept both formats on read (libpcap + a pcapng-aware path), and
  the manifest should record which container was actually ingested.
- **BPF is the filter contract.** The `bpf: string` argument in
  `jky_netforensics_extract_flows` is Berkeley Packet Filter syntax
  (`pcap-filter(7)`, compiled by `pcap_compile`): typed primitives
  (`host`, `port`, `net`, `tcp`, `udp`…), qualifiers, `and`/`or`/`not`
  combinators, even byte-level accessors (`tcp[13] & 0x02 != 0`). The
  Phase 4 validator must therefore treat `bpf` as a *compiled* artifact:
  reject strings `pcap_compile` refuses, before any capture is opened —
  unvalidated BPF text is exactly the unvalidated-interpolation class
  AGENTS.md §2.5 forbids.
- **Worked example continuity:** the architecture doc's `bpf: "tcp port
  443"` is a well-formed BPF primitive conjunction and a sensible
  triage slice (TLS-bearing flows for exfiltration/C2 review).

### 5.3 Windows event logs: the EVTX → JSON contract

- JOCKY's `eventlog` adapter consumes structured JSON/CSV, **not raw
  EVTX** — so the research question is what produces that JSON and what
  it contains. The answer is the §4.4 stack: Hayabusa/Chainsaw-style
  triage emits `timestamp/host/user/channel/event_id/message/process`
  rows (provider → `channel`, EventID → `event_id`, Computer →
  `host`, message payload → `message`, image/process fields →
  `process`) in CSV/JSON/JSONL directly consumable by the adapter and by
  Timesketch alike. Phase 9's smoke harness should bless one converter
  (Hayabusa `json-timeline` is the documented default) and pin its
  version in the manifest's `runtime_version`-adjacent metadata, so
  `log_event` rows are reproducible, not converter-folklore.
- **Gap:** native EVTX parsing inside the adapter is explicitly out of
  MVP scope; the adapter must fail loudly (manifest-logged) on binary
  EVTX input rather than misparse it.

### 5.4 NTFS/MFT → `file` (and what the schema still misses)

- Every MFT record carries **eight** timestamps: four in
  `$STANDARD_INFORMATION` (attribute 0x10) and four in `$FILE_NAME`
  (attribute 0x30) — Created/Modified/MFT-modified/Accessed in each.
  TSK's `mactime` timeline uses the SI set with per-filesystem MACB
  semantics (NTFS: m = File Modified, a = Accessed, c = MFT Modified, b
  = Created); the FN set is inspectable via `istat`, not the timeline.
- Three practitioner facts constrain the `file` entity:
  1. **SI and FN disagree by design.** FN-created is the most trustworthy
     "first appeared here" signal; SI-created is the easiest to forge
     (`SetFileTime` updates whichever of the four the caller names).
     MVP's single `mtime`/`ctime` pair therefore needs an explicit
     provenance note per row (SI vs FN) no later than Phase 4 — without
     it, a timestomped file looks authoritative.
  2. **Timestomping has a fingerprint.** NTFS ticks at 100ns; native
     operations leave sub-second noise, while common timestomping tools
     round to whole seconds. A `jky_compliance_*` check for
     `.0000000`-aligned clusters is a cheap, high-signal Phase 9 stdlib
     addition.
  3. **Records are reused.** The MFT record/sequence-number pair
     distinguishes a file from the deleted predecessor that previously
     owned its slot; timelines that drop the sequence silently conflate
     them. `file` needs `mft_record` + `seq` columns post-MVP.
- **Gaps (file):** no `atime`/`btime` (created), no SI-vs-FN provenance,
  no record/sequence, no alternate-data-stream awareness (`file:stream`
  naming per TSK's `MFT-128-id` addressing). Also note the platform skew:
  ext4/FAT/APFS timestamp sets differ from NTFS (TSK's MACB table), so
  the directory adapter must normalize per-filesystem, never assume NTFS.
- **Corroboration layer:** the USN journal (`FILE_CREATE`,
  `DATA_OVERWRITE`, `RENAME_*`, `FILE_DELETE`, `CLOSE` reason codes)
  gives the *operation* where the MFT gives the *resulting state*; the
  merge rule (sort to the second, USN as tiebreaker, millisecond drift
  expected) is the documented procedure for any future USN-aware
  `jky_timeline_*` function.

### 5.5 Logs, flows-by-other-means, and hash sets

- `log_event`'s seven fields are the intersection of syslog-style,
  JSONL, and EVTX-derived rows — deliberately narrow so every adapter
  output and every `call` output conforms without lossy squeezing.
- NetFlow v9 / IPFIX exporters produce flow records upstream of any
  capture; treating exporter output as a future `flow` source (same
  entity, different adapter) is already compatible with the schema.
- NIST's program pairs tool testing with **reference data**: NSRL known-
  good hash sets and CFReDS reference images. A `jky_compliance_*`
  allowlist check of `file.sha256` against a pinned NSRL slice is the
  obvious Phase 9 triage accelerator (and the reason `sha256` is a
  first-class `file` field, not metadata).

### 5.6 Memory forensics: the documented deferral

- RFC 3227's volatility order (§6.2) ranks memory *above* disk — so the
  absence of a memory adapter in the MVP is the schema's most conspicuous
  gap and must be owned, not discovered: live-memory acquisition is
  live acquisition (a justifiability-logged deviation, §6.2/§10), and
  the MVP's static-evidence scope (pcap, logs, filesystem) covers the
  PS's "computer & network forensic analysis" for captured evidence.
  Memory entities (`process`, `connection`-from-memory, `injected_region`
  indicators) are the first schema extension queued behind the MVP.

### 5.7 Time: UTC, clock drift, precision

- Adapters normalize to UTC before any operator runs (per
  `04-ingestion-pipeline.md`); RFC 3227 additionally requires recording
  the source system's **clock drift**, so drift notes belong in manifest
  `inputs[]` metadata. Precision varies by source (100ns NTFS ticks,
  nanosecond pcapng, second-granularity syslog) — `correlate … within …`
  windows must therefore be chosen per-pair, and the EBNF's bare
  `duration` deserves a Phase 3 note that sub-second windows are only
  meaningful for high-precision sources.

### 5.8 Known DNS-visibility limit

- Zeek's own docs scope `dns.log` to unencrypted DNS: DNS-over-HTTPS and
  DNS-over-TLS bypass passive collection. The `dns_event` entity models
  observable DNS only; the report domain must never present DNS absence
  as DNS innocence. (This is also why DoH/DoT-shaped `flow` rows —
  `tcp/443` to known-resolver addresses — are themselves an indicator
  class.)

## 6. Evidence Integrity Foundations

JOCKY's read-only-evidence and manifest rules are not inventions — they
are the ISO/IEC 27037 handling model plus IETF/NIST practice, compiled
into a language runtime.

### 6.1 ISO/IEC 27037 (core standard)

- ISO/IEC 27037:2012 (confirmed current on ISO's catalogue; adopted in
  Europe as EN ISO/IEC 27037:2017) gives guidelines for the
  **identification, collection, acquisition, and preservation** of
  digital evidence, inside a four-standard family (27037, 27041, 27042,
  27043) covering the evidence lifecycle end to end.
- **The four quality principles** (auditability, repeatability,
  reproducibility, justifiability) map onto JOCKY one-to-one:
  - *Auditability* (every action reconstructible by a third party via
    logs, hashes, records) → the manifest: per-execution entries with
    function, script hash, args, exit code, timestamps, stdout hash.
  - *Repeatability / reproducibility* (same procedure, same or
    equivalent setup → same results) → compiled/interpreted parity
    testing (Phase 8) plus `jocky verify` re-hashing.
  - *Justifiability* (every deviation documented) → capability denials
    and failures logged as manifest entries, never silent; live
    acquisition, if ever added, would have to be logged as a documented
    deviation (see §10).
- **Hash practice:** the standard requires a cryptographic hash at
  acquisition; field practice has converged on **SHA-256 for every new
  acquisition** — SHA-1 deprecated since the 2017 SHAttered collisions,
  MD5 never acceptable as sole verification. JOCKY standardizes on
  SHA-256 (`script_sha256`, `stdout_sha256`, input/output hashes) for
  exactly this reason; parallel SHA-512 is a noted future option (§10).
- **Read-only handling:** acquisition must not alter the original, via
  hardware/software write blockers; the UK NPCC principles state it as
  Principle 1 — *"No action taken … should change data which may
  subsequently be relied upon in court"*. JOCKY compiles this principle
  into the type system: adapters open read-only, and any pipeline
  `write` targeting a declared evidence path is rejected before
  execution.
- **Chain of custody:** who acted, with which tool, where and when (hash
  + timestamp + seal + custody register). The manifest schema
  (`case_id`, `runtime_version`, `run_start/end_utc`, `authorization`,
  `inputs[]`, `outputs[]`, `executions[]`) is that register, in JSON.

### 6.2 RFC 3227: collection order and discipline

- The IETF's evidence-collection guidelines (Brezinski & Killalea, 2002)
  add three operational rules the adapters and runbooks must encode:
  1. **Order of volatility** — collect registers/cache →
     routing/ARP/process/memory → temporary filesystems → disk → remote
     logs → topology → archival media. This order is both the
     justification for the MVP adapter set (disk, remote logs, captures
     are the acquirable-static tail) and the standing argument for the
     deferred memory adapter (§5.6).
  2. **Transparency and method** — procedures must be detailed,
     unambiguous, and minimize on-the-spot decisions; *"when in doubt
     err on the side of collecting too much."* The FIR is this rule as
     data: the procedure is written, reviewed, and frozen before contact
     with evidence.
  3. **Archiving** — strictly secured storage with a documented chain of
     custody; collection toolkits kept on **read-only media** prepared in
     advance. The `jockyc` standalone binary is the modern form of the
     read-only toolkit CD: fixed, hashed, carried to the evidence —
     never built ad hoc on the analysis machine.

### 6.3 NIST SP 800-86 and SP 800-61 (process models)

- **SP 800-86** (Guide to Integrating Forensic Techniques into Incident
  Response) defines the four-phase forensic process JOCKY's pipeline
  mirrors — **collection → examination → analysis → reporting** — and
  organizes data sources exactly along JOCKY's adapter seams: data files,
  operating systems, network traffic, applications. It dedicates
  subsections to data-file integrity and to file modification/access/
  creation times, i.e., the `sha256` and timestamp columns of §5 are
  NIST-blessed load-bearing fields, not decoration. Its standing warning
  — be ready *"to demonstrate the integrity"* of logs and records that
  *"can be altered or otherwise manipulated"* — is the manifest's mission
  statement in one sentence.
- **SP 800-61r2** (Computer Security Incident Handling Guide) places
  JOCKY in the incident lifecycle: preparation → **detection &
  analysis** → containment/eradication/recovery → post-incident lessons.
  JOCKY lives in detection & analysis (and feeds post-incident via the
  report); it never touches containment — a scope fact worth stating to
  judges, since "response" overreach is where forensic tools accumulate
  weaponizable features. Its jump-kit inventory (packet sniffers,
  forensic software, trusted binaries, chain-of-custody forms) is the
  physical analog of the embedded-script closure.

### 6.4 NIST CFTT (why parity testing is a precedent, not paranoia)

- The Computer Forensics Tool Testing program answers *"do forensic
  tools work as they should?"* with a conformance model: specification →
  single-claim test assertions → test cases → validated harness →
  published report — testing **by feature** (acquisition, search,
  recovery separately). Its reports have been cited in court; its stated
  benefits include reducing admissibility challenges and helping users
  choose tools. JOCKY's Phase 8 parity harness and Phase 9 smoke tests
  are a CFTT-style program stood up on day one: the FIR is the
  specification, each operator/adapter is a feature under test, and
  matching compiled/interpreted manifests are the published reports.

### 6.5 SLSA + in-toto (the manifest as provenance attestation)

- The software-supply-chain world already standardized JOCKY's manifest
  shape. **SLSA Build Provenance** (an in-toto attestation predicate)
  records *where, when, and how an artifact was produced*: a
  `buildDefinition` (build type, external/untrusted parameters that
  *must* be verified downstream, internal parameters, resolved
  dependencies with digests) plus `runDetails` (builder identity,
  invocation ID, start/finish timestamps, byproducts) over output
  `subject`s. Verification tooling (`slsa-verifier`, Sigstore bundles,
  Verification Summary Attestations) checks all of it.
- Field mapping (JOCKY → SLSA concept): `.jky` source + FIR ≈
  `buildDefinition`; `call` args ≈ `externalParameters` (untrusted until
  validated — the exact SLSA rule); embedded-script closure ≈
  `resolvedDependencies`; `runtime_version` ≈ `builder.id`;
  `run_start/end_utc` ≈ `startedOn`/`finishedOn`; report + outputs ≈
  `subject`/`byproducts`; `jocky verify` ≈ the verifier role (a future
  VSA-style signed verdict is the natural Phase 8+ upgrade).
- Two SLSA rules become JOCKY requirements: keep the untrusted-parameter
  surface minimal and reject the unexpected (⇒ strict `@jocky:inputs`
  schemas, no extra args), and record digests of everything that could
  influence the result (⇒ script hashes, input hashes, stdout hashes —
  already the schema). **Gap:** v0.1 manifests are unsigned; signing
  (Sigstore-style) is queued in §10 — until then `verify` proves
  integrity against the manifest, not the manifest's own authenticity.

## 7. Indian Admissibility: Section 65B and the Manifest as Certificate Material

For an NTRO-judged Indian hackathon, this section is the highest-value
research in the file: it connects the manifest to the courtroom.

- **The doctrine.** Sections 65A/65B of the Indian Evidence Act, 1872
  (inserted by the IT Act, 2000) are, per *Anvar P.V. v. P.K. Basheer*
  (2014, three-judge bench), **a complete code** for electronic-record
  admissibility — the general secondary-evidence routes are unavailable,
  and the §65B(4) certificate is a **condition precedent** (the looser
  *Navjot Sandhu* 2005 reading was overruled). *Arjun Panditrao Khotkar
  v. Kailash Gorantyal* (2020, three-judge bench) confirmed and
  clarified: the certificate is unnecessary **only if the original
  device itself is produced** (owner in the witness box); network/system
  originals that cannot be brought to court go through §65B(1)+(4)
  exclusively; oral testimony cannot substitute for the certificate;
  timing is flexible (certificate may follow during trial); courts can
  summon reluctant certifiers; and telecom/ISP CDR preservation plus
  future §67C-IT-Act rules on retention, chain of custody, stamping, and
  metadata were expressly directed. Per 2026 analysis, the doctrine
  carries onto **Section 63 of the Bharatiya Sakshya Adhiniyam**
  unchanged — so this research does not expire with the old code.
- **What a §65B(4) certificate must contain:** identification of the
  electronic record, the manner of its production and particulars of the
  producing device (including regular-use/ordinary-course/proper-
  operation conditions), signature of a responsible-position person, to
  the best of their knowledge and belief.
- **Manifest → certificate mapping (the judge slide):**

| §65B(4) element | Manifest field(s) supplying it |
| --------------- | ------------------------------ |
| Identify the record | `inputs[]` (path, sha256, bytes, adapter) |
| Manner of production / device particulars | `executions[]` (function, script_sha256, args) + adapter records in `inputs[]` |
| Regular use / proper operation | `runtime_version`, pinned converter versions (§5.3), CFTT-style parity reports (§6.4) |
| Responsible person, knowledge & belief | `authorization` (case, allowed capabilities) + analyst attestation over the manifest |
| Original-vs-copy distinction | `script_sha256` of source vs hashes of derived outputs; originals producible where held |

- **Positioning statement for Phase 10:** JOCKY does not issue §65B
  certificates — only a responsible person can sign one. What it does is
  make the certificate *writable in good conscience*: every factual
  assertion a certificate needs (what was processed, with what tool, on
  what input, producing what output, when, under whose authority) is a
  manifest field, hashed at the time, verifiable afterward. No other tool
  in the §4 landscape offers that sentence.

## 8. Why Each Excluded Component Is Excluded

Each item below follows the same structure: *what it is → documented
offensive use (with 2026 evidence where available) → why no legitimate
forensic workflow needs it → what JOCKY does instead.* All of these are
catalogued MITRE ATT&CK techniques — i.e., the industry's shared
vocabulary describes them as adversary behavior, which is precisely why a
defensive tool must not ship them. Defenders, for their part, counter
these techniques with network allow/block lists and TLS inspection
(MITRE's own mitigations for proxy/C2 tradecraft) — controls that only
work because analysis tooling stays visible. JOCKY stays visible.

### 8.1 Polymorphic / metamorphic engines (T1027.014, parent T1027)

- A polymorphic engine pairs an encrypted payload with a mutation engine
  that rewrites the decryptor stub (register swaps, instruction
  substitution, junk insertion, fresh keys) so **no two copies share the
  same bytes**; metamorphic engines rewrite the entire body. MITRE
  catalogues this under Defense Evasion (T1027.014, *"capable of changing
  its runtime footprint during code execution"*); 2026 analyses cite real
  implants (e.g., BlackTech's BendyBear) changing their runtime
  footprint during execution.
- The technical reason for exclusion goes beyond ethics: **a polymorphic
  component is definitionally incompatible with JOCKY's product.** JOCKY's
  output *is* verifiable hashes — of scripts, inputs, outputs, stdout.
  A component whose purpose is to make every byte-sequence unique
  destroys exactly the property (stable, matchable hashes) the manifest
  exists to provide. Instead: scripts are embedded verbatim, hashed at
  compile time, and re-hashed at execution; `verify` re-checks them.

### 8.2 Process hollowing, reflective injection, API unhooking, direct syscalls (T1055 family, T1562.001)

- Process hollowing (T1055.012) runs malicious code *"in the address
  space of a separate live process"* by spawning suspended, unmapping,
  and replacing memory; MITRE states plainly that it *"may also evade
  detection from security products since the execution is masked under a
  legitimate process."* Observed users include ransomware (e.g.,
  BlackByte). Sibling techniques — reflective injection, API unhooking
  (a form of impairing defenses, T1562.001), direct syscalls including
  syscall-based driver loads that dodge Service Control Manager
  telemetry — serve the same goal: executing while sensors watch
  somewhere else.
- No forensic step requires executing *as* another process or blinding a
  sensor. JOCKY's alternative is the sandboxed dispatcher: scripts run as
  themselves, as declared children of the runtime, with captured stdout,
  exit codes, timeouts, and hashes — attributable by construction.

### 8.3 BYOVD / kernel driver access (T1068, T1562.001, T1014)

- Bring-Your-Own-Vulnerable-Driver loads a signed-but-flawed driver to
  gain ring-0, then kills or blinds endpoint protection (unlinking kernel
  callbacks, terminating PPL-protected processes — PPL being irrelevant
  once the caller runs in ring-0). In 2026 this went industrial: Qilin
  and Warlock campaigns used curated, rotating driver pools (tracking
  Microsoft's blocklist lag) to disable 300+ security products as a
  standard pre-encryption step. The chain spans MITRE techniques from
  privilege escalation (T1068) through defense impairment (T1562.001) to
  rootkit behavior (T1014), with delivery-side support (proxy execution
  T1218, service execution T1569.002, code-signing abuse T1553.002).
- A forensic analyzer has no read path that requires kernel code —
  pcap, logs, and filesystem metadata are all obtainable in user space.
  Shipping a kernel primitive would convert every JOCKY deployment into a
  BYOVD loader waiting for a caller binary. JOCKY instead runs entirely
  in user space under the case capability set.

### 8.4 C2 channels and domain fronting (T1090.004 under T1090, tactic TA0011)

- Domain fronting places one domain in the TLS SNI and another in the
  HTTP Host header so CDN routing delivers traffic to an
  attacker-controlled backend while inspection sees a trusted name
  (variants include blank-SNI "domainless" fronting); MITRE (T1090.004,
  tactic: Command and Control) documents use by APT29 (meek plugin),
  Cobalt Strike, Mythic, and SMOKEDHAM, with application-layer C2 more
  broadly covered under T1071. CISA's mitigation notes concede blocking
  *"may be circumvented by … Domain Fronting."*
- JOCKY's answer is architectural, not policy-based: **there is no
  network surface at all** — no beacons, no exfiltration path, no proxy
  logic. Findings leave the machine as a static report file whose hash is
  manifest-logged, carried out by the analyst through existing authorized
  channels.

### 8.5 Exclusion summary

| Excluded component | ATT&CK | Why it kills a forensic tool's value | JOCKY alternative |
| ------------------ | ------ | ------------------------------------ | ----------------- |
| Polymorphic engine | T1027.014 | Destroys stable hashes — the manifest's currency | Verbatim embedding + script hashes + `verify` |
| Process hollowing / injection / unhooking / direct syscalls | T1055, T1562.001 | Unattributable execution | Named-child dispatcher, stdout/exit/timeout logging |
| BYOVD / kernel drivers | T1068, T1562.001, T1014 | Every install becomes a ring-0 loader | User-space only, capability-gated |
| C2 / domain fronting | T1090.004 (TA0011), T1071 | Covert channel contradicts auditability | No network surface; static report artifact |
| "Not triggering security solutions" (literal) | — | Specifies evasion as a requirement | Allowlisted, declared, logged execution |

## 9. Implementation Precedents (Phase-by-Phase)

### 9.1 Frontend: recursive descent, tree-walk interpreter, then a compiler (Phases 1, 3, 8)

- Robert Nystrom's *Crafting Interpreters* (Google/Dart language
  engineer; full text and both interpreters open-source) is the closest
  published recipe to JOCKY's front end: handwritten **recursive descent**
  — *"the simplest way to build a parser … fast, robust, and can support
  sophisticated error handling"*, as used by GCC, V8, and Roslyn — with
  one function per grammar rule, panic-mode synchronization for
  diagnostics, and a grammar designed to stay parseable (left recursion
  eliminated). That is the Phase 1 skeleton and the Phase 3 `call`
  extension, technique for technique.
- The book then builds **two** execution modes for one language — a
  tree-walking interpreter and a bytecode compiler — which is the direct
  structural precedent for `jocky` (interpret, Phase 8) vs `jockyc`
  (compile, Phase 6): same front end, same semantics, two back ends with
  a parity obligation between them.

### 9.2 Runtime sandbox: layered, unprivileged-first (Phase 7)

| Mechanism | Model | Inheritance | Fit for JOCKY's dispatcher |
| --------- | ----- | ----------- | -------------------------- |
| Subprocess + timeout + pipe capture | OS process boundary, wall-clock kill | N/A (baseline) | **MVP baseline.** Gives timeout, exit codes, stdout capture — the entire `executions[]` entry — with zero kernel dependencies |
| seccomp-bpf (Linux) | Syscall allow/deny lists in BPF, incl. argument filtering (used with args by ~80% of adopting packages) | Inherited, one-way | Hardening layer: deny `execve`-outside-closure, `socket`/`connect` (no-network guarantee), dangerous ioctls. Policy authoring is intricate (open vs openat equivalence traps) — budget review time |
| Landlock (Linux LSM, unprivileged) | Path-based access rights on filesystem/network, stacked one-way domains, `no_new_privs` | Inherited, one-way | **Best-fit hardening.** Kernel-enforced read-only evidence dirs + write-only `out/` + no network, set up by the unprivileged runtime itself. Kernel docs frame it as the complement to seccomp (access control on objects vs syscall filtering) — deploy both |
| Namespaces / containers | Isolation via mounts, PIDs, net | Configured | Heavyweight for a demo binary; question of `CAP_SYS_ADMIN` for setup. Defer |
| pledge/unveil (OpenBSD) | Promise-group syscall pledges + path unveiling, ~tens of lines | Cleared on exec (non-inheriting) | Elegant but platform-locked and non-inheriting; note as the portability answer if an OpenBSD target ever appears |
| Capsicum (FreeBSD) | Capability mode + attenuated fds, principled deny-by-default | Inherited | Same verdict as pledge: platform-locked; the capability *model* informs capability naming regardless |

- Phase 7 recommendation encoded here: ship subprocess+timeout+capture
  first (every manifest field obtainable), then Landlock filesystem
  scoping (evidence read-only enforced *by the kernel*, defense in depth
  behind the type checker), then seccomp-bpf syscall policy. An
  OpenBSD/Capsicum port is not on the roadmap; the table exists so the
  decision is recorded, not revisited.

### 9.3 Resolver: tree-shaking is a solved problem (Phase 5)

- JavaScript bundlers (Rollup/esbuild), the Go linker, and `ld
  --gc-sections` all compute reachable-closures from entry points and
  discard the rest. JOCKY's resolver is that algorithm over a different
  graph: entry points = `call` sites in the FIR, edges = `depends_on`,
  atoms = whole scripts (no intra-script splitting for MVP — shell does
  not link like objects do). `--list-used` is the bundle report.

### 9.4 Embedding: bytes-in-binary is standard practice (Phase 6)

- `objcopy --input binary`, `xxd -i`, Go `//go:embed`, Rust
  `include_bytes!` all solve "ship data inside the executable"; CMake
  can drive any of them at build time. JOCKY embeds the tree-shaken
  *script closure* the same way, with one forensic addition the
  precedents lack: each embedded blob's SHA-256 is recorded at embed
  time and re-checked at dispatch — embedding with a built-in chain of
  custody.

### 9.5 Validated arguments, capability gating, static reports

- Unchanged from the first pass: VQL environment parameters (§4.1) for
  validated args; capability discipline with denials-as-evidence for the
  gate; `psort`-style export plus the Hayabusa JSON-timeline beat
  (§4.4) for the static-report MVP; VQL `LET` chains and the FIR-as-
  procedure (§6.2) for the inspectable plan.

## 10. Open Questions & Risks Carried Forward

1. **Persistent graph store** (from `03-graph-schema.md` §3): still open,
   still post-MVP. Any proposal must preserve read-only evidence,
   manifest-logged execution, and capability gating.
2. **Schema gaps to close post-MVP:** `flow` lacks `service`,
   `conn_state`, `uid`; `dns_event` lacks `answers`; `file` lacks
   `atime`/`btime`, SI-vs-FN provenance, MFT record/sequence, and ADS
   awareness (§5.1, §5.4). Priority order: `service` + `conn_state` +
   `uid`, then SI/FN provenance, then the rest.
3. **EVTX preprocessing contract:** native EVTX parsing is out of MVP;
   the blessed converter + pinned version (§5.3) must be chosen in
   Phase 9, and the adapter must reject binary EVTX loudly.
4. **pcapng awareness:** read support for both containers; record the
   actual container in the manifest (§5.2).
5. **Memory adapter deferral:** owned in §5.6; first schema extension
   queued behind the MVP, gated on a justifiability-logged live-
   acquisition design.
6. **Live acquisition generally:** currently assumed static evidence;
   any live path needs manifest-level deviation records per ISO
   justifiability (§6.1) and RFC 3227 ordering (§6.2).
7. **Manifest signing:** v0.1 manifests are unsigned — integrity is
   proven *against* the manifest, not *of* it. Sigstore-style signing
   and VSA-style verdicts queued (§6.5).
8. **Hash agility:** SHA-256 now; consider parallel SHA-512 digests in
   the manifest schema (v0.2) as cheap insurance.
9. **Registry annotation burden:** the pruned registry (41 curated
Kalki functions plus the 6-function netforensics batch, both recorded
in `09-kalki-inventory.md`) needs valid `@jocky:` headers (Phase 9;
the 6 netforensics functions already carry them). Risk: inconsistent
`depends_on` graphs
   and capability labels; mitigation is the `scan_registry` reject path
   plus the smoke-test harness. YARA-rule hygiene (§4.5) is the quality
   bar to copy.
10. **Judge risk:** a literal-PS reading will ask where the evasion
    components are. Answer chain: §1 + §8 → `01-prd.md` boundary →
    Phase 10 rationale → Phase 11 gap-check matrix, with §3's rubric
    mapping setting the terms and §7's 65B angle as the closer.

## 11. Sources

All URLs retrieved 2026-09-11/12; titles as published.

**Problem statement**
- NTRO organisation page, SIH 2026 catalogue —
  `https://sih2026.vuce.in/orgs/national-technical-research-organisation-ntro`
- Blockchain & Cybersecurity theme page, SIH 2026 catalogue —
  `https://sih2026.vuce.in/themes/blockchain-cybersecurity`
- Community SIH 2026 problem-statement repository (NoBugNinja) —
  `https://github.com/NoBugNinja/Smart-India-Hackathon-SIH-2026-Problem-Statements`
- SIH 2026 problem-statements compilation (Scribd, doc 1077383836) and
  226-statement master catalogue (Scribd, doc 1078836162)

**Customer (NTRO)**
- NTRO official site — `https://ntro.gov.in/welcome.do`
- National Technical Research Organisation (Wikipedia — origins, mandate,
 TechINT disciplines, NCIIPC/NICRD) —
  `https://en.wikipedia.org/wiki/National_Technical_Research_Organisation`
- NTRO entity brief, UNIDIR Cyber Policy Portal —
  `https://database.cyberpolicyportal.org/en/entity/kurt7ye2f4`
- NTRO mandate/collection role analysis (Model Diplomat, 2026-07-07) —
  `https://modeldiplomat.com/learn/glossary/national-technical-research-organisation`
- NTRO leadership/role coverage (Hindustan Times, 2020-09-18) —
  `https://www.hindustantimes.com/india-news/anil-dhasmana-is-new-chief-of-ntro-spy-agency-that-keeps-an-eye-from-the-sky/story-rTvVl6VH7NG3pukNyXUExH.html`

**Contest (SIH scoring)**
- Internal SIH 2026 guide: judging parameters —
  `https://thenewviews.com/internal-smart-india-hackathon-2026/`
- SIH 2026 college-SPOC guidelines (Scribd, doc 1060690857)
- SIH 2025 evaluation criteria (Scribd, doc 915964336)
- SIH 2026 registration/eligibility/judging-criteria guide (Where U
  Elevate, 2026-07-20) —
  `https://whereuelevate.com/blogs/smart-india-hackathon-2026`
- GCELT internal-hackathon notice with 100-mark rubric (Scribd, doc
  1074627210)

**Forensic languages & platforms**
- VQL docs, Velociraptor — `https://docs.velociraptor.app/docs/vql/`
- VQL fundamentals — `https://www.velociraptor-docs.org/docs/vql/fundamentals/`
- Velocidex Query Language deep-dive (2018, retained for history) —
  `https://docs.velociraptor.app/blog/html/2018/08/10/the_velocidex_query_language/`
- Velociraptor repository (Velocidex) —
  `https://github.com/Velocidex/velociraptor/`
- Velociraptor DFIR overview (Rapid7) —
  `https://www.rapid7.com/products/velociraptor/`
- Plaso repository (log2timeline) —
  `https://github.com/log2timeline/plaso`
- Plaso user docs — `https://plaso.readthedocs.io/en/stable/sources/user/`
- Plaso super-timeline course notes (PSU) —
  `https://web.cecs.pdx.edu/~dmcgrath/courses/Su26/forensics/plaso.html`
- Forensic timelining overview — `https://theforensicsway.com/blog/timeline/`
- Log2Timeline guide (Cyber Forensics Academy, 2025-12-11) —
  `https://www.cyberforensicacademy.com/blog/log2timeline-guide-creating-forensic-timelines`
- TSK timelines wiki (fls/mactime two-step, per-FS times) —
  `https://github.com/sleuthkit/sleuthkit/wiki/Timelines`
- TSK mactime-output wiki (MACB table) —
  `https://github.com/sleuthkit/sleuthkit/wiki/Mactime_output`
- Sigma rules specification (SigmaHQ) —
  `https://sigmahq.io/sigma-specification/specification/sigma-rules-specification.html`
- Sigma rules/logsources/modifiers docs —
  `https://sigmahq.io/docs/basics/rules.html`,
  `https://sigmahq.io/docs/basics/log-sources.html`,
  `https://sigmahq.io/docs/basics/modifiers.html`
- Sigma repository (SigmaHQ) — `https://github.com/SigmaHQ/sigma`
- Hayabusa docs (Yamato Security) — `https://yamato-security.github.io/hayabusa/`
- Hayabusa repository — `https://github.com/Yamato-Security/hayabusa`
- Chainsaw repository (WithSecureLabs) —
  `https://github.com/WithSecureLabs/chainsaw`
- YARA rule writing docs — `https://yara.readthedocs.io/en/stable/writingrules.html`
- YARA-X rule docs (VirusTotal) —
  `https://virustotal.github.io/yara-x/docs/writing_rules/`
- YARA production-rule guide (ForgeWork, 2026-01-12) —
  `https://forge-work.com/blog/yara-rules-malware-detection.html`
- YARA malware-detection guide (Decryption Digest, 2026-05-15) —
  `https://www.decryptiondigest.com/blog/how-to-write-yara-rules-malware-detection`
- Kryvasis practice statement (N. Guha, LinkedIn, 2026-08-08) —
  `https://www.linkedin.com/posts/nilotpal-guha-2876b72b7_the-most-valuable-thing-a-founder-builds-activity-7491863365921923073-5xZT`
- Red-team engagement model (Praetorian) —
  `https://www.praetorian.com/services/red-team/`
- Audit/pentest/red-team layering (Hard2bit) —
  `https://hard2bit.com/en/services/pillar/pentesting-redteam/`

**Evidence formats**
- Zeek conn.log reference — `https://docs.zeek.org/en/current/reference/logs/conn.html`
- Zeek dns.log reference — `https://docs.zeek.org/en/current/reference/logs/dns.html`
- Zeek log tutorial (conn/dns/uid pivoting, zeek-cut) —
  `https://docs.zeek.org/en/v8.2.0/tutorial/logs.html`
- Zeek log-script reference (log inventory) —
  `https://docs.zeek.org/en/lts/script-reference/log-files.html`
- Zeek log field guide (Saade — service-by-content, conn_state, DGA shapes) —
  `https://www.patricksaade.com/reference/zeek-logs/`
- pcap-filter man page (BPF syntax, tcpdump.org) —
  `https://www.tcpdump.org/manpages/pcap-filter.7.html`
- pcapng draft (IETF OPSAWG, SHB/IDB/EPB) —
  `https://datatracker.ietf.org/doc/html/draft-ietf-opsawg-pcapng`
- libpcap file-format wiki (pcap vs pcapng, Wireshark defaults) —
  `https://wiki.wireshark.org/Development/LibpcapFileFormat`
- TSK NTFS wiki (MFT, $STANDARD_INFORMATION, $FILE_NAME) —
  `https://github.com/sleuthkit/sleuthkit/wiki/NTFS`
- TSK NTFS implementation notes (istat walkthrough, ADS) —
  `https://github.com/sleuthkit/sleuthkit/wiki/NTFS_Implementation_Notes`
- MFT-timeline review-grade practice (mftparser, 2026-05-22 — SI vs FN
  trust, timestomp fingerprints, USN merge, sequence numbers) —
  `https://www.mftparser.com/en/blog/build-mft-timeline`

**Integrity, process, provenance**
- ISO/IEC 27037:2012 catalogue entry —
  `https://www.iso.org/standard/44381.html`
- ISO/IEC 27037 guide (TrueScreen, 2026-05-14) —
  `https://truescreen.io/articles/iso-27037-digital-evidence-standard/`
- UNODC cybercrime education module 4: standards and best practices —
  `https://www.unodc.org/cld/en/education/tertiary/cybercrime/module-4/key-issues/standards-and-best-practices-for-digital-forensics.html`
- ISO/IEC 27037 civil-procedure adaptation study (2026-06-30, doi
  `10.32505/politica.v13i1.15704`)
- RFC 3227 (Brezinski & Killalea, 2002 — volatility order, collection,
  archiving) — `https://www.rfc-editor.org/rfc/rfc3227.html`
- NIST SP 800-86 (forensic techniques in incident response) —
  `https://csrc.nist.gov/pubs/sp/800/86/final`
- NIST SP 800-61r2 (incident handling lifecycle, jump kits) —
  `https://nvlpubs.nist.gov/nistpubs/SpecialPublications/NIST.SP.800-61r2.pdf`
- CFTT program overview (NIST deck — conformance model, NSRL/CFReDS,
  court citations) —
  `https://www.nist.gov/system/files/documents/2017/05/08/aafs-seattle-2006-engineering-section.pdf`
- SLSA Build Provenance spec — `https://slsa.dev/spec/v1.2-rc2/build-provenance`
- SLSA specification v1.2 — `https://slsa.dev/spec/v1.2/`
- in-toto and SLSA (layouts, complementary predicates, VSA) —
  `https://slsa.dev/blog/2023/05/in-toto-and-slsa`
- slsa-verifier (verification tooling, Sigstore bundles) —
  `https://github.com/slsa-framework/slsa-verifier/`

**Indian admissibility**
- *Arjun Panditrao Khotkar v. Kailash Gorantyal* full judgment (AP High
  Court archive PDF, 2020-07-14) —
  `https://aphc.gov.in/docs/imp_judgements/Arjun%20Panditrao%20Khotkar%20_%20Kailash%20Kushanrao%20Gorantyal%20And%20Ors._1701334263.pdf`
- Arjun Panditrao analysis (Cyril Amarchand, 2021-01-27) —
  `https://corporate.cyrilamarchandblogs.com/2021/01/supreme-court-on-the-admissibility-of-electronic-evidence-under-section-65b-of-the-evidence-act/`
- Arjun Panditrao analysis (Mondaq, 2020-08-07)
- Section 65B conundrum analysis (Mondaq, 2020-08-25)
- Daaman case note on certificate/stage/CDR directions
- *Anvar P.V.* doctrine and BSA Section 63 carryover (Valkya, 2026-05-20) —
  `https://valkya.org/editorial/anvar-pv-v-pk-basheer/`

**Excluded-component evidence**
- Process hollowing T1055.012, MITRE ATT&CK —
  `https://attack.mitre.org/techniques/T1055/012/`
- Process injection T1055, MITRE ATT&CK —
  `https://attack.mitre.org/techniques/T1055/`
- BYOVD-to-EDR-blindness attack-path walkthrough (2026-05-26) —
  `https://www.attackpaths.org/en/paths/edr-byovd-blind-edr`
- BYOVD analysis & detection (Ransom-ISAC blog, 2026-04-03) —
  `https://ransom-isac.org/blog/analysing-and-detecting-byovd/`
- Qilin/Warlock BYOVD industrialization (Druva, 2026-04-23) —
  `https://www.druva.com/blog/weaponizing-trust-byovd`
- Domain fronting T1090.004, MITRE ATT&CK —
  `https://attack.mitre.org/techniques/T1090/004/`
- Proxy T1090, MITRE ATT&CK — `https://attack.mitre.org/techniques/T1090/`
- Domain fronting (CISA eviction-strategies) —
  `https://www.cisa.gov/eviction-strategies-tool/info-attack/T1090.004`
- Polymorphic code T1027.014 Q&A (Security Scientist, 2026-03-22) —
  `https://www.securityscientist.net/blog/12-questions-and-answers-about-polymorphic-code-t1027-014/`
- Polymorphic vs metamorphic malware (pwnsy blog, 2026-07-24) —
  `https://blog.pwnsy.com/what-is-polymorphic-malware`
- Code mutation / polymorphism mechanics (MalwareTech, 2015-03-23) —
  `https://malwaretech.com/2015/03/code-mutation-polymorphism.html`
- Self-mutating malware engineering survey (DEV, 2026-04-11) —
  `https://dev.to/excalibra/the-art-of-self-mutating-malware-36ab`

**Implementation precedents**
- Recursive-descent parsing chapter, *Crafting Interpreters* (Nystrom) —
  `http://craftinginterpreters.com/parsing-expressions.html`
- *Crafting Interpreters* front page (jlox/clox dual interpreters) —
  `https://craftinginterpreters.com/`
- Tree-walk interpreter chapter — `https://craftinginterpreters.com/a-tree-walk-interpreter.html`
- craftinginterpreters repository (munificent) —
  `https://github.com/munificent/craftinginterpreters`
- Landlock kernel documentation (unprivileged LSM, no_new_privs,
  seccomp complement) —
  `https://kernel.org/doc/html/latest/userspace-api/landlock.html`
- Landlock talk slides (seccomp vs Landlock, pledge/Capsicum/AppContainer
  comparison) — `https://landlock.io/talks/2024-09-17_landlock-oss.pdf`
- Unix sandboxing comparison (seccomp/pledge/Capsicum analysis) —
  `https://freebsdfoundation.org/wp-content/uploads/2017/10/A-Comparison-of-Unix-Sandboxing-Techniques.pdf`
- Sandboxing adoption study (arXiv 2405.06447 — arg-filtering rates,
  gmid/seccomp line counts) — `https://arxiv.org/html/2405.06447`
- Landlock kernel docs v7.1 (domains, layers, fd rights) —
  `https://docs.kernel.org/7.1/security/landlock.html`
