# Phase 6–7 Implementation Audit (read-only; nothing fixed)

> Date: 2026-09-13. Scope: verify Phase 6 (`jockyc`/embedding) and Phase 7
> (runtime dispatcher + manifest) against the design commitments in `wiki/13`
> (Phase 4 gate), `wiki/16` (Phase 5 tree-shaking), and `wiki/17`
> (Phase 5.5 control flow). These commits predate this conversation; nothing
> about them was assumed — every claim below was re-tested live. No code was
> changed. All probe artifacts lived in `/tmp/audit67` (build, binaries,
> manifests, harnesses); verbatim outputs and digests are persisted HERE, so
> later sessions need nothing from `/tmp`.
>
> Build: `cmake -S . -B /tmp/audit67/build -DCMAKE_BUILD_TYPE=Release`,
> `cmake --build` — CONFIGURE_EXIT=0, BUILD_EXIT=0, zero warnings/errors
> (g++ 15.2.0, cmake 4.3.4). Project suite: `ctest` 4/4 passed
> (`phase6_embedder_stale_hash`, `phase6_jockyc_integration`,
> `phase7_runtime_safety`, `phase7_embedded_runtime`).

## 1. Phase 6 — embedding / jockyc

### 1.1 jockyc exists and produces a standalone binary — CONFIRMED

`samples/sample.jky` calls only ONE registered function
(`jky_netforensics_extract_flows`), so the audit used the documented
equivalent `/tmp/audit67/two.jky` (case + two `call` sites:
`extract_flows` and `jky_netforensics_top_talkers`):

```text
$ jockyc /tmp/audit67/two.jky --registry stat_scripts -o /tmp/audit67/two_bin
JOCKYC_EXIT=0
BUILT /tmp/audit67/two_bin with 2 embedded script(s)
  jky_netforensics_extract_flows be7d17fd98292bad0f900d677d29442113471261338c92984c5496107b4f78ea
  jky_netforensics_top_talkers ed0383d364a71ecc16151346679fe97e1ce2c1060d9c0836db436bb02063d269
```

