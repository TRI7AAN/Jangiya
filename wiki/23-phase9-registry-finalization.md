# Phase 9 Record: Registry Finalization (Headers, Starters, Smoke)

> Completed: 2026-09-13. Platform scope: Linux/WSL (unchanged).
> Build: `cmake -S . -B /tmp/p9build`, `cmake --build` — clean, zero
> warnings/errors (g++ 15.2.0). Starting evidence: `wiki/09`
> per-script purpose notes; every header below was additionally derived
> from a full read of the actual script body in this session (arguments,
> behavior, outputs), with three parallel analysis passes cross-checked
> against the sources before writing.
>
> Registry census after this session: **38 registered / 0 rejected /
> 16 skipped** (was 7/0/42). Per-domain: recon 10, netforensics 6,
> hostforensics 2, timeline 3, compliance 15, report 2.

## 1. PART A — annotation table (26 annotated of 41)

Convention for every header: `target`/`session_dir` positional pair
(`session_dir: string = ""`, matching the scripts' `${2:-}` and the
testssl-wrapper precedent); `path` type only where the script operates
on a local path argument; `text` outputs (all 26 print free text);
`depends_on` empty everywhere (verified: no `jky_` cross-reference in
any of the 41 bodies — one repo-wide grep, zero hits). Timeouts bound
worst-case hangs; a dispatcher kill is a first-class logged timeout
outcome, not an annotation failure (see §5).

| Script | Inputs | Outputs | Capability | Timeout | Description |
| ------ | ------ | ------- | ---------- | ------- | ----------- |
| recon/jky_recon_gather_osint | target: string, session_dir: string = "" | osint: text | recon.osint.gather | 300 | Gather target OSINT via theHarvester all-sources scan |
| recon/jky_recon_enum_subdomains | target: string, session_dir: string = "" | subdomains: text | recon.subdomain.enumerate | 300 | Enumerate subdomains via subfinder silent scan |
| recon/jky_recon_detect_dns_spoof | target: string, session_dir: string = "" | verdict: text | recon.dns.analyze | 120 | Test DNS spoof resistance via DNSSEC, consistency, TTL and wildcard checks |
| recon/jky_recon_grab_banners | target: string, session_dir: string = "" | banners: text | recon.banner.grab | 300 | Grab service banners via nmap, curl, netcat and TLS probes |
| recon/jky_recon_detect_waf | target: string, session_dir: string = "" | waf_report: text | recon.waf.detect | 120 | Detect web application firewall via wafw00f |
| recon/jky_recon_discover_params | target: string, session_dir: string = "" | params: text | recon.param.discover | 300 | Discover hidden HTTP parameters via arjun |
| recon/jky_recon_audit_graphql | target: string, session_dir: string = "" | graphql_response: text | recon.graphql.audit | 60 | Probe a GraphQL endpoint with a single introspection query via curl |
| recon/jky_recon_enum_paths | target: string, session_dir: string = "" | paths: text | recon.path.enumerate | 300 | Enumerate web paths via dirsearch wordlist scan |
| recon/jky_recon_discover_admin | target: string, session_dir: string = "" | admin_report: text | recon.admin.discover | 180 | Discover exposed admin interfaces via port scan and path probes |
| recon/jky_recon_check_jdwp | target: string, session_dir: string = "" | jdwp_report: text | recon.jdwp.scan | 300 | Check JDWP exposure on ports 8000-9999 via nmap |
| compliance/jky_compliance_check_tls_version | target: string, session_dir: string = "" | report: text | compliance.tls.scan | 300 | Probe offered SSL/TLS versions via testssl.sh, openssl, nmap and curl |
| compliance/jky_compliance_check_ciphers | target: string, session_dir: string = "" | report: text | compliance.tls.scan | 300 | Probe weak cipher suites via testssl.sh, openssl and nmap |
| compliance/jky_compliance_check_cert | target: string, session_dir: string = "" | report: text | compliance.tls.scan | 180 | Dump remote TLS certificate chain, dates, subject and issuer via openssl |
| compliance/jky_compliance_check_pinning | target: string, session_dir: string = "" | report: text | compliance.tls.scan | 300 | Assess SSL pinning indicators via sslscan, openssl SPKI pin and HTTP headers |
| compliance/jky_compliance_check_hsts | target: string, session_dir: string = "" | report: text | compliance.web.read | 120 | Check HSTS header presence via curl across HTTPS endpoints and HTTP redirect |
| compliance/jky_compliance_check_password_policy | target: string, session_dir: string = "" | report: text | compliance.web.read | 300 | Assess registration password policy via passive curl page and field analysis |
| compliance/jky_compliance_check_session_fixation | target: string, session_dir: string = "" | report: text | compliance.web.audit | 120 | Test session fixation and cookie scope via curl cookie jars and failed-login POST |
| compliance/jky_compliance_check_format_strings | target: path, session_dir: string = "" | report: text | compliance.code.audit | 120 | Scan source tree for format-string flaws via flawfinder |
| compliance/jky_compliance_check_binary_hardening | target: path, session_dir: string = "" | report: text | compliance.host.audit | 60 | Report binary hardening via checksec, or kernel ASLR state for non-file targets |
| compliance/jky_compliance_scan_dependencies | target: path, session_dir: string = "" | report: text | compliance.code.audit | 300 | Scan dependencies for known vulnerabilities via syft and grype |
| compliance/jky_compliance_build_sbom | target: path, session_dir: string = "" | report: text | compliance.code.audit | 180 | Generate software bill of materials via syft truncated JSON |
| compliance/jky_compliance_enum_cloud_storage | target: string, session_dir: string = "" | report: text | compliance.cloud.read | 300 | Enumerate public cloud storage for a keyword via cloud_enum |
| compliance/jky_compliance_scan_build_secrets | target: string, session_dir: string = "" | report: text | compliance.web.read | 60 | Fetch remote .env and git config via curl and print truncated contents |
| compliance/jky_compliance_audit_code | target: path, session_dir: string = "" | report: text | compliance.code.audit | 300 | Audit source tree via flawfinder and cppcheck |
| hostforensics/jky_hostforensics_scan_secrets | target: string, session_dir: string = "" | findings: text | hostforensics.secret.scan | 60 | Fetch remote .env and git HEAD via curl and print truncated contents |
| hostforensics/jky_hostforensics_check_apk_hardening | target: path, session_dir: string = "" | findings: text | hostforensics.app.audit | 120 | Print truncated jadx output for a regular-file target, silently empty otherwise |

