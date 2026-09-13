#!/bin/bash
# @jocky:function jky_recon_discover_admin
# @jocky:domain recon
# @jocky:description Discover exposed admin interfaces via port scan and path probes
# @jocky:inputs target: string, session_dir: string = ""
# @jocky:outputs admin_report: text
# @jocky:capability recon.admin.discover
# @jocky:timeout_seconds 180
# @jocky:depends_on
# V-071 -- Exposed admin interface
# Tool: nmap+httpx
# Usage: bash V-071.sh <target> <session_output_dir>

TARGET="${1:?Usage: V-071.sh <target> <session_output_dir>}"
SESSION_DIR="${2:-}"
HOST=$(echo "$TARGET" | sed -E 's|https?://||' | sed 's|/.*$||' | sed 's|:.*$||')
nmap -sV -T4 --open "$HOST" 2>&1 | head -30
for path in /admin /administrator /wp-admin /phpmyadmin /console /manage; do code=$(curl -s -o /dev/null -w "%{http_code}" "http://$HOST$path"); [ "$code" != "000" ] && [ "$code" != "404" ] && echo "  $path -> HTTP $code"; done
exit $?
