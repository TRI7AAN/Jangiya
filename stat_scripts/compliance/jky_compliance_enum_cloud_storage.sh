#!/bin/bash
# V-106 -- Public cloud storage exposure
# Tool: cloud_enum
# Usage: bash V-106.sh <target> <session_output_dir>

TARGET="${1:?Usage: V-106.sh <target> <session_output_dir>}"
SESSION_DIR="${2:-}"

cloud_enum -k "$TARGET" 2>&1 | head -30
exit $?
