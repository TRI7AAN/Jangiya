#!/bin/bash
# V-088 -- Missing/incomplete SBOM
# Tool: syft
# Usage: bash V-088.sh <target> <session_output_dir>

TARGET="${1:?Usage: V-088.sh <target> <session_output_dir>}"
SESSION_DIR="${2:-}"

syft "$TARGET" -o json 2>&1 | head -50
exit $?
