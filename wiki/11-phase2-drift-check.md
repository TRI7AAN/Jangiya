# Phase 2 Drift Check (Verification Only)

> Read-only checkpoint. No changes to `implementationplan.md`,
> `roadmap.md`, or any source file. `build/` was recreated to run this
> verification and removed afterward to restore the approved tree state.
> Current phase as recorded (stated, not acted on): **Phase 3 — `call`
> Expression Grammar Extension (Resolution & Checking)**: call resolver
> against `scan_registry` output, arity checking with explicit `= default`
> decision, parse-level input-type validation, return-type plumbing, tests
> on the real 7-function registry. The renumbering fix has not run; it is
> Prompt 3's job and is not touched here.

## 1. Build/Run Reverification — PASS (no drift)

Command: `rm -rf build && cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j5`

Real terminal output:

```text
-- The CXX compiler identification is GNU 15.2.0
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Check for working CXX compiler: /usr/bin/c++ - skipped
-- Detecting CXX compile features
-- Detecting CXX compile features - done
-- Configuring done (0.3s)
-- Generating done (0.0s)
-- Build files have been written to: /home/kali/Desktop/Jangiya/build
[ 20%] Building CXX object CMakeFiles/jocky.dir/src/cli/main.cpp.o
[ 40%] Building CXX object CMakeFiles/scan_registry.dir/tools/scan_registry.cpp.o
[ 60%] Building CXX object CMakeFiles/scan_registry.dir/src/stdlib/metadata_scanner.cpp.o
[ 80%] Linking CXX executable jocky
[ 80%] Built target jocky
[100%] Linking CXX executable scan_registry
[100%] Built target scan_registry
```

Command: `./build/jocky check samples/sample.jky; echo "EXIT=$?"`

Real terminal output:

