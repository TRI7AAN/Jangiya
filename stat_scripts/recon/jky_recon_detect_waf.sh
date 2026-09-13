#!/bin/bash
# @jocky:function jky_recon_detect_waf
# @jocky:domain recon
# @jocky:description Detect web application firewall via wafw00f
# @jocky:inputs target: string, session_dir: string = ""
# @jocky:outputs waf_report: text
# @jocky:capability recon.waf.detect
# @jocky:timeout_seconds 120
# @jocky:depends_on
# V-051 -- WAF bypass
# Tool: wafw00f
# Usage: bash V-051.sh <target> <session_output_dir>

TARGET="${1:?Usage: V-051.sh <target> <session_output_dir>}"
SESSION_DIR="${2:-}"

wafw00f "$TARGET" -a 2>&1
exit $?
