#!/bin/bash
# V-069 -- LPE
# Tool: peass-ng
# Usage: bash V-069.sh <target> <session_output_dir>

TARGET="${1:?Usage: V-069.sh <target> <session_output_dir>}"
SESSION_DIR="${2:-}"

echo "=== LPE vectors ==="; find / -perm -4000 -type f 2>/dev/null | head -20; echo "---"; sudo -l 2>/dev/null | head -20
exit $?