Running count: recon 10/10 annotated, compliance 14/24, hostforensics
2/7 — **26 annotated, 15 flagged (§2)**. 26 + 15 = 41 ✓.

Judgment calls recorded (deviations from analysis-pass proposals):
- `grab_banners`, `discover_admin` annotated despite "single capability
  cannot cover multi-tool" flags: purposes are nameable from code, and
  coarse single capabilities match existing precedent
  (`check_tls_version` spans testssl+openssl+nmap+curl under
  `compliance.tls.scan`).
- `audit_graphql` described from CODE (single curl introspection POST),
  not its header comment (`graphql-cop` never invoked).
- `scan_build_secrets` + `scan_secrets` annotated accurately, but both
  print fetched secrets to stdout — which lands in manifests/logs.
  Human must bless that log-handling before operational use (caveat,
  not a blocker: behavior is as described).
- `binary_hardening` keeps one function for both branches with a
  bifurcated description; `compliance.host.audit` (both branches are
  host-local reads).
- Timeouts raised vs proposals in two cases: `check_cert` 60→180 and
  `audit_graphql` kept at 60 — both scripts issue unbounded connects
  (plain `openssl s_client` / `curl` with no per-probe timeout), so the
  dispatcher ceiling is the only bound; values cover worst-case TCP
  timeouts while normal runs finish in seconds.

## 2. NEEDS HUMAN REVIEW (15 left headerless — no guessing)

- `compliance/jky_compliance_check_cors`, `..._verify_segmentation`,
  `..._check_imds`, `..._audit_kernel_config`,
  `..._verify_patches`: pure `which <probe>` stubs (10 lines each).
  They perform no analysis; any header would either lie about behavior
  or register a tool-presence check as a forensic function. Human:
  implement the real check, keep as presence-probe, or drop.
