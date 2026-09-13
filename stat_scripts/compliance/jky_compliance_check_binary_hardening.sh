#!/bin/bash
# @jocky:function jky_compliance_check_binary_hardening
# @jocky:domain compliance
# @jocky:description Report binary hardening via checksec, or kernel ASLR state for non-file targets
# @jocky:inputs target: path, session_dir: string = ""
# @jocky:outputs report: text
# @jocky:capability compliance.host.audit
# @jocky:timeout_seconds 60
# @jocky:depends_on
# V-086 -- ROP exposure
# Tool: checksec
# Usage: bash V-086.sh <target> <session_output_dir>

TARGET="${1:?Usage: V-086.sh <target> <session_output_dir>}"
SESSION_DIR="${2:-}"

if [ -f "$TARGET" ]; then checksec --file="$TARGET" 2>&1; else cat /proc/sys/kernel/randomize_va_space 2>/dev/null; fi
exit $?
