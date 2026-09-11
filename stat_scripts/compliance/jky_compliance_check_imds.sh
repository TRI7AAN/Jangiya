#!/bin/bash
# V-109 -- IMDS abuse
# Tool: imds_curl_probe
# Usage: bash V-109.sh <target> <session_output_dir>

TARGET="${1:?Usage: V-109.sh <target> <session_output_dir>}"
SESSION_DIR="${2:-}"

echo "=== V-109: IMDS abuse ==="; echo "Tool: imds_curl_probe"; echo "Target: $TARGET"; which imds_curl_probe 2>/dev/null && echo "Tool found" || echo "Tool not found"
exit $?
