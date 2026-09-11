#!/bin/bash
# V-051 -- WAF bypass
# Tool: wafw00f
# Usage: bash V-051.sh <target> <session_output_dir>

TARGET="${1:?Usage: V-051.sh <target> <session_output_dir>}"
SESSION_DIR="${2:-}"

wafw00f "$TARGET" -a 2>&1
exit $?