- `compliance/jky_compliance_audit_iam` +
  `..._audit_secgroups`: byte-identical bodies (generic `scout aws`
  scan), divergent names, required `$TARGET` unused by both. Human:
  differentiate the bodies or drop one; decide the unused-arg contract.
- `compliance/jky_compliance_audit_cloud`: `prowler` never receives
  `$TARGET` though it is required. Human: wire or drop the arg.
- `compliance/jky_compliance_verify_supply_chain`: byte-identical to
  `build_sbom`; name promises cosign verification that never runs.
  Human: add cosign verification or drop (keep `build_sbom`).
- `compliance/jky_compliance_check_nx`: `checksec` branch is clean,
  but the unconditional `paxtest blackhat` half is a host-intrusive
  kernel test ignoring `$TARGET`. Human: confirm paxtest-in-registry
  scope.
- `hostforensics/jky_hostforensics_scan_logs`,
  `..._check_app_storage`: `which`-probe stubs (same reasoning as the
  compliance stubs).
- `hostforensics/jky_hostforensics_enum_privesc`,
  `..._audit_permissions`, `..._audit_cron`: byte-identical bodies
  (`find / -perm -4000` + `sudo -l`) under three divergent names, none
  touching cron/permissions specifically, `$TARGET` unused. Deeper
  issue: all three are SELF-probing (they audit the execution machine,
  not an evidence path), so under sandboxing they audit the sandbox —
  misleading in a forensic manifest. Human: keep at most one with an
  accurate name, or drop the class.

## 3. PART B — starter scripts (5 new, pure bash, verified live)

New dirs `stat_scripts/timeline/`, `stat_scripts/report/`. All five
use builtins only (`read`, `printf`, `[[ ]]`, `(( ))`, arrays, `case`);
no external commands whatsoever — verified by running under `bash`
with an empty PATH equivalent (direct host runs) AND through the
sandbox (smoke §4: 5/5 PASS with correct outputs).

| Function | Header (inputs → outputs, cap, timeout) | Logic |
| -------- | --------------------------------------- | ----- |
| `jky_timeline_correlate_events` | `left_csv: path, right_csv: path, window: string = "5m"` → `timeline: text`; `timeline.correlate`; 120 | Pairwise temporal join; window suffix s/m/h/d parsed in bash; non-integer-ts rows (headers) skipped; O(n·m) nested scan |
| `jky_timeline_build_super` | `a_csv: path, b_csv: path` → `supertimeline: text` (`ts,source,host,event`) | Union + manual insertion sort by ts (stable); exactly two inputs — longer chains by repeated application |
| `jky_timeline_extract_window` | `events_csv: path, start_ts: int, end_ts: int` → `window: text` | Inclusive integer-range filter; reversed bounds rejected (exit 1) |
| `jky_report_render_case_summary` | `findings_csv: path, case_id: string = ""` → `summary: text` | Markdown summary; severity tallies by loop counters; detail capped at 20 rows by counter; `finding` header row skipped |
| `jky_report_render_timeline` | `timeline_csv: path, max_rows: int = 20` | Markdown table; non-integer-ts lead row skipped; row cap by counter; non-positive max_rows rejected |

Event-row contract (all five): `ts,host,event` with epoch-integer ts
(ISO strings would need `date -d`, unavailable — documented
constraint, not hidden). Findings contract: `finding,severity,source`.

Bash-constraint workarounds (impact visible, per brief): timestamp
sort → manual insertion sort O(n²), starter-scale only;
counts/tallies → loop counters (no `wc`); row caps → counters (no
`head`); date parsing → epoch-integer contract (no `date`); JSON →
CSV-in/markdown-out via `printf` (no `jq`); temp files avoided
entirely (no `mktemp`).

Unit evidence (direct `bash` runs): correlate pairs
login→allow(100s), upload→allow/download, query unmatched (500s out);
super orders shuffled 5-row input stably; window selects [..0100,
..0400]∩rows and rejects reversed bounds (exit 1); summary counts
4 (1/1/1/1/0) with markdown table; timeline renders max_rows=2 then
`Rendered 2 row(s)`; max_rows=0 rejected (exit 1); empty inputs warn
and exit 0.

## 4. PART C — smoke test (38/38 attempted, 8 PASS / 30 FAIL, 0 unlogged)

