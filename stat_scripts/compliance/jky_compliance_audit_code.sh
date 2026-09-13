#!/bin/bash
# @jocky:function jky_compliance_audit_code
# @jocky:domain compliance
# @jocky:description Audit source tree via flawfinder and cppcheck
# @jocky:inputs target: path, session_dir: string = ""
# @jocky:outputs report: text
# @jocky:capability compliance.code.audit
# @jocky:timeout_seconds 300
# @jocky:depends_on
# V-143 -- Static source auditing
# Tool: flawfinder+cppcheck
# Usage: bash V-143.sh <target> <session_output_dir>

TARGET="${1:?Usage: V-143.sh <target> <session_output_dir>}"
SESSION_DIR="${2:-}"

echo "=== flawfinder ==="; flawfinder --quiet "$TARGET" 2>&1 | head -30; echo "=== cppcheck ==="; cppcheck --enable=warning,style,performance "$TARGET" 2>&1 | head -30
exit $?