Both digests match the `wiki/19` persisted table exactly. Binary runs
without the registry (`run` after registry deletion is exercised by the
project's own `phase7_embedded_runtime` test, passed).

### 1.2 Tree-shaking proven via `strings` — CONFIRMED (exact closure, nothing more)

```text
$ strings two_bin | grep -c "tshark-backed flow extractor"   # CALLED extract_flows body
1
$ strings two_bin | grep -c "top-N endpoints by bytes"        # CALLED top_talkers body
1
$ strings two_bin | grep -c "jky_netforensics_extract_dns"    # UNCALLED
0  (grep exit 1)
$ strings two_bin | grep -c "jky_netforensics_detect_beaconing"  # UNCALLED
0  (grep exit 1)
$ strings two_bin | grep -c "jky_compliance_run_testssl"      # UNCALLED
0  (grep exit 1)
```

Called scripts' content IS present (count 1 each); three uncalled
scripts' content is ABSENT (count 0). `--list-embedded` agrees:

```text
JOCKY standalone case audit_two scripts=2
EMBEDDED jky_netforensics_extract_flows sha256=be7d17fd... bytes=5741
EMBEDDED jky_netforensics_top_talkers sha256=ed0383d3... bytes=2195
```

### 1.3 `registry_version_hash` in binary / query flag — GAP (missing)

Searched all of `include/`, `src/`, `tools/` for
`registry_version_hash|version_hash|registry_hash` (plus `--version` on
the artifact): zero toolchain matches (only hit in the repo is
`nmap --version` inside an unannotated recon script). The artifact's
flags are exactly `--list-embedded`, `--extract <function>`, and `run`
(verified: any other flag prints `usage: standalone ...`, exit 2).
Per-script SHA-256 values are embedded and listed, but there is NO
aggregate registry digest and NO flag to query one. Stated as a gap per
the brief; nothing was assumed to exist.

### 1.4 Linux/WSL-only scope documented — CONFIRMED

- `src/compiler/main.cpp:1-4` header comment: `Linux/WSL scope`.
- `wiki/19`: `Platform scope: Linux/WSL`; `wiki/20`: `Linux/WSL, x86-64`.
- Construction is Linux-only regardless of docs: hardcoded
  `/usr/bin/bash`, `/lib/x86_64-linux-gnu/*`, `/proc/self/exe`
  (`dispatcher.hpp:397-405`), `unshare --user --map-root-user --net --pid
  --fork`, and `chroot` (`dispatcher.hpp:413-439, 623-631`).

### 1.5 Missing/unreadable script at embed time — FAILS CLEAN, no partial binary

Two layers, both tested directly:

(a) Scan-stage break (unresolvable `depends_on`, registry with only
`pair_top.sh` whose leaf is absent):

```text
$ jockyc tests/phase5/fixture1_direct.jky --registry /tmp/audit67/brokenreg -o broken_bin
JOCKYC_EXIT=1
REJECT /tmp/audit67/brokenreg/jky_recon_pair_top.sh: unresolvable depends_on 'jky_recon_pair_leaf'
error: registry scan rejected entries
NO-BINARY-CONFIRMED (test ! -e broken_bin)
```

(b) Embed-stage break (post-scan unreadable path), via a minimal C++
harness calling `embed_scripts` with `script_path` pointing at a
nonexistent file:

```text
EMBED-ERROR: cannot embed 'jky_recon_pair_leaf': cannot open file '/tmp/audit67/does-not-exist.sh'
RUN_EXIT=1
```

`embed_scripts` (`compiler/embedder.hpp:31-58`) throws `EmbedError`
before pushing anything for the failed entry, and `jockyc` catches it
(`src/compiler/main.cpp:411-412`) → exit 1; the generated source is
removed by `TempGuard` and the output binary is only written on host-
compiler success. No partial binary is produced at either stage. The
"no partial embedding" design holds.

## 2. Phase 7 — runtime dispatcher and manifest

### 2.1 Argument delivery: positional argv, NOT shell interpolation, NOT JSON-stdin

Quoted from the actual invocation code. Parent builds an argv vector —
never a command string (`dispatcher.hpp:623-631`):

```cpp
std::vector<std::string> owned = {
    "unshare", "--user", "--map-root-user", "--net", "--pid",
    "--fork", "--kill-child=KILL", options.executable_path,
    "--jocky-sandbox-child", sandbox_root.string()};
owned.insert(owned.end(), values.begin(), values.end());
std::vector<char*> args;
for (std::string& item : owned) args.push_back(item.data());
args.push_back(nullptr);
execvp(args[0], args.data());
```

The sandbox child re-execs bash with the script plus those argv entries
(`dispatcher.hpp:432-437`):

```cpp
child_args.push_back(const_cast<char*>("/bin/bash"));
child_args.push_back(const_cast<char*>("/script.sh"));
for (int i = 3; i < argc; ++i) child_args.push_back(argv[i]);
child_args.push_back(nullptr);
execv(child_args[0], child_args.data());
```

Static sweep: zero matches for `system(|popen|sh -c|bash -c` in all of
`include/` and `src/`. Live probe (script `echo "ARGS:$1|$2"`, timeout
20s) with hostile values produced, verbatim from the manifest:

```json
"args":{"a":"$(touch /tmp/audit67/PWNED)","b":"x y; touch /tmp/audit67/PWNED2"},
"outcome":"success","stdout":"ARGS:$(touch /tmp/audit67/PWNED)|x y; touch /tmp/audit67/PWNED2\n"
```

Both metacharacter payloads survived literally into `$1`/`$2`; neither
marker file was created (`ls /tmp/audit67/PWNED*`: no matches). The
project's own `phase7_runtime_safety` test asserts the same shape
(`literal = "$(touch /tmp/jocky-argv-injection); spaced"`, marker must
not exist — and `/tmp/jocky-argv-injection` is absent). **No injection
risk: nothing to flag under AGENTS.md §2.5.**

### 2.2 Per-script timeout from `@jocky:timeout_seconds` — CONFIRMED, process killed

The call's `timeout_seconds` (carried from registry metadata into
`RuntimeCall`) becomes a wall deadline; expiry kills the whole process
group (`dispatcher.hpp:647-667`: `kill(-child, SIGTERM)`, 100ms grace,
`kill(-child, SIGKILL)`, exit code forced to 124). Live probe (script
`sleep 30`, `timeout_seconds = 2`):

```text
PROBE2 outcome=timeout timed_out=1 exit=124 dur=2113 wall=2116
```

Wall time ≈ deadline (2.1s, not 30s); no lingering `sleep 30` afterward.
The project's suite covers the same path (`sleep 5` / timeout 1 →
`"timed_out":true`, `"outcome":"timeout"`). Timeout is enforced from the
declared per-script value.

