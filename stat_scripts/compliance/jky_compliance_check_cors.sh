#!/bin/bash
# V-050 -- CORS misconfig
# Tool: origin_header_probe
# Usage: bash V-050.sh <target> <session_output_dir>

TARGET="${1:?Usage: V-050.sh <target> <session_output_dir>}"
SESSION_DIR="${2:-}"

echo "=== V-050: CORS misconfig ==="; echo "Tool: origin_header_probe"; echo "Target: $TARGET"; which origin_header_probe 2>/dev/null && echo "Tool found" || echo "Tool not found"
exit $?
