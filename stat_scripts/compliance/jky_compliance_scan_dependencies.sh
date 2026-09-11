#!/bin/bash
# V-087 -- Vulnerable dependencies/SCA
# Tool: grype+syft
# Usage: bash V-087.sh <target> <session_output_dir>

TARGET="${1:?Usage: V-087.sh <target> <session_output_dir>}"
SESSION_DIR="${2:-}"

if [ -d "$TARGET" ]; then syft "$TARGET" -o json 2>/dev/null | grype 2>&1 | head -30; else grype "$TARGET" 2>&1 | head -20; fi
exit $?
