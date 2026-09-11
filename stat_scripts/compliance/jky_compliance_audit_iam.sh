#!/bin/bash
# V-107 -- IAM/policy misconfig
# Tool: scoutsuite
# Usage: bash V-107.sh <target> <session_output_dir>

TARGET="${1:?Usage: V-107.sh <target> <session_output_dir>}"
SESSION_DIR="${2:-}"

echo "Cloud security audit"; scout aws --profile default 2>&1 | head -20 || echo "not configured"
exit $?
