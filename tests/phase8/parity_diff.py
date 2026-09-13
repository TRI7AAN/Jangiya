#!/usr/bin/env python3
"""Phase 8 parity differ: compiled vs. interpreted manifests.

Comparison rules (locked by the Phase 8 brief, decision #3):
- Compared EXACTLY: every execution's function name, capability,
  script_sha256, args, exit_code, outcome, timed_out, stdout,
  stdout_sha256, stderr; total count AND order of executions (loop
  iterations and taken-vs-untaken branch behavior included); top-level
  case_id, program_sha256, status, authorization, inputs, outputs,
  errors, manifest_version, runtime_version.
- EXCLUDED (timing only): run_start_utc, run_end_utc, and per-execution
  start_utc, end_utc, duration_ms.
- script_sha256 equality is additionally asserted as its own explicit
  check (decision #4): the compiled path's embedded copy must be an
  unmodified, faithful copy of what the interpreter ran from disk.

Exit 0 + "PARITY PASS" on full agreement; exit 1 + unified diff plus
per-assertion failures otherwise. Always prints the concrete per-entry
(function/outcome) projection so a pass shows what matched, not just
an exit code.
"""
import difflib
import json
import re
import sys

TIMING_TOP = {"run_start_utc", "run_end_utc"}
TIMING_EXEC = {"start_utc", "end_utc", "duration_ms"}
SHA64 = re.compile(r"^[0-9a-f]{64}$")


def project_execution(entry):
    return {k: v for k, v in entry.items() if k not in TIMING_EXEC}


def project_manifest(manifest):
    projected = {k: v for k, v in manifest.items() if k not in TIMING_TOP}
    projected["executions"] = [
        project_execution(e) for e in manifest.get("executions", [])
    ]
    return projected


def main(compiled_path, interpreted_path):
    failures = []
    with open(compiled_path, encoding="utf-8") as handle:
        compiled = json.load(handle)
    with open(interpreted_path, encoding="utf-8") as handle:
        interpreted = json.load(handle)

    compiled_exec = compiled.get("executions", [])
    interpreted_exec = interpreted.get("executions", [])
    print(
        "compiled executions:   "
        + ", ".join(
            "%s/%s" % (e.get("function"), e.get("outcome"))
            for e in compiled_exec
        )
    )
    print(
        "interpreted executions: "
        + ", ".join(
            "%s/%s" % (e.get("function"), e.get("outcome"))
            for e in interpreted_exec
        )
    )

    # Dedicated script_sha256 assertion (decision #4), separate from the
    # general diff: index-wise equality, plus well-formedness (64-hex
    # for every real script entry on BOTH sides; the <while-loop>
    # ceiling marker carries "" on both sides by construction).
    if len(compiled_exec) != len(interpreted_exec):
        failures.append(
            "execution count differs: compiled=%d interpreted=%d"
            % (len(compiled_exec), len(interpreted_exec))
        )
    for index, (left, right) in enumerate(zip(compiled_exec, interpreted_exec)):
        left_sha = left.get("script_sha256", None)
        right_sha = right.get("script_sha256", None)
        if left_sha != right_sha:
            failures.append(
                "entry %d script_sha256 differs: compiled=%r interpreted=%r"
                % (index, left_sha, right_sha)
            )
        elif left.get("function") == "<while-loop>":
            if left_sha != "":
                failures.append(
                    "entry %d: ceiling marker must carry empty script_sha256, got %r"
                    % (index, left_sha)
                )
        else:
            for side, sha in (("compiled", left_sha), ("interpreted", right_sha)):
                if not isinstance(sha, str) or not SHA64.match(sha):
                    failures.append(
                        "entry %d: %s script_sha256 is not a 64-hex digest: %r"
                        % (index, side, sha)
                    )

    left_text = json.dumps(
        project_manifest(compiled), sort_keys=True, indent=2
    ).splitlines()
    right_text = json.dumps(
        project_manifest(interpreted), sort_keys=True, indent=2
    ).splitlines()
    diff = list(
        difflib.unified_diff(
            left_text,
            right_text,
            fromfile="compiled",
            tofile="interpreted",
            lineterm="",
        )
    )
    if diff:
        failures.append(
            "projected manifests differ (%d diff lines):\n%s"
            % (len(diff), "\n".join(diff))
        )

    if failures:
        print("PARITY FAIL (%d assertion(s)):" % len(failures))
        for failure in failures:
            print("--- " + failure)
        return 1
    print(
        "PARITY PASS: %d executions agree exactly "
        "(timing fields excluded)" % len(compiled_exec)
    )
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1], sys.argv[2]))
