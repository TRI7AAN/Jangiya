# Kalki Inventory (Phase 1, Part A — Inventory Only)

> No file was deleted, modified, or moved to produce this inventory.
> `KernJC/` and `difuze/` were not read; they appear below only as
> mandated EXCLUDED rows. `src/` provenance is **RESOLVED** (see §1) —
> no UNRESOLVED rows were needed.

## 1. src/ Provenance Verdict: RESOLVED — User's Own Framework

`src/` is the **user's own Trinetra/Kalki pentesting framework (Kalki
Beta)**, not the SEI IoT security platform. Evidence:

- `prd.md`: "Kalki is a modular pentesting masterclass framework built
  for Kali Linux 2026.2" with Java-centered orchestration, HexStrike
  script execution, V-code catalog, session evidence.
- `dev_brain.md` documents exactly these 9 Java files and their roles;
  `src/Kalki.java`'s header comment documents the same CLI
  (`-pen -hex run <V-XXX>`, `-stat`, `-mind`, `-agr`).
- `schema.md`'s session layout matches `sessions/` on disk byte for
  byte (`<session>.json`, `brain_<session>.md`, artifacts/).
- All 9 files are default-package `java.io/nio/util`-only classes named
  `Kalki{,Agr,Brain,Common,Ide,Json,Pen,Session,Stat}` — a CLI audit
  workbench, bearing no resemblance to SEI Kalki's IoT device/policy
  architecture, which has no V-codes, HexStrike server, or session-brain
  concepts anywhere in this tree.

## 2. Methodology

- Headers scanned for **all 289 shell scripts** (`stat_scripts/` 158,
  `hex_scripts/` 131); full reads for `kalki`, `stat_script/testssl.sh`,
  `stat_scripts/V-001, V-002, V-003, V-011, V-014, V-134, V-158`,
  `hex_scripts/V-001`, plus all top-level docs, data files, session
  samples, and `src/` headers.
- Flag meanings: **Keep** = reusable as-is (annotation candidate);
  **Strip** = dead, duplicate, stub, debug, or unrelated to the six
  forensic domains; **Split** = bundles ≥2 distinct checks (note says
  how); **Excluded** = out-of-scope tool class; **Unresolved** = needs
  user call (none).
