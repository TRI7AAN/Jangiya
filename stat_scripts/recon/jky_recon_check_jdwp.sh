#!/bin/bash
# V-118 -- JDWP exposure
# Tool: nmap_jdwp
# Usage: bash V-118.sh <target> <session_output_dir>

TARGET="${1:?Usage: V-118.sh <target> <session_output_dir>}"
SESSION_DIR="${2:-}"
HOST=$(echo "$TARGET" | sed -E 's|https?://||' | sed 's|/.*$||' | sed 's|:.*$||')
nmap -p 8000-9999 --script jdwp-info "$HOST" 2>&1 | head -30
exit $?
