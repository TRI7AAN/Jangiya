#!/bin/bash
# @jocky:function jky_hostforensics_scan_secrets
# @jocky:domain hostforensics
# @jocky:description Fetch remote .env and git HEAD via curl and print truncated contents
# @jocky:inputs target: string, session_dir: string = ""
# @jocky:outputs findings: text
# @jocky:capability hostforensics.secret.scan
# @jocky:timeout_seconds 60
# @jocky:depends_on
# V-057 -- Hardcoded secrets in src/binary
# Tool: trufflehog
# Usage: bash V-057.sh <target> <session_output_dir>

TARGET="${1:?Usage: V-057.sh <target> <session_output_dir>}"
SESSION_DIR="${2:-}"

echo "Scanning for secrets..."; curl -s "$TARGET/.env" 2>&1 | head -20; echo "---"; curl -s "$TARGET/.git/HEAD" 2>&1 | head -5
exit $?