- Proposed domains use JOCKY's six: `recon, netforensics,
  hostforensics, timeline, compliance, report`. Nothing in the tree maps
  to `timeline` or `report` (gaps noted in §5).

## 3. Scope Exclusions (Mandated Rows)

| Path | Apparent purpose | Proposed domain | Flag |
| ---- | ---------------- | --------------- | ---- |
| `KernJC/` | EXCLUDED — kernel vulnerability research tooling, out of scope per AGENTS.md safety boundary | — | Excluded |
| `difuze/` | EXCLUDED — kernel vulnerability research tooling, out of scope per AGENTS.md safety boundary | — | Excluded |

## 4. Entry Points and Shared Helpers

| Path | Apparent purpose | Proposed domain | Flag |
| ---- | ---------------- | --------------- | ---- |
| `kalki` | 5-line wrapper launching the Kalki Beta Java framework from `/home/kali/Desktop/Kalki_v`; JOCKY's toolchain is `jocky`/`jockyc`, not this | — | Strip |
| `stat_script/testssl.sh` | Shared TLS-scanner wrapper (contract `<target> <session_output_dir>`), referenced by V-006/V-007 headers; linkage by PATH, unverified in Phase 1 | compliance | Keep |

## 5. stat_scripts/ — Full Catalog (V-001–V-158)

Conventions observed: `bash V-NNN.sh <target> <session_output_dir>`;
thin tool wrappers with stdout passthrough; `*_probe` scripts are
custom passive checks; `PEN_REQ` stubs shell out to manual pen flow
with no test logic of their own.

| Path | Apparent purpose | Proposed domain | Flag |
| ---- | ---------------- | --------------- | ---- |
| `stat_scripts/V-001.sh` | OSINT via theHarvester | recon | Keep |
| `stat_scripts/V-002.sh` | Subdomain enum via subfinder | recon | Keep |
| `stat_scripts/V-003.sh` | DNS resolution + 5-phase nmap scan in one file | recon | Split (dns_resolve vs port_scan) |
| `stat_scripts/V-004.sh` | DNS spoofing check via dig + custom probe | recon | Keep |
| `stat_scripts/V-005.sh` | Banner grabbing via nmap -sV | recon | Keep |
| `stat_scripts/V-006.sh` | Weak TLS version (testssl/openssl/nmap fallback chain, one check) | compliance | Keep |
| `stat_scripts/V-007.sh` | Weak cipher suites (same fallback pattern, one check) | compliance | Keep |
| `stat_scripts/V-008.sh` | Cert validity via openssl | compliance | Keep |
| `stat_scripts/V-009.sh` | SSL pinning check via sslscan | compliance | Keep |
| `stat_scripts/V-010.sh` | HSTS header via curl | compliance | Keep |
| `stat_scripts/V-011.sh` | 10 distinct MITM tests (TLS downgrade, HSTS, CT, ARP×3, DNS×2, mixed content, promiscuous mode) in one 225-line file | recon / hostforensics | Split (tls_http_indicators, arp_anomaly, dns_consistency) |
| `stat_scripts/V-012.sh` | Default-credential login via hydra (active credential attack) | — | Strip |
| `stat_scripts/V-013.sh` | Passive registration-page policy analysis via curl | compliance | Keep |
| `stat_scripts/V-014.sh` | PEN_REQ stub (MFA bypass, manual flow, no logic) | — | Strip |
| `stat_scripts/V-015.sh` | PEN_REQ stub (ATO, no logic) | — | Strip |
| `stat_scripts/V-016.sh` | Breach-list passive check bundled with hydra active probing (header says both) | compliance | Split (passive breach-list check vs active hydra half, latter Strip) |
| `stat_scripts/V-017.sh` | Passive reset-flow analysis bundled with hydra token testing (header says both) | compliance | Split (passive flow analysis vs active token testing, latter Strip) |
| `stat_scripts/V-018.sh` | Session-fixation comparison via curl | compliance | Keep |
| `stat_scripts/V-019.sh` | Active JWT bypass via jwt_tool | — | Strip |
| `stat_scripts/V-020.sh` | PEN_REQ stub (OAuth, no logic) | — | Strip |
| `stat_scripts/V-021.sh` | Active SAML bypass via xmlsec1 | — | Strip |
| `stat_scripts/V-022.sh` | PEN_REQ stub (BAC, no logic) | — | Strip |
| `stat_scripts/V-023.sh` | Active IDOR enumeration | — | Strip |
| `stat_scripts/V-024.sh` | PEN_REQ stub (BOLA, no logic) | — | Strip |
| `stat_scripts/V-025.sh` | PEN_REQ stub (BFLA, no logic) | — | Strip |
| `stat_scripts/V-026.sh` | PEN_REQ stub (BOPLA, no logic) | — | Strip |
| `stat_scripts/V-027.sh` | PEN_REQ stub (vertical privesc, no logic) | — | Strip |
| `stat_scripts/V-028.sh` | PEN_REQ stub (horizontal privesc, no logic) | — | Strip |
| `stat_scripts/V-029.sh` | Active SQLi via sqlmap | — | Strip |
| `stat_scripts/V-030.sh` | Active NoSQLi via nosqlmap | — | Strip |
| `stat_scripts/V-031.sh` | Active LDAPi payload probe | — | Strip |
| `stat_scripts/V-032.sh` | Active XPATHi payload probe | — | Strip |
| `stat_scripts/V-033.sh` | Active CMDi via commix | — | Strip |
| `stat_scripts/V-034.sh` | Active reflected XSS via xsstrike | — | Strip |
| `stat_scripts/V-035.sh` | Active stored XSS via xsstrike | — | Strip |
| `stat_scripts/V-036.sh` | Active DOM XSS via dalfox | — | Strip |
| `stat_scripts/V-037.sh` | Active XXE payload probe | — | Strip |
| `stat_scripts/V-038.sh` | Active SSTI via tplmap | — | Strip |
| `stat_scripts/V-039.sh` | Active CSTI payload probe | — | Strip |
| `stat_scripts/V-040.sh` | Active SSI payload probe | — | Strip |
| `stat_scripts/V-041.sh` | Active ESI payload probe | — | Strip |
| `stat_scripts/V-042.sh` | Active XSLT injection probe | — | Strip |
| `stat_scripts/V-043.sh` | Active host-header injection probe | — | Strip |
| `stat_scripts/V-044.sh` | Active HPP probe | — | Strip |
| `stat_scripts/V-045.sh` | Active CRLF injection probe | — | Strip |
| `stat_scripts/V-046.sh` | Active CSRF PoC probe | — | Strip |
| `stat_scripts/V-047.sh` | Active SSRF callback probe | — | Strip |
| `stat_scripts/V-048.sh` | Active open-redirect probe | — | Strip |
| `stat_scripts/V-049.sh` | Active HRS/desync via smuggler | — | Strip |
| `stat_scripts/V-050.sh` | Passive CORS header check | compliance | Keep |
| `stat_scripts/V-051.sh` | WAF fingerprinting via wafw00f (non-exploitative) | recon | Keep |
| `stat_scripts/V-052.sh` | Active ReDoS input probe | — | Strip |
| `stat_scripts/V-053.sh` | PEN_REQ stub (business logic, no logic) | — | Strip |
| `stat_scripts/V-054.sh` | Parameter discovery via arjun (non-exploitative enum) | recon | Keep |
| `stat_scripts/V-055.sh` | GraphQL introspection via graphql-cop (non-exploitative enum) | recon | Keep |
| `stat_scripts/V-056.sh` | Fetch via curl bundled with python3 entropy analysis (header says both) | compliance | Split (fetch vs token_entropy_analysis, latter Keep) |
| `stat_scripts/V-057.sh` | Hardcoded-secret scan via trufflehog | hostforensics | Keep |
| `stat_scripts/V-058.sh` | Sensitive-data log grep probe | hostforensics | Keep |
| `stat_scripts/V-059.sh` | Mobile static analysis via mobsf | hostforensics | Keep |
| `stat_scripts/V-060.sh` | GPP password extraction via impacket (credential attack) | — | Strip |
| `stat_scripts/V-061.sh` | DPAPI backup-key extraction via pypykatz (credential attack; safety-aligned Strip) | — | Strip |
| `stat_scripts/V-062.sh` | Active LFI via ffuf payloads | — | Strip |
| `stat_scripts/V-063.sh` | Active RFI payload probe | — | Strip |
| `stat_scripts/V-064.sh` | Active upload-bypass probe | — | Strip |
| `stat_scripts/V-065.sh` | Exploit payloads via ysoserial | — | Strip |
| `stat_scripts/V-066.sh` | Exploit payloads via phpggc | — | Strip |
| `stat_scripts/V-067.sh` | Path traversal discovery via dirsearch (wordlist enum, non-payload) | recon | Keep |
| `stat_scripts/V-068.sh` | PEN_REQ stub (RCE, no logic) | — | Strip |
| `stat_scripts/V-069.sh` | On-host enum via peass-ng (gated local audit) | hostforensics | Keep |
| `stat_scripts/V-070.sh` | Full vuln scan via openvas (heavy active scanner, not forensic analysis) | — | Strip |
| `stat_scripts/V-071.sh` | Admin-interface discovery via nmap+httpx (one check) | recon | Keep |
| `stat_scripts/V-072.sh` | PEN_REQ stub (VM escape, no logic) | — | Strip |
| `stat_scripts/V-073.sh` | Permission enum via linpeas (gated local audit; merge candidate with V-074) | hostforensics | Keep |
| `stat_scripts/V-074.sh` | Cron-task enum via linpeas (gated local audit; merge candidate with V-073) | hostforensics | Keep |
| `stat_scripts/V-075.sh` | PEN_REQ stub (stack overflow, no logic) | — | Strip |
| `stat_scripts/V-076.sh` | PEN_REQ stub (heap overflow, no logic) | — | Strip |
| `stat_scripts/V-077.sh` | Kernel UAF triage via kasan+syzkaller (kernel-space, not forensic analysis) | — | Strip |
| `stat_scripts/V-078.sh` | Kernel double-free triage via kasan+syzkaller | — | Strip |
| `stat_scripts/V-079.sh` | Kernel OOB-read triage via kasan+syzkaller | — | Strip |
| `stat_scripts/V-080.sh` | Kernel OOB-write triage via kasan+syzkaller | — | Strip |
| `stat_scripts/V-081.sh` | Kernel NPD triage via syzkaller | — | Strip |
| `stat_scripts/V-082.sh` | Dev-time leak check via valgrind (not forensic analysis) | — | Strip |
| `stat_scripts/V-083.sh` | Static format-string audit via flawfinder | compliance | Keep |
| `stat_scripts/V-084.sh` | Kernel race detection via kcsan | — | Strip |
| `stat_scripts/V-085.sh` | PEN_REQ stub (TOCTOU, no logic) | — | Strip |
| `stat_scripts/V-086.sh` | Binary hardening check via checksec | compliance | Keep |
| `stat_scripts/V-087.sh` | SCA via grype+syft (one check) | compliance | Keep |
| `stat_scripts/V-088.sh` | SBOM inventory via syft | compliance | Keep |
| `stat_scripts/V-089.sh` | ARP spoofing via bettercap (active MITM tool; safety-aligned Strip) | — | Strip |
| `stat_scripts/V-090.sh` | LLMNR poisoning via responder (credential capture; safety-aligned Strip) | — | Strip |
| `stat_scripts/V-091.sh` | NBT-NS poisoning via responder (safety-aligned Strip) | — | Strip |
| `stat_scripts/V-092.sh` | WPAD spoofing via responder (safety-aligned Strip) | — | Strip |
| `stat_scripts/V-093.sh` | SMB relay via netexec+ntlmrelayx (credential attack; safety-aligned Strip) | — | Strip |
| `stat_scripts/V-094.sh` | Kerberoasting via impacket (credential attack; safety-aligned Strip) | — | Strip |
| `stat_scripts/V-095.sh` | AS-REP roasting via impacket (safety-aligned Strip) | — | Strip |
| `stat_scripts/V-096.sh` | Pass-the-Hash via netexec (safety-aligned Strip) | — | Strip |
| `stat_scripts/V-097.sh` | Pass-the-Ticket via impacket (safety-aligned Strip) | — | Strip |
| `stat_scripts/V-098.sh` | Pass-the-Key via pypykatz (safety-aligned Strip) | — | Strip |
| `stat_scripts/V-099.sh` | DCSync via impacket_secretsdump (safety-aligned Strip) | — | Strip |
| `stat_scripts/V-100.sh` | PEN_REQ stub (DCShadow, no logic) | — | Strip |
| `stat_scripts/V-101.sh` | Silver-ticket forgery via impacket_ticketer (safety-aligned Strip) | — | Strip |
| `stat_scripts/V-102.sh` | Golden-ticket forgery via impacket_ticketer (safety-aligned Strip) | — | Strip |
| `stat_scripts/V-103.sh` | ADCS abuse via certipy (credential attack; safety-aligned Strip) | — | Strip |
| `stat_scripts/V-104.sh` | VLAN hopping via yersinia (active L2 attack) | — | Strip |
| `stat_scripts/V-105.sh` | Segmentation validation probe | compliance | Keep |
| `stat_scripts/V-106.sh` | Public cloud-storage exposure via cloud_enum | compliance | Keep |
| `stat_scripts/V-107.sh` | IAM/policy review via scoutsuite | compliance | Keep |
| `stat_scripts/V-108.sh` | Security-group review via scoutsuite | compliance | Keep |
| `stat_scripts/V-109.sh` | IMDS exposure probe via curl | compliance | Keep |
| `stat_scripts/V-110.sh` | CSPM review via prowler | compliance | Keep |
| `stat_scripts/V-111.sh` | PEN_REQ stub (D-PPE, no logic) | — | Strip |
| `stat_scripts/V-112.sh` | PEN_REQ stub (I-PPE, no logic) | — | Strip |
| `stat_scripts/V-113.sh` | CI/CD secret scan via trufflehog | compliance | Keep |
| `stat_scripts/V-114.sh` | Supply-chain verify+inventory via cosign+syft (one verdict) | compliance | Keep |
| `stat_scripts/V-115.sh` | PEN_REQ stub (DLL hijacking, no logic) | — | Strip |
| `stat_scripts/V-116.sh` | PEN_REQ stub (UAC bypass, no logic) | — | Strip |
| `stat_scripts/V-117.sh` | PEN_REQ stub (EDR bypass, no logic; safety-aligned Strip) | — | Strip |
| `stat_scripts/V-118.sh` | JDWP exposure check via nmap (non-exploitative) | recon | Keep |
| `stat_scripts/V-119.sh` | Dynamic mobile exploitation via drozer | — | Strip |
| `stat_scripts/V-120.sh` | PEN_REQ stub (deep-link hijacking, no logic) | — | Strip |
| `stat_scripts/V-121.sh` | PEN_REQ stub (Frida bypass, no logic) | — | Strip |
| `stat_scripts/V-122.sh` | Dynamic app instrumentation via objection | — | Strip |
| `stat_scripts/V-123.sh` | Static mobile analysis via jadx+apktool | hostforensics | Keep |
| `stat_scripts/V-124.sh` | PEN_REQ stub (DoS, no logic; DoS tooling never enters registry) | — | Strip |
| `stat_scripts/V-125.sh` | PEN_REQ stub (DDoS, no logic; same) | — | Strip |
| `stat_scripts/V-126.sh` | Active algorithmic-complexity probe | — | Strip |
| `stat_scripts/V-127.sh` | Active brute-force via hydra (credential attack) | — | Strip |
| `stat_scripts/V-128.sh` | Active LLM prompt-injection via garak (not forensic analysis) | — | Strip |
| `stat_scripts/V-129.sh` | Active LLM jailbreak via garak | — | Strip |
| `stat_scripts/V-130.sh` | Active model-output probing via garak | — | Strip |
| `stat_scripts/V-131.sh` | PEN_REQ stub (training poisoning, no logic) | — | Strip |
| `stat_scripts/V-132.sh` | Coverage-guided kernel fuzzing via syzkaller | — | Strip |
| `stat_scripts/V-133.sh` | Syscall fuzzing via syzkaller | — | Strip |
| `stat_scripts/V-134.sh` | Driver-targeted fuzzing; depends on out-of-scope kernel-fuzzing harness class (excluded-path contents not read) | — | Excluded |
| `stat_scripts/V-135.sh` | KASAN crash triage (kernel-space) | — | Strip |
| `stat_scripts/V-136.sh` | UBSAN dev-time detection (not forensic analysis) | — | Strip |
| `stat_scripts/V-137.sh` | Kernel race detection via kcsan+lockdep | — | Strip |
| `stat_scripts/V-138.sh` | PEN_REQ stub (property testing, no logic) | — | Strip |
| `stat_scripts/V-139.sh` | Dev-time runtime-verification harness (not forensic analysis) | — | Strip |
| `stat_scripts/V-140.sh` | Formal kernel memory-model analysis (not forensic analysis) | — | Strip |
| `stat_scripts/V-141.sh` | eBPF-verifier fuzzing via syz-manager (header) / `agni` (classification CSV) — header-vs-catalog divergence noted §6 | — | Strip |
| `stat_scripts/V-142.sh` | Active overflow brute-forcing via bfbtester | — | Strip |
| `stat_scripts/V-143.sh` | Static source audit via flawfinder+cppcheck | compliance | Keep |
| `stat_scripts/V-144.sh` | Kernel-hardening config audit (no exploitation) | compliance | Keep |
| `stat_scripts/V-145.sh` | NX/exec-shield verification via checksec+paxtest (one verdict) | compliance | Keep |
| `stat_scripts/V-146.sh` | PaX/grsecurity *bypass* testing (active bypass; safety-aligned Strip — contrast V-145) | — | Strip |
| `stat_scripts/V-147.sh` | PEN_REQ stub (KASLR bypass, no logic; safety-aligned Strip) | — | Strip |
| `stat_scripts/V-148.sh` | PEN_REQ stub (SMEP/SMAP bypass, no logic; safety-aligned Strip) | — | Strip |
| `stat_scripts/V-149.sh` | Kernel module-loading probe (kernel-space; safety-adjacent Strip) | — | Strip |
| `stat_scripts/V-150.sh` | PEN_REQ stub (netlink abuse, no logic; safety-adjacent Strip) | — | Strip |
| `stat_scripts/V-151.sh` | PEN_REQ stub (container escape, no logic; safety-adjacent Strip) | — | Strip |
| `stat_scripts/V-152.sh` | PEN_REQ stub (seccomp bypass, no logic; safety-adjacent Strip) | — | Strip |
| `stat_scripts/V-153.sh` | io_uring abuse via syzkaller harness | — | Strip |
| `stat_scripts/V-154.sh` | PEN_REQ stub (signal race, no logic) | — | Strip |
| `stat_scripts/V-155.sh` | Filesystem-layer fuzzing harness | — | Strip |
| `stat_scripts/V-156.sh` | PEN_REQ stub (symlink race, no logic) | — | Strip |
| `stat_scripts/V-157.sh` | Live-patch signature verification probe | compliance | Keep |
| `stat_scripts/V-158.sh` | CVE-regression flow; depends on out-of-scope kernel-environment tooling class (excluded-path contents not read) | — | Excluded |

Summary: 158 scripts → 41 Keep, 5 Split, 110 Strip, 2 Excluded (+1 Keep shared helper).

## 6. hex_scripts/ — Duplicate Harness (V-001–V-131, All Strip)

Same V-code numbering as `stat_scripts/` V-001–V-131 with divergent
tool choices per code (e.g., hex V-005 uses whatweb where stat V-005
uses nmap -sV; hex V-006/V-007 use sslscan where stat uses testssl.sh;
hex V-014+ collapse manual-flow tests into nuclei templates). All 131
route through the HexStrike localhost API and append session JSON with
side effects — coupled to the Kalki Beta runtime, unusable as pure
`call` functions. Disposition: **Strip the entire directory as
duplicate-purpose**; canonicalize to one implementation per V-code in
Phase 2/9 (stat-style direct invocation recommended). Representative
rows below; the full per-file header list was scanned and follows the
same pattern throughout (Discovery/Web/API: V-001–V-067, Host/Software:
V-068–V-105, Cloud/Client/LLM: V-106–V-131).

| Path | Apparent purpose | Proposed domain | Flag |
| ---- | ---------------- | --------------- | ---- |
| `hex_scripts/V-001.sh` | OSINT via theharvester (dup of stat V-001) | recon | Strip |
| `hex_scripts/V-002.sh` | Subdomain enum via subfinder (dup of stat V-002) | recon | Strip |
| `hex_scripts/V-003.sh` | Port scan via nmap (dup of stat V-003) | recon | Strip |
| `hex_scripts/V-004.sh` | DNS check via dnsenum (diverges: stat uses dig+probe) | recon | Strip |
| `hex_scripts/V-005.sh` | Banner grab via whatweb (diverges: stat uses nmap -sV) | recon | Strip |
| `hex_scripts/V-006.sh` | TLS version via sslscan (diverges: stat uses testssl.sh) | compliance | Strip |
| `hex_scripts/V-007.sh` | Cipher suites via sslscan (diverges: stat uses testssl.sh) | compliance | Strip |
| `hex_scripts/V-008.sh` | Cert check via openssl (same tool as stat V-008) | compliance | Strip |
| `hex_scripts/V-009.sh` | Pinning check via sslscan (diverges: stat uses sslscan+openssl) | compliance | Strip |
| `hex_scripts/V-010.sh` | HSTS via curl (same tool as stat V-010) | compliance | Strip |
| `hex_scripts/V-011.sh` | MITM via sslscan (diverges: stat V-011 is a 10-test custom script) | recon | Strip |
| `hex_scripts/V-012.sh`–`V-067.sh` | Web/API checks V-012–V-067 (hydra/nuclei/ffuf/dalfox/sqlmap per code; same purposes as stat V-012–V-067, HexStrike-coupled) | recon / compliance | Strip |
| `hex_scripts/V-068.sh`–`V-105.sh` | Host checks V-068–V-105 (nuclei/netexec/impacket/grype/syft/responder per code; same purposes as stat V-068–V-105, HexStrike-coupled) | hostforensics / compliance | Strip |
| `hex_scripts/V-106.sh`–`V-131.sh` | Cloud/LLM checks V-106–V-131 (cloud_enum/nuclei/garak/frida per code; same purposes as stat V-106–V-131, HexStrike-coupled) | compliance | Strip |

## 7. Non-Script Material

| Path | Apparent purpose | Proposed domain | Flag |
| ---- | ---------------- | --------------- | ---- |
| `src/Kalki{,Agr,Brain,Common,Ide,Json,Pen,Session,Stat}.java` (9 files) | Kalki Beta Java orchestrator (provenance resolved §1); JOCKY is C++20, nothing to port | — | Strip |
| `sessions/demo,test1,ses27_07_26,insta-29-7-26/` | Generated Kalki run artifacts (session JSON, run logs, brain MDs, raw outputs incl. a live run against app.indiapost.gov.in); no shell scripts present | — | Strip |
| `out/*.class` | Compiled Kalki build artifacts | — | Strip |
| `output/` | Empty directory | — | Strip |
| `Makefile` | Kalki/HexStrike lifecycle (compile, install, server run/stop) | — | Strip |
| `LICENSE.txt` | License boilerplate (noted, untouched) | — | Strip |
| `prd.md, design.md, dataflow.md, dev_brain.md, rules.md, schema.md, techspec.md, skills.md` | Kalki Beta project docs (JOCKY docs live in `wiki/`) | — | Strip |
| `prompt.txt` | Operator working note on V-017 robustness iteration | — | Strip |
| `brain_state.json` | Kalki global runtime state | — | Strip |
| `_quarantine/1_classification.csv` | V-code taxonomy (158 codes: tool_parsing/custom_script/pen_req + severities) — was reference data for Phase 2/9 annotation; moved unmodified 2026-09-12, see §13 | — | QUARANTINED |
| `_quarantine/2_static_map.json` | Decision rules per V-code (method, pass/fail criteria, severity) — was reference data for input/output schema design; moved unmodified 2026-09-12, see §13 | — | QUARANTINED |
| `_quarantine/3_decision_engine.csv` | Same rules in tabular form — was reference data; moved unmodified 2026-09-12, see §13 | — | QUARANTINED |

## 8. Data-Quality Findings (Phase 2 Must Reconcile)

1. Header-vs-catalog drift: `1_classification.csv` lists V-003 as
   `rustscan+nmap` (script uses nmap only), V-009 as `pen_req`
   (script implements sslscan+openssl), V-141 as `agni` (script uses
   syzkaller). The CSV is stale relative to the scripts for these codes.
2. PEN_REQ count: 34 stub files on disk vs "35" claimed in
   `dev_brain.md` — off-by-one to resolve, not assumed.
3. `prompt.txt` shows a V-017 runtime variant referencing
   `token_entropy_probe` ("Tool not found") while the on-disk V-017
   header claims curl+hydra — scripts are mid-iteration; Phase 2
   validation must execute, not just read.
4. `hex_scripts/` arg order (`<session> <target>`) is reversed vs
   `stat_scripts/` (`<target> <session_output_dir>`) — canonicalize to
   one contract before annotating. (Update 2026-09-12: `hex_scripts/`
   pruned; stat-style contract stands.)
5. Seven `@jocky:` headers exist (6 netforensics + testssl wrapper);
   the 41 curated scripts remain unannotated pending Phase 9.
   (Updated 2026-09-12; supersedes the original "none yet" note.)

## 9. Phase 2 Handoff

- 41 Keep scripts (+1 shared helper) are annotation candidates (recon passive tooling,
  compliance checks, host-forensic static analysis).
- 5 Split scripts need decomposition before annotation (V-003, V-011,
  V-016, V-017, V-056; V-073/V-074 are merge candidates, not splits).
- Credential-attack, exploit-confirm, kernel-fuzzing, DoS, and bypass
  classes are Stripped — several on safety alignment, all logged above.
- `timeline` and `report` domains have zero incoming scripts; both will
  need greenfield stdlib in Phase 9.
- The 3 Keep data files seed the registry index (names, categories,
  severities, eval methods → `@jocky:` inputs/outputs/capability drafts).

## 10. Prune Record (2026-09-12, per user instruction)

Tree reduced from ~13,827 files to 66, keeping only problem-statement
material: JOCKY core docs/code (`AGENTS.md`, `roadmap.md`,
`implementationplan.md`, `logs.md`, `README.md`, `wiki/`,
`include/`, `src/cli/main.cpp`, `CMakeLists.txt`, `samples/`) plus
registry material (41 Keep scripts, `stat_script/testssl.sh`,
`1_classification.csv`, `2_static_map.json`, `3_decision_engine.csv`).
Removed: `KernJC/`, `difuze/` (contents never read), `hex_scripts/`,
110 Strip + 5 Split + 2 Excluded `stat_scripts/`, `sessions/`,
`out/`, `output/`, `Makefile`, `LICENSE.txt` (unfilled template),
`kalki` wrapper, `src/*.java`, all Kalki docs/state, `build/`
(regenerated to verify, then removed). Full pre-prune backup:
`/tmp/jangiya-prune-backup-2026-09-12.tar.gz` (14,024 entries).
Post-prune clean rebuild + `jocky check samples/sample.jky` passed
before `build/` removal. Split-script logic survives only in the
backup — Phase 2 decomposition must re-derive V-003/011/016/017/056
from backup or reimplementation.

## 11. Rename Record (Phase 2 Canonicalization)

All 41 Keep scripts renamed to `jky_<domain>_<verb>_<object>.sh` and
moved into per-domain subdirectories; contents untouched (SHA-256
content digest identical before/after:
`48f6d2ee…e584d`). The shared helper moved unchanged to
`stat_scripts/shared/testssl.sh`. `@jocky:function` names for Phase 2
annotation are the basenames minus `.sh`. The CSV/JSON reference files
still use V-codes — this table is the reconciliation key (open item
`wiki/09 §8`).

| Old path | New path |
| -------- | -------- |
| `stat_scripts/V-001.sh` | `stat_scripts/recon/jky_recon_gather_osint.sh` |
| `stat_scripts/V-002.sh` | `stat_scripts/recon/jky_recon_enum_subdomains.sh` |
| `stat_scripts/V-004.sh` | `stat_scripts/recon/jky_recon_detect_dns_spoof.sh` |
| `stat_scripts/V-005.sh` | `stat_scripts/recon/jky_recon_grab_banners.sh` |
| `stat_scripts/V-051.sh` | `stat_scripts/recon/jky_recon_detect_waf.sh` |
| `stat_scripts/V-054.sh` | `stat_scripts/recon/jky_recon_discover_params.sh` |
| `stat_scripts/V-055.sh` | `stat_scripts/recon/jky_recon_audit_graphql.sh` |
| `stat_scripts/V-067.sh` | `stat_scripts/recon/jky_recon_enum_paths.sh` |
| `stat_scripts/V-071.sh` | `stat_scripts/recon/jky_recon_discover_admin.sh` |
| `stat_scripts/V-118.sh` | `stat_scripts/recon/jky_recon_check_jdwp.sh` |
| `stat_scripts/V-006.sh` | `stat_scripts/compliance/jky_compliance_check_tls_version.sh` |
| `stat_scripts/V-007.sh` | `stat_scripts/compliance/jky_compliance_check_ciphers.sh` |
| `stat_scripts/V-008.sh` | `stat_scripts/compliance/jky_compliance_check_cert.sh` |
| `stat_scripts/V-009.sh` | `stat_scripts/compliance/jky_compliance_check_pinning.sh` |
| `stat_scripts/V-010.sh` | `stat_scripts/compliance/jky_compliance_check_hsts.sh` |
| `stat_scripts/V-013.sh` | `stat_scripts/compliance/jky_compliance_check_password_policy.sh` |
| `stat_scripts/V-018.sh` | `stat_scripts/compliance/jky_compliance_check_session_fixation.sh` |
| `stat_scripts/V-050.sh` | `stat_scripts/compliance/jky_compliance_check_cors.sh` |
| `stat_scripts/V-083.sh` | `stat_scripts/compliance/jky_compliance_check_format_strings.sh` |
| `stat_scripts/V-086.sh` | `stat_scripts/compliance/jky_compliance_check_binary_hardening.sh` |
| `stat_scripts/V-087.sh` | `stat_scripts/compliance/jky_compliance_scan_dependencies.sh` |
| `stat_scripts/V-088.sh` | `stat_scripts/compliance/jky_compliance_build_sbom.sh` |
| `stat_scripts/V-105.sh` | `stat_scripts/compliance/jky_compliance_verify_segmentation.sh` |
| `stat_scripts/V-106.sh` | `stat_scripts/compliance/jky_compliance_enum_cloud_storage.sh` |
| `stat_scripts/V-107.sh` | `stat_scripts/compliance/jky_compliance_audit_iam.sh` |
| `stat_scripts/V-108.sh` | `stat_scripts/compliance/jky_compliance_audit_secgroups.sh` |
| `stat_scripts/V-109.sh` | `stat_scripts/compliance/jky_compliance_check_imds.sh` |
| `stat_scripts/V-110.sh` | `stat_scripts/compliance/jky_compliance_audit_cloud.sh` |
| `stat_scripts/V-113.sh` | `stat_scripts/compliance/jky_compliance_scan_build_secrets.sh` |
| `stat_scripts/V-114.sh` | `stat_scripts/compliance/jky_compliance_verify_supply_chain.sh` |
| `stat_scripts/V-143.sh` | `stat_scripts/compliance/jky_compliance_audit_code.sh` |
| `stat_scripts/V-144.sh` | `stat_scripts/compliance/jky_compliance_audit_kernel_config.sh` |
| `stat_scripts/V-145.sh` | `stat_scripts/compliance/jky_compliance_check_nx.sh` |
| `stat_scripts/V-157.sh` | `stat_scripts/compliance/jky_compliance_verify_patches.sh` |
| `stat_scripts/V-057.sh` | `stat_scripts/hostforensics/jky_hostforensics_scan_secrets.sh` |
| `stat_scripts/V-058.sh` | `stat_scripts/hostforensics/jky_hostforensics_scan_logs.sh` |
| `stat_scripts/V-059.sh` | `stat_scripts/hostforensics/jky_hostforensics_check_app_storage.sh` |
| `stat_scripts/V-069.sh` | `stat_scripts/hostforensics/jky_hostforensics_enum_privesc.sh` |
| `stat_scripts/V-073.sh` | `stat_scripts/hostforensics/jky_hostforensics_audit_permissions.sh` |
| `stat_scripts/V-074.sh` | `stat_scripts/hostforensics/jky_hostforensics_audit_cron.sh` |
| `stat_scripts/V-123.sh` | `stat_scripts/hostforensics/jky_hostforensics_check_apk_hardening.sh` |
| `stat_script/testssl.sh` | `stat_scripts/shared/testssl.sh` (name unchanged — already functional) |

## 12. NetForensics Greenfield Batch (Phase 2, per user request)

Six new scripts close the `netforensics` gap from §9 — the first
registry functions written directly in `jky_` form, each carrying a
complete `@jocky:` header (function, domain, description, inputs,
outputs, capability, timeout_seconds, depends_on) per AGENTS.md §3:

| Function (`stat_scripts/netforensics/`) | Purpose | depends_on |
| -------------------------------------- | ------- | ---------- |
| `jky_netforensics_extract_flows.sh` | tshark→CSV flow extraction (5-tuple, bytes, timestamps; BPF via tcpdump pre-filter since `tshark -r` takes display filters only) | — |
| `jky_netforensics_extract_dns.sh` | DNS query/response extraction with type/rcode name maps | — |
| `jky_netforensics_extract_http.sh` | HTTP method/URI/host/UA/status extraction | — |
| `jky_netforensics_top_talkers.sh` | Endpoint-pair ranking by bytes | extract_flows |
| `jky_netforensics_detect_beaconing.sh` | Inter-arrival CV analysis per (src,dst,port,proto); PERIODIC/OK verdicts | extract_flows |
| `jky_netforensics_check_dns_anomalies.sh` | TXT-ratio + NXDOMAIN-burst verdicts (TUNNEL/DGA_SUSPECT/OK) | extract_dns |

Verified against a synthetic 28-packet pcap (DNS A+TXT, HTTP GET,
ICMP, 12×60s TCP beacons): extractors emit correct CSVs, beaconing
flags the 60s series (cv=0.0, PERIODIC), DNS check flags the TXT
fixture (TUNNEL_SUSPECT), BPF slicing and all arg-validation paths
return the documented exit codes. Two real bugs found and fixed in
testing: `python3 -` heredoc stdin conflict (now `/dev/fd/3`) and
`tshark -r -f` rejection (now tcpdump pre-filter). `1_classification.csv`
(+6 rows) and `2_static_map.json` (+6 entries, validated by parse)
updated. `timeline`/`report` domains remain greenfield.

## 13. Quarantine Notice (2026-09-12, per user instruction)

`1_classification.csv`, `2_static_map.json`, and
`3_decision_engine.csv` were moved unmodified (SHA-256 digests
identical before/after) from the repo root to `_quarantine/`, which
carries its own README: origin unconfirmed, not referenced by the
registry, scanner, or any wiki doc, do-not-integrate pending owner
confirmation. `scan_registry` skips `_quarantine/` at any depth by
design, so the quarantine is enforced in code, not just policy.

Consequences for this document:

- §7 rows for the three files are re-flagged QUARANTINED (history
  preserved, paths updated). They are no longer registry reference
  data.
- §8 items 1–3 cite quarantined or pruned files (CSV drift, PEN_REQ
  count vs `dev_brain.md`, `prompt.txt` variant). Drift reconciliation
  is **suspended** until origin is confirmed — reconciling against
  quarantined data would integrate it through the back door.
- §9's "3 Keep data files seed the registry index" and §12's
  "+6 rows/entries" notes are historical: the seeded content now lives
  under quarantine. The six netforensics functions' headers remain
  valid on their own (self-describing scripts); only the bulk-seeding
  path is suspended.
- `README.md` repo map and `wiki/10 §6` describe the quarantine state;
  no wiki doc treats quarantined content as authoritative.
