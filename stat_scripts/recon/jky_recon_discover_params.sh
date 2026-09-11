#!/bin/bash
# V-054 -- Mass assignment
# Tool: arjun
# Usage: bash V-054.sh <target> <session_output_dir>

TARGET="${1:?Usage: V-054.sh <target> <session_output_dir>}"
SESSION_DIR="${2:-}"

if [[ ! "$TARGET" =~ ^https?:// ]]; then TARGET="http://$TARGET"; fi
arjun -u "$TARGET" -t 10 2>&1 | tail -20
exit $?
