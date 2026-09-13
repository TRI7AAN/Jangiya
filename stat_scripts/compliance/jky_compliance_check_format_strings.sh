#!/bin/bash
# @jocky:function jky_compliance_check_format_strings
# @jocky:domain compliance
# @jocky:description Scan source tree for format-string flaws via flawfinder
# @jocky:inputs target: path, session_dir: string = ""
# @jocky:outputs report: text
# @jocky:capability compliance.code.audit
# @jocky:timeout_seconds 120
# @jocky:depends_on
# V-083 -- Format string vuln
# Tool: flawfinder
# Usage: bash V-083.sh <target> <session_output_dir>

TARGET="${1:?Usage: V-083.sh <target> <session_output_dir>}"
SESSION_DIR="${2:-}"

flawfinder --quiet "$TARGET" 2>&1 | head -30
exit $?
