#!/bin/bash
# @jocky:function jky_hostforensics_check_apk_hardening
# @jocky:domain hostforensics
# @jocky:description Print truncated jadx output for a regular-file target, silently empty otherwise
# @jocky:inputs target: path, session_dir: string = ""
# @jocky:outputs findings: text
# @jocky:capability hostforensics.app.audit
# @jocky:timeout_seconds 120
# @jocky:depends_on
# V-123 -- Lack of obfuscation/anti-tamper
# Tool: jadx+apktool
# Usage: bash V-123.sh <target> <session_output_dir>

TARGET="${1:?Usage: V-123.sh <target> <session_output_dir>}"
SESSION_DIR="${2:-}"

if [ -f "$TARGET" ]; then jadx "$TARGET" 2>&1 | head -20; fi
exit $?
