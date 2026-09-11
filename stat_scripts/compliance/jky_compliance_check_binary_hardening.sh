#!/bin/bash
# V-086 -- ROP exposure
# Tool: checksec
# Usage: bash V-086.sh <target> <session_output_dir>

TARGET="${1:?Usage: V-086.sh <target> <session_output_dir>}"
SESSION_DIR="${2:-}"

if [ -f "$TARGET" ]; then checksec --file="$TARGET" 2>&1; else cat /proc/sys/kernel/randomize_va_space 2>/dev/null; fi
exit $?
