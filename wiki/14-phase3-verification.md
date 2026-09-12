# Phase 3 Verification (Retroactive Persistence + Fresh Re-Run)

> This file persists the read-only verification session that was reported
> in chat but never written down: clean build, `scan_registry` 7/0/42,
> all 6 `tests/phase3/` fixtures behaving as specified, and the
> `$?`-after-substitution self-correction. Per `AGENTS.md` §6 (a claim
> without a persisted, checkable artifact is not valid evidence), that
> session's findings are re-verified live here — fresh command output,
> not a copy of the prior summary — and recorded as checkable artifact.
> No source file was touched; the build ran in `/tmp/jocky-chkpt`
> (removed afterward), never `./build`, so the tree is untouched.
> Toolchain this session: `g++ (Debian 15.2.0-17) 15.2.0`,
> `cmake version 4.3.4`. Current phase (stated, not acted on): **Phase 4**,
> per `implementationplan.md`; this file changes nothing about that.

## 1. Build — PASS

Command: `cmake -S . -B /tmp/jocky-chkpt -DCMAKE_BUILD_TYPE=Release && cmake --build /tmp/jocky-chkpt -j5` (run from repo root)

Real terminal output (tail):

```text
[ 83%] Built target scan_registry
[100%] Linking CXX executable jocky
[100%] Built target jocky
```

`CONFIG_EXIT=0`, both targets link. Same two targets as every prior
checkpoint (`jocky`, `scan_registry`); no new warnings surfaced above
the tail cut.

## 2. `jocky check samples/sample.jky` — PASS (AST identical to wiki/10 §1 and wiki/11 §1)

Command: `/tmp/jocky-chkpt/jocky check samples/sample.jky; echo "EXIT=$?"` (repo root as CWD)

Real terminal output, verbatim:

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

Line-for-line identical to the `wiki/11` §1 block (which itself matched
`wiki/10` §1): same case capabilities, same evidence, same rule, same
four investigation statements, `EXIT=0`. The frozen `check` printer is
unchanged — **no drift**. (Note: `sample.jky` stays intentionally
unresolvable — `threshold`/`verbose` are not in the real schema — so it
is a parse-level fixture only; see `wiki/12`. It is not resolved here.)

## 3. `scan_registry stat_scripts/` — PASS (7/0/42, SKIP set identical)

Command: `/tmp/jocky-chkpt/scan_registry stat_scripts/; echo "EXIT=$?"` (repo root as CWD; JSON index on stdout, elided here — schema unchanged since Phase 3)

Stderr SUMMARY, verbatim:

```text
SUMMARY 7 registered, 0 rejected, 42 skipped
SUMMARY recon: 0 registered (Phase 9 pending)
SUMMARY netforensics: 6 registered
SUMMARY hostforensics: 0 registered (Phase 9 pending)
SUMMARY timeline: 0 registered (Phase 9 pending)
SUMMARY compliance: 1 registered
SUMMARY report: 0 registered (Phase 9 pending)
EXIT=0
```

SKIP-line verification (stronger than eyeballing): the 42 live `SKIP`
lines were extracted, sorted, and compared programmatically against the
42 `SKIP` lines pasted in `wiki/11` §2 — **42/42 identical, same paths,
same reason strings** (`no @jocky: header (not a registry function)`).
`7 registered + 42 skipped = 49 = find stat_scripts -name '*.sh' | wc -l`.
No script added, removed, or annotated outside a logged session.

## 4. `jocky resolve` on all six `tests/phase3/` fixtures — PASS (6/6 as specified in wiki/12)

Commands (repo root as CWD, default registry `stat_scripts/`, exits
captured into a variable immediately — see §5 for why that matters):

`call_ok.jky` (`extract_flows` with all three args explicit) → `EXIT=0`:

```text
RESOLVED call jky_netforensics_extract_flows -> table<flow> [capability netforensics.pcap.read]
  arg pcap_path: path
  arg bpf: string
  arg out_csv: path
BOUND flows: table<flow>
```

`call_default_ok.jky` (omits defaulted `bpf`/`out_csv`) → `EXIT=0`:

```text
RESOLVED call jky_netforensics_extract_flows -> table<flow> [capability netforensics.pcap.read]
  arg pcap_path: path
  arg bpf: string = <default "">
  arg out_csv: path = <default "">
BOUND flows: table<flow>
```

`call_unknown.jky` → `EXIT=1`:

```text
error: tests/phase3/call_unknown.jky:2:11: unknown function 'jky_netforensics_frob_widgets' (no registry entry; run scan_registry to rebuild the index)
```

`call_missing_required.jky` → `EXIT=1`:

```text
error: tests/phase3/call_missing_required.jky:2:15: missing required argument 'pcap_path' for function 'jky_netforensics_extract_flows' (no default declared)
```

`call_type_mismatch.jky` → `EXIT=1`:

```text
error: tests/phase3/call_type_mismatch.jky:2:65: type mismatch for argument 'top_n' of function 'jky_netforensics_top_talkers': declared 'int' but got string
```

`call_extra_arg.jky` → `EXIT=1`:

```text
error: tests/phase3/call_extra_arg.jky:2:72: unknown argument 'threshold' for function 'jky_netforensics_extract_flows' (declared inputs: pcap_path, bpf, out_csv)
```

Two pass with `RESOLVED` + `BOUND table<flow>`; four refuse with
`file:line:col` diagnostics — exactly the `wiki/12` §§3–5
specification. Fixture contents (each 3 lines, `investigate t { ... }`)
re-read live this session and unchanged; resolved capabilities
(`netforensics.pcap.read`) are recorded on `ResolvedCall` and still
unenforced — that is Phase 4's job, not drift.

## 5. Shell-artifact self-correction (persisted so it is not re-learned)

During the original chat-only session, a one-liner of the form
`echo "$(basename $f) exit=$? msg=$(cat err)"` printed `exit=0` for all
six fixtures. That was a shell expansion-order artifact, not a code
bug: `bash` expands `$(basename $f)` before `$?`, so `$?` reported
basename's status (0), not `jocky`'s. Direct re-runs (`cmd; echo
"EXIT=$?"` and `cmd; ec=$?; ...`) show the true codes above (0,0,1,1,1,1).
Rule for future sessions: **capture `$?` into a named variable on the
very next statement after the command under test, before any
`$(...)` expansion.**

## 6. Quarantine cross-reference — NO DRIFT (see Part C of this checkpoint)

Recomputed live this session (`sha256sum _quarantine/...`):

```text
a5c042715c68128f57314c867f2b993d826d353caa6ccb6f3b83f3d61882dd91  _quarantine/1_classification.csv
5d237007d83398830f6fa326f4f685872aa841a33744ff1326c36865ed8388cc  _quarantine/2_static_map.json
16ae24689169b29f486d6566054f9589c71712e478b0db85262ce554db0683e5  _quarantine/3_decision_engine.csv
```

All three match the `wiki/11` §3 baseline byte-for-byte, and the
structural shape still matches `wiki/10` §6 (165-line classification
CSV, 124-line decision CSV, 130-key static map). Standing
per-phase-boundary check: still clean.

**Verdict: CLEAN** — build, frozen `check` AST, registry counts plus
SKIP-set identity, all six resolve fixtures, and quarantine digests all
match the logged record with zero drift. The prior session's only real
defect was procedural (unpersisted, unlogged verification); this file
plus the retroactive `logs.md` row close exactly that gap. Phase 4
capability-gate work is untouched and still current.