Harness: `tests/phase9/run_smoke.sh` (bash; per-function `.jky` with a
case allowing exactly that function, real `jocky` interpreter, real
registry; exit contract: 0 iff every selected function was attempted
with a manifest entry — environment failures are data, not harness
failure) + `tests/phase9/smoke_fixtures.json` (38 entries: required
`path` → committed `tests/phase9/inputs/placeholder.txt`, required
`string` → `example.com`, int/float/bool sentinels, defaulted inputs
omitted to exercise default-fill; timeline/report functions get the
real committed CSVs). Fixture inputs: `tests/phase9/inputs/`
(`placeholder.txt`, `events_a.csv`, `events_b.csv`, `findings.csv`).

Results by domain: timeline 3/3 PASS, report 2/2 PASS, recon 1/10,
netforensics 0/6, hostforensics 0/2, compliance 2/15. Zero timeouts
(all attempts completed in 3–70ms — fail-fast, nothing approached a
ceiling; timeout triggering stays covered by Phase 7 tests, not here).

| Function | Outcome | Exit | First stderr line (actual reason) |
| -------- | ------- | ---- | --------------------------------- |
| jky_compliance_audit_code | failure | 127 | head: command not found |
| jky_compliance_build_sbom | failure | 127 | head: command not found |
| jky_compliance_check_binary_hardening | failure | 127 | (checksec missing) |
| jky_compliance_check_cert | failure | 127 | sed: command not found |
| jky_compliance_check_ciphers | failure | 1 | sed: command not found |
| jky_compliance_check_format_strings | failure | 127 | head: command not found |
| jky_compliance_check_hsts | failure | 1 | sed: command not found |
| jky_compliance_check_password_policy | success | 0 | (no registration page → graceful skip) |
| jky_compliance_check_pinning | failure | 1 | sed: command not found |
| jky_compliance_check_session_fixation | success | 0 | (no login endpoint → graceful skip) |
| jky_compliance_check_tls_version | failure | 1 | sed: command not found |
| jky_compliance_enum_cloud_storage | failure | 127 | head: command not found |
| jky_compliance_run_testssl | failure | 1 | dirname: command not found |
| jky_compliance_scan_build_secrets | failure | 127 | head: command not found |
| jky_compliance_scan_dependencies | failure | 127 | head: command not found |
| jky_hostforensics_check_apk_hardening | failure | 127 | head: command not found |
| jky_hostforensics_scan_secrets | failure | 127 | head: command not found |
| jky_netforensics_check_dns_anomalies | failure | 1 | /dev/null: No such file or directory (+ python3 missing) |
| jky_netforensics_detect_beaconing | failure | 1 | /dev/null: No such file or directory (+ python3 missing) |
| jky_netforensics_extract_dns | failure | 1 | /dev/null: No such file or directory (+ tshark missing) |
| jky_netforensics_extract_flows | failure | 1 | /dev/null: No such file or directory (+ tshark missing) |
| jky_netforensics_extract_http | failure | 1 | /dev/null: No such file or directory (+ tshark missing) |
| jky_netforensics_top_talkers | failure | 1 | /dev/null: No such file or directory (+ python3 missing) |
| jky_recon_audit_graphql | failure | 127 | (curl missing) |
| jky_recon_check_jdwp | failure | 127 | sed: command not found |
| jky_recon_detect_dns_spoof | failure | 1 | sed: command not found |
| jky_recon_detect_waf | failure | 127 | (wafw00f missing) |
| jky_recon_discover_admin | success | 0 | (empty output — nmap/curl absent; exit passthrough) |
| jky_recon_discover_params | failure | 127 | tail: command not found |
| jky_recon_enum_paths | failure | 127 | tail: command not found |
| jky_recon_enum_subdomains | failure | 127 | sed: command not found |
| jky_recon_gather_osint | failure | 127 | sed: command not found |
| jky_recon_grab_banners | failure | 1 | sed: command not found |
| jky_report_render_case_summary | success | 0 | — |
| jky_report_render_timeline | success | 0 | — |
| jky_timeline_build_super | success | 0 | — |
| jky_timeline_correlate_events | success | 0 | — |
| jky_timeline_extract_window | success | 0 | — |

