#!/bin/bash
# V-008 -- Expired/self-signed certificate
# Tool: openssl s_client
# Usage: bash V-008.sh <target> <session_output_dir>

TARGET="${1:?Usage: V-008.sh <target> <session_output_dir>}"
SESSION_DIR="${2:-}"
HOST=$(echo "$TARGET" | sed -E 's|https?://||' | sed 's|/.*$||' | sed 's|:.*$||')

echo | openssl s_client -connect "$HOST:443" -servername "$HOST" 2>&1 | head -50
echo "--- cert details ---"
echo | openssl s_client -connect "$HOST:443" -servername "$HOST" 2>&1 | openssl x509 -noout -dates -subject -issuer 2>&1
exit $?
