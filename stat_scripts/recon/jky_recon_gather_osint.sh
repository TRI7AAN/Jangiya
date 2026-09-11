#!/bin/bash
# V-001 -- Sensitive data via OSINT
# Tool: theHarvester
# Usage: bash V-001.sh <target> <session_output_dir>

TARGET="${1:?Usage: V-001.sh <target> <session_output_dir>}"
SESSION_DIR="${2:-}"
HOST=$(echo "$TARGET" | sed -E 's|https?://||' | sed 's|/.*$||' | sed 's|:.*$||')

theHarvester -d "$HOST" -b all -f /dev/stdout 2>&1
exit $?
