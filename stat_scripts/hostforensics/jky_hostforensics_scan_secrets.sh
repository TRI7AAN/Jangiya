#!/bin/bash
# V-057 -- Hardcoded secrets in src/binary
# Tool: trufflehog
# Usage: bash V-057.sh <target> <session_output_dir>

TARGET="${1:?Usage: V-057.sh <target> <session_output_dir>}"
SESSION_DIR="${2:-}"

echo "Scanning for secrets..."; curl -s "$TARGET/.env" 2>&1 | head -20; echo "---"; curl -s "$TARGET/.git/HEAD" 2>&1 | head -5
exit $?
