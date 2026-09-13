#!/bin/bash
# @jocky:function jky_compliance_build_sbom
# @jocky:domain compliance
# @jocky:description Generate software bill of materials via syft truncated JSON
# @jocky:inputs target: path, session_dir: string = ""
# @jocky:outputs report: text
# @jocky:capability compliance.code.audit
# @jocky:timeout_seconds 180
# @jocky:depends_on
# V-088 -- Missing/incomplete SBOM
# Tool: syft
# Usage: bash V-088.sh <target> <session_output_dir>

TARGET="${1:?Usage: V-088.sh <target> <session_output_dir>}"
SESSION_DIR="${2:-}"

syft "$TARGET" -o json 2>&1 | head -50
exit $?
