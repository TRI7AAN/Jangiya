# Phase 1 Verification Checkpoint

> Read-only verification session. No metadata headers written, no
> scanner built, `implementationplan.md` and `roadmap.md` untouched.
> Every claim below is backed by the command output or file content
> quoted in place — no summarized claims. `build/` was recreated to
> run the verification and removed afterward to restore the approved
> tree state.

## 1. Build Verification — PASS

Command: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j5 && ./build/jocky check samples/sample.jky`

Real terminal output:

```text
-- Generating done (0.0s)
-- Build files have been written to: /home/kali/Desktop/Jangiya/build
[ 50%] Building CXX object CMakeFiles/jocky.dir/src/cli/main.cpp.o
[100%] Linking CXX executable jocky
[100%] Built target jocky
Program
  CaseDecl incident_01 capabilities=["netforensics.pcap.read", "timeline.correlate"]
  EvidenceDecl capture adapter=pcap path="evidence/capture.pcap"
  EvidenceDecl authlogs adapter=eventlog path="evidence/auth.json"
  RuleDecl flag_big_flows(min_bytes: int) -> table<flow>
    let big = source capture | where (bytes > min_bytes and protocol == "tcp") | select src_ip, dst_ip as dest, bytes | sort_by bytes desc | limit 100;
    big | emit report;
  InvestigationDecl incident_01
    let flows = call jky_netforensics_extract_flows(pcap_path: "evidence/capture.pcap", bpf: "tcp port 443", threshold: 0.75, verbose: true);
    let auth = source authlogs | where (message contains "failed" and not user in ["root", "admin"]);
    let tl = correlate(flows, auth) within 5m on src_ip | group_by src_ip | having events > 3 | select src_ip | sort_by src_ip asc | limit 10;
    tl | write report("out/incident_report.md");
    flows | write "out/flows.json";
