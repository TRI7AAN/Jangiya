#!/bin/bash
# @jocky:function jky_recon_check_jdwp
# @jocky:domain recon
# @jocky:description Check JDWP exposure on ports 8000-9999 via nmap
# @jocky:inputs target: string, session_dir: string = ""
# @jocky:outputs jdwp_report: text
# @jocky:capability recon.jdwp.scan
# @jocky:timeout_seconds 300
# @jocky:depends_on
# V-118 -- JDWP exposure
# Tool: nmap_jdwp
# Usage: bash V-118.sh <target> <session_output_dir>

TARGET="${1:?Usage: V-118.sh <target> <session_output_dir>}"
SESSION_DIR="${2:-}"
HOST=$(echo "$TARGET" | sed -E 's|https?://||' | sed 's|/.*$||' | sed 's|:.*$||')
nmap -p 8000-9999 --script jdwp-info "$HOST" 2>&1 | head -30
exit $?
