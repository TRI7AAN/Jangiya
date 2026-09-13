#!/usr/bin/env bash
# Phase 9 registry smoke test: invoke EVERY registered function through
# the real `jocky` interpreter against the real filesystem registry,
# one minimal program per function, and report per-function results.
#
# Usage: run_smoke.sh <jocky-bin> <source-root> <work-dir> [filter]
#   filter: optional substring; only functions whose name contains it run
#           (lets callers chunk the 38-function corpus: recon, netforensics,
#           hostforensics, compliance, timeline, report).
# Fixtures: tests/phase9/smoke_fixtures.json (per-function default args).
#
# Exit code contract: 0 iff every selected function was ATTEMPTED and
# produced a manifest entry (the no-unlogged-execution invariant holds
# for the whole run). Per-function PASS/FAIL is REPORTED, not encoded
# in the exit code — environment-caused script failures (missing tool
# binaries, no network in the sandbox) are expected data, not harness
# failure. A function with no manifest entry, or a harness error, exits
# nonzero. Never skips or omits a failure: every attempt lands in the
# summary table with its actual reason.
set -u

JOCKY=$1
SOURCE_ROOT=$2
WORK=$3
FILTER=${4:-}

command -v python3 >/dev/null 2>&1
PY_OK=$?
if [ "$PY_OK" -ne 0 ]; then
  echo "SMOKE ERROR: python3 not found" >&2
  exit 2
fi

mkdir -p "$WORK"
SUMMARY="$WORK/smoke_summary.tsv"
printf 'function\toutcome\texit_code\ttimed_out\tduration_ms\toutput_ok\treason\n' > "$SUMMARY"

COUNT=$(python3 -c "import json; print(len(json.load(open('$SOURCE_ROOT/tests/phase9/smoke_fixtures.json'))['fixtures']))")
echo "SMOKE: $COUNT fixtures loaded"

i=0
PASS=0
FAIL=0
while [ "$i" -lt "$COUNT" ]; do
  FN=$(python3 -c "import json; print(json.load(open('$SOURCE_ROOT/tests/phase9/smoke_fixtures.json'))['fixtures'][$i]['function'])")
  i=$((i + 1))
  case "$FN" in
    *"$FILTER"*) ;;
    *) continue ;;
  esac
  FDIR="$WORK/$FN"
  mkdir -p "$FDIR"
  python3 - "$SOURCE_ROOT/tests/phase9/smoke_fixtures.json" "$FDIR/prog.jky" "$FN" <<'PYEOF'
import json, sys
fixtures_path, prog_path, fn = sys.argv[1], sys.argv[2], sys.argv[3]
fixtures = json.load(open(fixtures_path))['fixtures']
fx = next(f for f in fixtures if f['function'] == fn)
lines = []
lines.append('case smoke_%s {' % fn.replace('.', '_'))
lines.append('  allowed_capabilities: ["%s"];' % fx['capability'])
lines.append('}')
lines.append('investigate smoke_%s {' % fn.replace('.', '_'))
call_args = []
for name, value in fx['args'].items():
    t = fx['arg_types'][name]
    if t in ('string', 'path'):
        lit = '"%s"' % value.replace('\\', '\\\\').replace('"', '\\"')
    else:
        lit = value
    call_args.append('%s: %s' % (name, lit))
lines.append('  let r = call %s(%s);' % (fn, ', '.join(call_args)))
lines.append('  r | emit fin;')
lines.append('}')
open(prog_path, 'w').write('\n'.join(lines) + '\n')
PYEOF
  "$JOCKY" "$FDIR/prog.jky" --registry "$SOURCE_ROOT/stat_scripts" \
    --output-root "$FDIR/out" --manifest "$FDIR/out/manifest.json" \
    >"$FDIR/run.log" 2>&1
  EXIT_CODE=$?
  python3 - "$FDIR/out/manifest.json" "$SUMMARY" "$FN" "$EXIT_CODE" "$SOURCE_ROOT/tests/phase9/smoke_fixtures.json" <<'PYEOF'
import json, sys
manifest_path, summary_path, fn, exit_code, fixtures_path = sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4], sys.argv[5]
try:
    m = json.load(open(manifest_path))
except Exception as ex:
    open(summary_path, 'a').write('%s\tNO_MANIFEST\t%s\t\t\t\tfail: manifest unreadable (%s)\n' % (fn, exit_code, ex))
    sys.exit(3)
entries = [e for e in m.get('executions', []) if e.get('function') == fn]
if not entries:
    open(summary_path, 'a').write('%s\tNO_ENTRY\t%s\t\t\t\tfail: no manifest entry for function\n' % (fn, exit_code))
    sys.exit(3)
e = entries[0]
outcome = e.get('outcome', '?')
code = e.get('exit_code', -999)
timed = e.get('timed_out', False)
dur = e.get('duration_ms', -1)
stdout = e.get('stdout', '')
otype = ''
for f in json.load(open(fixtures_path))['fixtures']:
    if f['function'] == fn:
        otype = f['output_type']
        break
output_ok = 'yes'
reason = 'outcome=%s' % outcome
if outcome != 'success':
    output_ok = 'n/a'
    reason = 'outcome=%s exit=%s stderr=%.120s' % (outcome, code, e.get('stderr', ''))
elif otype.startswith('table') or otype in ('json', 'csv'):
    if not stdout:
        output_ok = 'no'
        reason = 'empty stdout for declared %s' % otype
if outcome == 'success' and output_ok != 'no':
    verdict = 'PASS'
else:
    verdict = 'FAIL'
open(summary_path, 'a').write('%s\t%s\t%s\t%s\t%s\t%s\t%s: %s\n' % (fn, verdict, code, timed, dur, output_ok, exit_code, reason))
sys.exit(0 if verdict == 'PASS' else 4)
PYEOF
  ROW_EXIT=$?
  if [ "$ROW_EXIT" -eq 0 ]; then
    PASS=$((PASS + 1))
  elif [ "$ROW_EXIT" -eq 4 ]; then
    FAIL=$((FAIL + 1))
  else
    echo "SMOKE ERROR: harness failure on $FN (row exit $ROW_EXIT)" >&2
    exit 1
  fi
  echo "SMOKE [$FN] row_exit=$ROW_EXIT"
done

echo "SMOKE COMPLETE: pass=$PASS fail=$FAIL (see $SUMMARY)"
cat "$SUMMARY"
exit 0
