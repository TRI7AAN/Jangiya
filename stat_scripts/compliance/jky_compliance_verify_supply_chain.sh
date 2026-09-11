#!/bin/bash
# V-114 -- Artifact/dependency poisoning
# Tool: cosign+syft
# Usage: bash V-114.sh <target> <session_output_dir>

TARGET="${1:?Usage: V-114.sh <target> <session_output_dir>}"
SESSION_DIR="${2:-}"

syft "$TARGET" -o json 2>&1 | head -50
exit $?