### 2.3 CRITICAL — WhileStmt hard iteration cap — NOT IMPLEMENTED (gap)

`grep` for `WhileStmt|requires_runtime_ceiling|iteration_cap|max_iter`
across `include/jocky/runtime/` AND `src/compiler/`: **zero matches**.
The parser sets `requires_runtime_ceiling = true` on every `WhileStmt`
and `jocky check` renders it, but nothing in Phase 6 or 7 reads the
flag. The only ceiling in the dispatcher is `max_executions` (default
1000, `dispatcher.hpp:785-791` → `ceiling_denied`): a bound on total
dispatched *call attempts*, not on loop iterations.

Direct test — `while (r == r)` (unconditionally true; would loop forever
if evaluated) over a trivial `exit 0` fixture script:

```text
JOCKYC_EXIT=0   (BUILT with 2 embedded scripts)
RUN_EXIT=0 in 0.009s, status=success, executions: 1
jky_recon_pair_top success 0
```

The loop body ran EXACTLY ONCE and the run finished in 9ms: the runtime
does not evaluate `while` (or `if`/`for`) at all — it dispatches the
static authorized-call list once each. So an infinite loop cannot hang
the system, but for the wrong reason: there is no iteration semantics
and no cap mechanism. `wiki/17` §3's MUST ("Phase 7's runtime dispatcher
MUST enforce a hard iteration cap on any WhileStmt") is unmet, and
`wiki/20` admits the underlying cause ("does not yet evaluate table
pipelines or choose runtime if/loop paths. Phase 8 owns full interpreted
execution"). Corollary nobody stated: until Phase 8, an
*untaken-branch* call still EXECUTES (verified: `if/else` fixture over
the fixture registry ran both authorized calls to `success`). Phase 8
must implement branch selection AND the iteration cap together — the cap
has to exist before loop evaluation lands, not after.

### 2.4 CRITICAL — runtime capability re-check — CONFIRMED (defense in depth is real)

`run_runtime_plan` re-checks every call against the plan's
`allowed_capabilities` after logging the `started` entry and before any
spawn (`dispatcher.hpp:793-800`):

```cpp
if (!capability_allowed(plan.allowed_capabilities, call.capability)) {
    entry.outcome = "capability_denied";
    entry.stderr_text = "runtime capability re-check denied '" +
                        call.capability + "'";
    all_ok = false;
    persist_manifest(manifest_path, manifest);
    continue;
}
```

Live probe (plan allows `allowed.cap`, call requires `denied.cap`,
script would `touch` an output marker if spawned):

```text
PROBE3 outcome=capability_denied stderr=runtime capability re-check denied 'denied.cap'
(stdout empty; marker never created)
```

The runtime does NOT trust the compile-time gate: denial happens
without spawn and is manifest-logged. `wiki/13` §5's "necessary but not
sufficient" commitment is implemented. (The project's suite asserts the
same: `denied/manifest.json` contains `"outcome":"capability_denied"`
and the marker file must not exist.)

### 2.5 Manifest schema — actual JSON quoted; DIFFERS from the brief's field list

Real generated manifest (success case, `/tmp/audit67/probe1out`,
sha256 `76757b7f...`; failure-case manifest `71817221...` has identical
shape with `validation_denied` entries):

```json
{"manifest_version":"0.1.0","case_id":"probe1","script_sha256":"probe1","runtime_version":"jocky 0.1.0","run_start_utc":"2026-09-13T05:27:12Z","run_end_utc":"2026-09-13T05:27:12Z","status":"success","authorization":{"case":"probe1","allowed_capabilities":["test.exec"]},"inputs":[],"outputs":[],"executions":[{"function":"test_probe","capability":"test.exec","script_sha256":"e24010f8ec7d3278b2f22dd020f9ecaf80b14f70da9e900700be7569764b9b72","args":{"a":"$(touch /tmp/audit67/PWNED)","b":"x y; touch /tmp/audit67/PWNED2"},"outcome":"success","exit_code":0,"timed_out":false,"duration_ms":4,"stdout":"ARGS:$(touch /tmp/audit67/PWNED)|x y; touch /tmp/audit67/PWNED2\n","stderr":""}],"errors":[]}
```

