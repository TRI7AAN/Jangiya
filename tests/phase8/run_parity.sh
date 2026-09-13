#!/usr/bin/env bash
# Phase 8 parity driver: one fixture, both paths, exact diff.
# Usage: run_parity.sh <jocky> <jockyc> <source_root> <work> <fixture> <registry>
# All paths except the binaries may be relative; runs both sides from
# <source_root> so relative evidence/output paths resolve identically.
# Exit 0 iff the parity differ passes; captures $? immediately after
# every step (repo standing rule: no command substitution on $?) and
# prints each stage's raw exit for the record.
set -u

JOCKY=$1
JOCKYC=$2
SOURCE_ROOT=$3
WORK=$4
FIXTURE=$5
REGISTRY=$6

command -v python3 >/dev/null 2>&1
PY_OK=$?
if [ "$PY_OK" -ne 0 ]; then
  echo "PARITY ERROR: python3 not found (required by parity_diff.py)" >&2
  exit 2
fi

rm -rf "$WORK"
mkdir -p "$WORK/compiled" "$WORK/interpreted"

"$JOCKYC" "$SOURCE_ROOT/$FIXTURE" --registry "$SOURCE_ROOT/$REGISTRY" \
  -o "$WORK/compiled.bin" >"$WORK/build.log" 2>&1
BUILD_EXIT=$?
echo "BUILD_EXIT=$BUILD_EXIT"
if [ "$BUILD_EXIT" -ne 0 ]; then
  cat "$WORK/build.log" >&2
  echo "PARITY ERROR: jockyc build failed" >&2
  exit 1
fi

(
  cd "$SOURCE_ROOT"
  "$WORK/compiled.bin" run --output-root "$WORK/compiled" \
    --manifest "$WORK/compiled/manifest.json" >"$WORK/compiled.log" 2>&1
)
COMPILED_EXIT=$?
echo "COMPILED_EXIT=$COMPILED_EXIT"
cat "$WORK/compiled.log"

(
  cd "$SOURCE_ROOT"
  "$JOCKY" "$SOURCE_ROOT/$FIXTURE" --registry "$SOURCE_ROOT/$REGISTRY" \
    --output-root "$WORK/interpreted" \
    --manifest "$WORK/interpreted/manifest.json" \
    >"$WORK/interpreted.log" 2>&1
)
INTERP_EXIT=$?
echo "INTERP_EXIT=$INTERP_EXIT"
cat "$WORK/interpreted.log"

if [ ! -f "$WORK/compiled/manifest.json" ]; then
  echo "PARITY ERROR: compiled manifest missing" >&2
  exit 1
fi
if [ ! -f "$WORK/interpreted/manifest.json" ]; then
  echo "PARITY ERROR: interpreted manifest missing" >&2
  exit 1
fi

python3 "$SOURCE_ROOT/tests/phase8/parity_diff.py" \
  "$WORK/compiled/manifest.json" "$WORK/interpreted/manifest.json"
DIFF_EXIT=$?
echo "DIFF_EXIT=$DIFF_EXIT"

if [ "$COMPILED_EXIT" -ne "$INTERP_EXIT" ]; then
  echo "PARITY FAIL: exit codes differ: compiled=$COMPILED_EXIT interpreted=$INTERP_EXIT" >&2
  exit 1
fi
if [ "$DIFF_EXIT" -ne 0 ]; then
  exit 1
fi
echo "PARITY OK: $FIXTURE"
