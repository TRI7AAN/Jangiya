#!/bin/bash
# V-143 -- Static source auditing
# Tool: flawfinder+cppcheck
# Usage: bash V-143.sh <target> <session_output_dir>

TARGET="${1:?Usage: V-143.sh <target> <session_output_dir>}"
SESSION_DIR="${2:-}"

echo "=== flawfinder ==="; flawfinder --quiet "$TARGET" 2>&1 | head -30; echo "=== cppcheck ==="; cppcheck --enable=warning,style,performance "$TARGET" 2>&1 | head -30
exit $?