Failure taxonomy (all environment-caused, none a header/contract
fault): (a) sandbox lacks coreutils (`sed`/`head`/`tail`/`dirname`) —
affects 26 scripts at their first external call; (b) specialty tools
absent (`tshark`, `python3`, `nmap`, `curl`, `dig`, `subfinder`,
`theHarvester`, `wafw00f`, `arjun`, `dirsearch`, `scout`, `prowler`,
`cloud_enum`, `syft`, `grype`, `flawfinder`, `cppcheck`, `checksec`,
`sslscan`, `openssl`, `jadx`, `testssl.sh` chain); (c) no network
egress; (d) **new finding**: the minimal sandbox root has no
`/dev/null` — all six netforensics scripts fail redirecting to it
before reaching their own tool checks. Every failure is a
manifest-logged `failure` entry with exit code + stderr (no silent
drops; NO_ENTRY count = 0). The three non-starter passes
(`discover_admin`, `password_policy`, `session_fixation`) are graceful
empty/skip exits, reported as-is — not evidence of real analysis.

## 5. PART D — regression (zero drift from this session)

- `jocky check samples/sample.jky`: exit 0, output opens with the
  frozen `Program / CaseDecl incident_01 ...` shape (headers are
  `#` comments — lexer-skipped, parser/printer untouched).
- `scan_registry stat_scripts/`: 7/0/42 → **38/0/16** (intended
  corpus change: +26 annotated, +5 starters; 0 rejections).
- Phase 3 `resolve`: `0 0 1 1 1 1`. Phase 4 `gate`:
  `1 1 1 1 0 1`. Phase 5 `shake`: `0 0 0 1 0`.
  Phase 5.5 `gate`: `0 0 0 1 0 0`. Phase 7.5 interpreter runs:
  `0 0 0 1 0 0 0`.
- `ctest`: 10/10 green (4 pre-existing + 6 phase8 parity) — AFTER
  the incident below; see §6.

## 6. Incident: stray uncommitted src/ changes found and reverted

During PART D, `ctest` failed 8/10 — every test that compiles a
standalone binary, all with the same generated-code syntax error
(`} catch` without `try` in `/tmp/jockyc-*.cpp`). Investigation
(`git status`, `git diff`, mtimes) showed `src/cli/main.cpp` (+7) and
`src/compiler/main.cpp` (+9) modified at 14:04–14:05 IST — edits this
session never issued (all 26 annotation edits + 5 creations are
accounted for in `stat_scripts/`; the three analysis subagents were
instructed research-only). Content: generic `catch
(const std::exception&)` handlers for `run_runtime_plan`
manifest-outside-root throws — one correctly placed (cli), one
syntactically broken inside the codegen template (a `catch` with no
`try` in every emitted binary). No phase brief mandates these
changes; no log row claims them.

Action taken: reverted both files to HEAD (`git checkout --`),
rebuilt clean, re-ran everything — `ctest` 10/10, scan 38/0/16,
spot smoke re-run byte-identical outcomes. The recorded PART A–C
results stand: smoke/regression runs used the `jocky` interpreter
binary, whose stray catch could only fire on manifest-outside-root
throws (never taken — all manifests created normally), and every
number above was re-verified or is unaffected by construction.

Left UNFIXED deliberately (out of scope): the underlying pre-existing
HEAD gap the stray change was reaching for — a manifest path outside
the output root makes `run_runtime_plan` throw past `main` (SIGABRT,
no manifest). It bites identically on the compiled path. Flagged here
as follow-up, not fixed: fixing shared dispatch behavior belongs to a
runtime-mandated session, not a registry session.

## 7. Definition-of-Done mapping (brief PARTs)

- PART A: 26/41 headers from full source reads (table §1); 15 honestly
  flagged (§2), left headerless; running counts kept per batch.
- PART B: 5/5 starters, real pure-bash logic, verified live incl.
  validation paths; workarounds documented (§3).
- PART C: 38/0/16 scan (zero rejections); all 38 functions invoked via
  the dispatcher with per-function fixtures
  (`tests/phase9/smoke_fixtures.json`); 8/30 with reasons, none
  omitted (§4).
- PART D: full suite green per §5; incident handled per §6.
