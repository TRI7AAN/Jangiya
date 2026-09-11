#!/bin/bash
# V-123 -- Lack of obfuscation/anti-tamper
# Tool: jadx+apktool
# Usage: bash V-123.sh <target> <session_output_dir>

TARGET="${1:?Usage: V-123.sh <target> <session_output_dir>}"
SESSION_DIR="${2:-}"

if [ -f "$TARGET" ]; then jadx "$TARGET" 2>&1 | head -20; fi
exit $?
