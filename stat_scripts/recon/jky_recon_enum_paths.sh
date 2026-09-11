#!/bin/bash
# V-067 -- Path/directory traversal
# Tool: dirsearch
# Usage: bash V-067.sh <target> <session_output_dir>

TARGET="${1:?Usage: V-067.sh <target> <session_output_dir>}"
SESSION_DIR="${2:-}"

if [[ ! "$TARGET" =~ ^https?:// ]]; then TARGET="http://$TARGET"; fi
dirsearch -u "$TARGET" -e php,html,js,txt -t 10 2>&1 | tail -30
exit $?