```text
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

Comparison against the AST shape in `wiki/10-phase1-verification.md` §1:
identical — same case block (`incident_01`, same two capabilities), same
two evidence declarations (`capture`/`pcap`, `authlogs`/`eventlog`, same
paths), same rule (`flag_big_flows(min_bytes: int) -> table<flow>`, same
two statements), same investigation (`incident_01`, same four
statements), EXIT=0. **No difference, however small.** (The build log
additionally shows the `scan_registry` target compiling — expected from
the Phase 2 `CMakeLists.txt`, not new.)

## 2. Registry Reverification — PASS (counts match exactly)

Command: `./build/scan_registry stat_scripts/; echo "EXIT=$?"` (JSON index
elided; per-file SKIP lines and SUMMARY pasted verbatim)

```text
SKIP stat_scripts/compliance/jky_compliance_audit_cloud.sh: no @jocky: header (not a registry function)
SKIP stat_scripts/compliance/jky_compliance_audit_code.sh: no @jocky: header (not a registry function)
SKIP stat_scripts/compliance/jky_compliance_audit_iam.sh: no @jocky: header (not a registry function)
SKIP stat_scripts/compliance/jky_compliance_audit_kernel_config.sh: no @jocky: header (not a registry function)
SKIP stat_scripts/compliance/jky_compliance_audit_secgroups.sh: no @jocky: header (not a registry function)
SKIP stat_scripts/compliance/jky_compliance_build_sbom.sh: no @jocky: header (not a registry function)
SKIP stat_scripts/compliance/jky_compliance_check_binary_hardening.sh: no @jocky: header (not a registry function)
SKIP stat_scripts/compliance/jky_compliance_check_cert.sh: no @jocky: header (not a registry function)
SKIP stat_scripts/compliance/jky_compliance_check_ciphers.sh: no @jocky: header (not a registry function)
SKIP stat_scripts/compliance/jky_compliance_check_cors.sh: no @jocky: header (not a registry function)
SKIP stat_scripts/compliance/jky_compliance_check_format_strings.sh: no @jocky: header (not a registry function)
SKIP stat_scripts/compliance/jky_compliance_check_hsts.sh: no @jocky: header (not a registry function)
SKIP stat_scripts/compliance/jky_compliance_check_imds.sh: no @jocky: header (not a registry function)
SKIP stat_scripts/compliance/jky_compliance_check_nx.sh: no @jocky: header (not a registry function)
SKIP stat_scripts/compliance/jky_compliance_check_password_policy.sh: no @jocky: header (not a registry function)
SKIP stat_scripts/compliance/jky_compliance_check_pinning.sh: no @jocky: header (not a registry function)
SKIP stat_scripts/compliance/jky_compliance_check_session_fixation.sh: no @jocky: header (not a registry function)
SKIP stat_scripts/compliance/jky_compliance_check_tls_version.sh: no @jocky: header (not a registry function)
SKIP stat_scripts/compliance/jky_compliance_enum_cloud_storage.sh: no @jocky: header (not a registry function)
SKIP stat_scripts/compliance/jky_compliance_scan_build_secrets.sh: no @jocky: header (not a registry function)
SKIP stat_scripts/compliance/jky_compliance_scan_dependencies.sh: no @jocky: header (not a registry function)
SKIP stat_scripts/compliance/jky_compliance_verify_patches.sh: no @jocky: header (not a registry function)
SKIP stat_scripts/compliance/jky_compliance_verify_segmentation.sh: no @jocky: header (not a registry function)
SKIP stat_scripts/compliance/jky_compliance_verify_supply_chain.sh: no @jocky: header (not a registry function)
SKIP stat_scripts/hostforensics/jky_hostforensics_audit_cron.sh: no @jocky: header (not a registry function)
SKIP stat_scripts/hostforensics/jky_hostforensics_audit_permissions.sh: no @jocky: header (not a registry function)
SKIP stat_scripts/hostforensics/jky_hostforensics_check_apk_hardening.sh: no @jocky: header (not a registry function)
SKIP stat_scripts/hostforensics/jky_hostforensics_check_app_storage.sh: no @jocky: header (not a registry function)
SKIP stat_scripts/hostforensics/jky_hostforensics_enum_privesc.sh: no @jocky: header (not a registry function)
SKIP stat_scripts/hostforensics/jky_hostforensics_scan_logs.sh: no @jocky: header (not a registry function)
SKIP stat_scripts/hostforensics/jky_hostforensics_scan_secrets.sh: no @jocky: header (not a registry function)
SKIP stat_scripts/recon/jky_recon_audit_graphql.sh: no @jocky: header (not a registry function)
SKIP stat_scripts/recon/jky_recon_check_jdwp.sh: no @jocky: header (not a registry function)
SKIP stat_scripts/recon/jky_recon_detect_dns_spoof.sh: no @jocky: header (not a registry function)
SKIP stat_scripts/recon/jky_recon_detect_waf.sh: no @jocky: header (not a registry function)
SKIP stat_scripts/recon/jky_recon_discover_admin.sh: no @jocky: header (not a registry function)
SKIP stat_scripts/recon/jky_recon_discover_params.sh: no @jocky: header (not a registry function)
SKIP stat_scripts/recon/jky_recon_enum_paths.sh: no @jocky: header (not a registry function)
SKIP stat_scripts/recon/jky_recon_enum_subdomains.sh: no @jocky: header (not a registry function)
SKIP stat_scripts/recon/jky_recon_gather_osint.sh: no @jocky: header (not a registry function)
SKIP stat_scripts/recon/jky_recon_grab_banners.sh: no @jocky: header (not a registry function)
SKIP stat_scripts/shared/testssl.sh: no @jocky: header (not a registry function)
SUMMARY 7 registered, 0 rejected, 42 skipped
SUMMARY recon: 0 registered (Phase 9 pending)
SUMMARY netforensics: 6 registered
SUMMARY hostforensics: 0 registered (Phase 9 pending)
SUMMARY timeline: 0 registered (Phase 9 pending)
SUMMARY compliance: 1 registered
SUMMARY report: 0 registered (Phase 9 pending)
EXIT=0
```

**7 registered, 0 rejected, 42 skipped, EXIT=0 — matches the
end-of-Phase-2 report exactly.** Supporting count:
`find stat_scripts -name '*.sh' | wc -l` → 49 total `.sh`
(24 compliance + 7 hostforensics + 6 netforensics + 10 recon + 2 shared);
7 registered + 42 skipped = 49. No script added, removed, or annotated
outside a logged session.

## 3. Quarantine Integrity Recheck — BASELINE GAP, NOT DRIFT

Recomputed SHA-256 (no recorded per-file digests exist to compare
against — see note):

```text
a5c042715c68128f57314c867f2b993d826d353caa6ccb6f3b83f3d61882dd91  _quarantine/1_classification.csv
5d237007d83398830f6fa326f4f685872aa841a33744ff1326c36865ed8388cc  _quarantine/2_static_map.json
16ae24689169b29f486d6566054f9589c71712e478b0db85262ce554db0683e5  _quarantine/3_decision_engine.csv
```

Structural re-verification passes: `_quarantine/` holds exactly the 3
files plus its README; `1_classification.csv` is 165 lines (header + 164
rows), `3_decision_engine.csv` is 124 lines (header + 123 rows),
`2_static_map.json` parses with 130 top-level keys — all matching
`wiki/10` §6.

Honest limitation, stated plainly: the end-of-Phase-2 record
(`wiki/09` §13, `logs.md` scanner row) claims "digests identical
before/after" **without recording any digest values**, so an exact
hash-to-record comparison is impossible. The three digests above are
therefore published here as the first recorded baseline for all future
checkpoints. No evidence of tampering was found (contents still match
the §6 shape), but provable untouched-ness starts from this baseline.

## 4. Git State Check — COMMITTED, TREE CLEAN (earlier flag resolved)

```text
On branch main
Your branch is up to date with 'origin/main'.

