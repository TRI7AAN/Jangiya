#!/bin/bash
# V-113 -- Build secret exposure
# Tool: trufflehog_cicd
# Usage: bash V-113.sh <target> <session_output_dir>

TARGET="${1:?Usage: V-113.sh <target> <session_output_dir>}"
SESSION_DIR="${2:-}"

echo "Scanning CI/CD for secrets..."; curl -s "$TARGET/.env" 2>&1 | head -10; echo "---"; curl -s "$TARGET/.git/config" 2>&1 | head -10
exit $?
