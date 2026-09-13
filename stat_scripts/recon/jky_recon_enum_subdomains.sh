#!/bin/bash
# @jocky:function jky_recon_enum_subdomains
# @jocky:domain recon
# @jocky:description Enumerate subdomains via subfinder silent scan
# @jocky:inputs target: string, session_dir: string = ""
# @jocky:outputs subdomains: text
# @jocky:capability recon.subdomain.enumerate
# @jocky:timeout_seconds 300
# @jocky:depends_on
# V-002 -- Sub-domain enumeration
# Tool: subfinder
# Usage: bash V-002.sh <target> <session_output_dir>

TARGET="${1:?Usage: V-002.sh <target> <session_output_dir>}"
SESSION_DIR="${2:-}"
HOST=$(echo "$TARGET" | sed -E 's|https?://||' | sed 's|/.*$||' | sed 's|:.*$||')

subfinder -d "$HOST" -silent 2>&1
exit $?
