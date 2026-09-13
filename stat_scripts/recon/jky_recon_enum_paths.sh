#!/bin/bash
# @jocky:function jky_recon_enum_paths
# @jocky:domain recon
# @jocky:description Enumerate web paths via dirsearch wordlist scan
# @jocky:inputs target: string, session_dir: string = ""
# @jocky:outputs paths: text
# @jocky:capability recon.path.enumerate
# @jocky:timeout_seconds 300
# @jocky:depends_on
# V-067 -- Path/directory traversal
# Tool: dirsearch
# Usage: bash V-067.sh <target> <session_output_dir>

TARGET="${1:?Usage: V-067.sh <target> <session_output_dir>}"
SESSION_DIR="${2:-}"

if [[ ! "$TARGET" =~ ^https?:// ]]; then TARGET="http://$TARGET"; fi
dirsearch -u "$TARGET" -e php,html,js,txt -t 10 2>&1 | tail -30
exit $?
