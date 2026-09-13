#!/bin/bash
# @jocky:function jky_compliance_scan_dependencies
# @jocky:domain compliance
# @jocky:description Scan dependencies for known vulnerabilities via syft and grype
# @jocky:inputs target: path, session_dir: string = ""
# @jocky:outputs report: text
# @jocky:capability compliance.code.audit
# @jocky:timeout_seconds 300
# @jocky:depends_on
# V-087 -- Vulnerable dependencies/SCA
# Tool: grype+syft
# Usage: bash V-087.sh <target> <session_output_dir>

TARGET="${1:?Usage: V-087.sh <target> <session_output_dir>}"
SESSION_DIR="${2:-}"

if [ -d "$TARGET" ]; then syft "$TARGET" -o json 2>/dev/null | grype 2>&1 | head -30; else grype "$TARGET" 2>&1 | head -20; fi
exit $?
