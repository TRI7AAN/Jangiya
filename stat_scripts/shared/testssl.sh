#!/bin/bash
# Shared wrapper: testssl.sh TLS/SSL scanner
# Called by stat_scripts/V-006.sh, V-007.sh
# Usage: bash testssl.sh <target> <session_output_dir>

TARGET="${1:?Usage: testssl.sh <target> <session_output_dir>}"
SESSION_DIR="${2:-}"

TESTSSL=$(which testssl.sh 2>/dev/null || which testssl 2>/dev/null || echo "/usr/bin/testssl.sh")
if [ ! -x "$TESTSSL" ]; then
  echo "ERROR: testssl.sh not found"
  exit 1
fi

"$TESTSSL" --quiet --color 0 "$TARGET" 2>&1
exit $?