Field verdict against the brief's list (`function, script_sha256, args,
exit_code, start_utc, end_utc, stdout_sha256, timed_out`):

- PRESENT: `function` ✓, `script_sha256` ✓ (per-execution),
  `args` ✓ (name→value object), `exit_code` ✓, `timed_out` ✓.
- MISSING: per-execution `start_utc`/`end_utc` — only run-level
  `run_start_utc`/`run_end_utc` exist; per-execution timing is
  `duration_ms` alone. `stdout_sha256` — stdout/stderr are captured as
  RAW TEXT (`"stdout":...`, `"stderr":...`), never hashed.
- EXTRA (present, unasked): `capability`, `outcome`, `duration_ms`,
  `stderr`; top-level `manifest_version`, `runtime_version`,
  `authorization{case, allowed_capabilities}`, `inputs[]`/`outputs[]`
  artifact records (`name/adapter/path/sha256/bytes`), `errors[]`.
- WART: the top-level key `"script_sha256"` serializes the **.jky
  source hash** (`write_manifest_json`, `dispatcher.hpp:284-285`:
  `out << ",\"script_sha256\":"; write_json_string(out,
  manifest.jky_sha256);`). Only the per-execution `script_sha256` is a
  script hash. Misleading name — rename or document before parity.

### 2.6 Failed / timed-out executions keep manifest entries — CONFIRMED

Live probes: `exit 3` script → `"outcome":"failure","exit_code":3`
with captured stderr (`oops`), entry present; `sleep 30`/timeout 2 →
`"outcome":"timeout","timed_out":true,"exit_code":124`, entry present;
plus `capability_denied`, `validation_denied`, `integrity_denied`
(paths in the project's suite), `evidence_write_denied`,
`ceiling_denied`, `dispatcher_failure` arms all persist the entry
(`persist_manifest` after every transition; manifest-before-spawn in
`dispatcher.hpp:781-783`). A performed audit run (`two_bin`, missing
evidence paths) returned `status=failed` with two `validation_denied`
entries — nothing silently dropped.

## 3. Gap summary (backlog before Phase 8 parity can be trusted)

Plain list, no softening. Numbers are stable section refs for the
backlog:

1. **[GAP-1] No WhileStmt iteration cap.** `wiki/17` §3 commitment
   unmet (§2.3 above). `max_executions` is not a substitute (counts
   dispatches, not iterations). MUST precede loop evaluation in Phase 8.
2. **[GAP-2] Runtime ignores control flow.** No branch selection, no loop
   iteration, no `for` multiplication — the static authorized list runs
   once each, so untaken-branch calls EXECUTE (§2.3 corollary).
   `wiki/20` admits it; Phase 8 must define and test branch/loop
   semantics, or parity has nothing to compare.
3. **[GAP-3] Manifest schema ≠ specified schema.** No per-execution
   start/end UTC, no `stdout_sha256` (raw stdout instead) (§2.5). Phase 8
   parity MUST diff the actual schema documented here, and either adopt
   it as the spec or extend the writer first.
4. **[GAP-4] No `registry_version_hash`.** No aggregate registry digest,
   no query flag (§1.3). Per-script hashes only.
5. **[GAP-5] Misnamed top-level `script_sha256`** (holds the .jky hash;
   §2.5 wart). Rename to `jky_sha256` (matching the struct field) or
   document; parity normalizers must not confuse it with script hashes.
6. **[GAP-6] `jocky verify <manifest.json>` unimplemented.** AGENTS.md §4
   advertises it; zero matches for `verify` in `src/` and `tools/`, and
   the `jocky` usage string lists only
   `check|resolve|gate|shake`. No manifest can currently be re-verified
   by the toolchain.
7. **[GAP-7] `jocky <file.jky>` interpret path missing** (expected — it IS
   Phase 8). Listed only because parity testing is impossible until both
   paths exist; not a Phase 6/7 defect.
8. **[CARRIED, not new] Case-metadata gap** (`wiki/13` §6): case blocks
   still carry only `allowed_capabilities`; manifests carry `case_id` +
   capability list, no analyst/authorization/timezone provenance.

What PASSED and needs no action: jockyc standalone generation (§1.1),
exact tree-shaken closure (§1.2), Linux scope docs (§1.4), clean embed
failure (§1.5), argv-without-interpolation (§2.1), per-script timeout
kill (§2.2), runtime capability re-check (§2.4), complete failure
entries (§2.6), and the 4/4 project suite in this environment.
