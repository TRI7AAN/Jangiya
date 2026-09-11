#!/bin/bash
# V-083 -- Format string vuln
# Tool: flawfinder
# Usage: bash V-083.sh <target> <session_output_dir>

TARGET="${1:?Usage: V-083.sh <target> <session_output_dir>}"
SESSION_DIR="${2:-}"

flawfinder --quiet "$TARGET" 2>&1 | head -30
exit $?
