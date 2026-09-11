#!/bin/bash
# V-074 -- Insecure cron/scheduled task
# Tool: linpeas
# Usage: bash V-074.sh <target> <session_output_dir>

TARGET="${1:?Usage: V-074.sh <target> <session_output_dir>}"
SESSION_DIR="${2:-}"

echo "=== LPE vectors ==="; find / -perm -4000 -type f 2>/dev/null | head -20; echo "---"; sudo -l 2>/dev/null | head -20
exit $?