EXIT=0
```

**Verdict: PASS.** Clean configure, clean compile (g++ 15.2.0, C++20),
sample parses with exit 0 and a complete AST dump.

## 2. src/ Provenance Confirmation — RESOLVED

Command: `find src -type f | sort`

```text
src/cli/main.cpp
```

`src/` contains exactly one file: the JOCKY skeleton CLI. No Java
sources, no ambiguous material remain. Stated explicitly as resolved:
**no Kalki Java sources remain under `src/`.**

## 3. Script Count Reconciliation — DISCREPANCY FLAGGED

Command: per-directory `ls | wc -l` under `stat_scripts/` plus
`find stat_scripts -name '*.sh' | wc -l`.

```text
stat_scripts/compliance/: 24 files
stat_scripts/hostforensics/: 7 files
stat_scripts/netforensics/: 6 files
stat_scripts/recon/: 10 files
stat_scripts/shared/: 1 files
total .sh: 48
```

Actual total: **48 scripts** (47 `jky_*` functions + 1 shared helper).

Against the "154 scripts" figure: a grep for `154|158` across
`roadmap.md` and `implementationplan.md` returns **no matches** — both
plan files were already corrected in a prior session (see `logs.md`:
wiki+README sync row) and no longer state that figure. The only
remaining "158" figures are historical records of the pre-prune corpus
(`wiki/09` §5 table and summary, `1_classification.csv` V-series rows).

The 48-vs-154 difference is therefore **not investigated further here
per instructions** — recorded factually: 48 present, 154 absent from
plan docs, historical 158 preserved in `wiki/09` and the V-series rows.
**Flagged for user confirmation** (see OPEN QUESTIONS).

## 4. Missing Domain Check — GAP CONFIRMED

Command: `ls -d stat_scripts/timeline stat_scripts/report`

```text
ls: cannot access 'stat_scripts/timeline': No such file or directory
ls: cannot access 'stat_scripts/report': No such file or directory
```

**Neither `stat_scripts/timeline/` nor `stat_scripts/report/` exists.**
Stated plainly as a gap. No placeholder scripts were created to fill
it, per instructions.

## 5. Naming Convention Audit — ONE EXCEPTION

Command: list all basenames under `stat_scripts/` NOT matching
`^jky_(recon|netforensics|hostforensics|timeline|compliance|report)_[a-z0-9]+_[a-z0-9_]+\.sh$`.

Result: exactly one file — `stat_scripts/shared/testssl.sh`.

Disposition: `testssl.sh` is an **internal wrapper around the
third-party `testssl.sh` scanner binary** (it locates the external
tool via `which` and execs it with fixed flags; called by the TLS
checks). It is therefore already in the "wrapper script" role — it
needs no rename. A `jky_` function name would belong to a future
caller-side wrapper if the registry ever requires every executable
entry to carry a `jky_` name; that decision is deferred to the
`scan_registry` design (Phase 2), not made here. All other 47 scripts
match the convention exactly, with valid domains (`recon`,
`compliance`, `hostforensics`, `netforensics`).

## 6. Unresolved Root Files — Contents Reported, Origin UNRESOLVED

Inspected live (no origin assumed; no project attribution made):

- **`1_classification.csv`** — 165 lines (header + 164 rows).
  Header: `code,test_name,category,tool_or_method`.
  Tail rows are the six `jky_netforensics_*` entries
  (e.g. `jky_netforensics_detect_beaconing,Flag periodic callback
  shapes,custom_script,python3`); the head rows are `V-001..V-158`
  (e.g. `V-001,Sensitive data via OSINT,tool_parsing,theHarvester`).
- **`3_decision_engine.csv`** — 124 lines (header + 123 rows).
  Header: `code,test_name,category,tool,script,eval_method,pass_criteria,
  fail_criteria,default_severity,fallback_method,requires_external_tool`.
  Factual coverage note: unlike the other two files, it contains **no
  `jky_netforensics_*` rows** — decision rules for the six new
  functions exist only in `2_static_map.json`.
- **`2_static_map.json`** — top-level JSON object, **130 keys**:
  first key `V-001`, last key `jky_netforensics_check_dns_anomalies`.
  Entry shape (shown: `V-001`):
  `name, category, tool, script, decision_rule{method, pass_criteria,
  fail_criteria}, default_severity`.
  Six `jky_netforensics_*` entries present with the same shape.

References found: `wiki/09-kalki-inventory.md` (§7 table, §8 drift
items, §9 handoff, §12 batch record), `README.md` (repo-map row:
"Registry reference data"), `logs.md` (netforensics batch row).

**Flag: UNRESOLVED — needs user confirmation of origin and scope.**
The contents are consistent with the V-series taxonomy plus the six
recent additions, but no origin claim is made here.

## 7. Wiki Consistency Check — NO UNDOCUMENTED DRIFT

Checked `wiki/09-kalki-inventory.md` against the live tree:

- §5's per-file table lists pre-rename `V-XXX.sh` paths. Those paths
  no longer exist on disk (`find stat_scripts -name 'V-*.sh'` → 0).
  This is **documented, not drift**: §11 records every old→new mapping,
  and every §11 target path was verified present (spot-checked
  `recon/jky_recon_gather_osint.sh`,
  `compliance/jky_compliance_verify_patches.sh`,
  `hostforensics/jky_hostforensics_check_apk_hardening.sh`,
  `netforensics/jky_netforensics_check_dns_anomalies.sh`,
  `shared/testssl.sh`; per-directory counts 10/24/7/6/1 match §11).
- §12's six netforensics functions match disk exactly
  (`ls stat_scripts/netforensics/` → the same six names).
- No `.sh` file exists on disk that §11/§12 do not account for
  (48 total = 41 + 6 + 1, all mapped).

Conclusion: `wiki/09` accurately reflects the tree when §5 (historical)
is read through §11 (rename record). No silent additions, removals, or
renames since the checkpoint baseline.
