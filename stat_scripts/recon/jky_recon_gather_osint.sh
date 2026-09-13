#!/bin/bash
# @jocky:function jky_recon_gather_osint
# @jocky:domain recon
# @jocky:description Gather target OSINT via theHarvester all-sources scan
# @jocky:inputs target: string, session_dir: string = ""
# @jocky:outputs osint: text
# @jocky:capability recon.osint.gather
# @jocky:timeout_seconds 300
# @jocky:depends_on
# V-001 -- Sensitive data via OSINT
# Tool: theHarvester
# Usage: bash V-001.sh <target> <session_output_dir>

TARGET="${1:?Usage: V-001.sh <target> <session_output_dir>}"
SESSION_DIR="${2:-}"
HOST=$(echo "$TARGET" | sed -E 's|https?://||' | sed 's|/.*$||' | sed 's|:.*$||')

theHarvester -d "$HOST" -b all -f /dev/stdout 2>&1
exit $?
