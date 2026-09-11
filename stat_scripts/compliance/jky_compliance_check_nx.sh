#!/bin/bash
# V-145 -- NX/Exec-shield verification
# Tool: checksec+paxtest
# Usage: bash V-145.sh <target> <session_output_dir>

TARGET="${1:?Usage: V-145.sh <target> <session_output_dir>}"
SESSION_DIR="${2:-}"

if [ -f "$TARGET" ]; then checksec --file="$TARGET" 2>&1; fi; echo "=== PaX ==="; paxtest blackhat 2>&1 | head -20
exit $?