Untracked files:
	build/
nothing added to commit but untracked files present (use "git add" to track)
===LOG===
5c7372b Log git push of Phases 0-2
186abd3 JOCKY Phases 0-2: docs, skeleton, registry, scanner
626392d Update README.md
f20807c Update README.md
88b5bd5 Initial commit
```

`git diff --stat` and `git diff --cached --stat` are both empty: no
tracked working-tree changes outside the log. The earlier "zero commits
since the 3 pre-JOCKY commits" flag is **resolved, not recurring**:
`186abd3` (JOCKY Phases 0–2, 78 files) and `5c7372b` (log row) sit on top
of the 3 pre-JOCKY commits and are pushed (`main` up to date with
`origin/main`). The only uncommitted item is `build/`, untracked
regenerable output recreated by this session and removed afterward —
not a logs.md gap. The next unlogged item after this session will be
this checkpoint's own `logs.md` row, appended below.

## 5. Wiki-to-Code Consistency Spot Check — MATCH (prompt premise stale, not drift)

`wiki/06-api-contracts.md` §5 documents `call_expr`:

```ebnf
call_expr      = "call" qualified_name "(" [ arg_list ] ")" ;
```

and `include/jocky/parser/parser.hpp` implements it (`parse_call`,
called from both `parse_expr_value` for head position and
`parse_operand` for predicate position; `main.cpp` pretty-prints `call
f(...)`). Grammar and implementation **match**; neither file shows edits
outside the logged Phase 1/2 sessions (`git log` for both paths ends at
`186abd3`; `git diff` empty).

Flag on the session premise, not on the code: the brief expected
`call_expr` to be **absent** pending Prompt 3. It is present — but that
is the Phase 1 baseline, not ahead-of-plan editing. `logs.md` (Phase 1
row) and `implementationplan.md` ("the Phase 1 parser already produces
`call` AST nodes") both record that call *parsing* landed in Phase 1;
Phase 3 is call *resolution + arity/type checking + return-type
plumbing*. If Prompt 3 purports to add `call_expr` syntax, its premise
is stale; the resolution/checking scope in `implementationplan.md`
stands.

## 6. Open Items Carried Forward (re-listed, not resolved)

1. The three quarantined files' origin is still unconfirmed
   (`_quarantine/README.md` unchanged; do-not-integrate still in force).
2. `stat_scripts/timeline/` and `stat_scripts/report/` still do not
   exist (confirmed this session: `ls -d` → "No such file or
   directory" for both); greenfield stdlib pending Phase 9.
3. The two staleness notes — `README.md` repo map and `wiki/09`
   (§7/§8/§12/§13) passages that still treat the quarantined CSV/JSON
   as registry reference data — remain deferred to Phase 9, untouched
   by this session.
4. New observation from this checkpoint (recorded, not fixed —
   verification only): `README.md`'s header ("Phase 1 Complete, Phase 2
   Current"), its `implementationplan.md` map row ("Phase 2: metadata
   convention + registry scanner"), and its status checklist (Phase 2
   listed as current) are stale against the actual current phase
   (Phase 3, per `implementationplan.md`). Likewise, §3 above records
   the missing quarantine digest baseline. Both are carried forward,
   not resolved here.

**Verdict: CLEAN** — build output, AST shape, registry counts, git
state, and grammar/implementation consistency all match the end-of-Phase-2
record with zero drift. Two non-drift notes carried forward: (a) no
prior quarantine digests on record, so §3 publishes the first baseline;
(b) `README.md` phase labels still say Phase 2.
