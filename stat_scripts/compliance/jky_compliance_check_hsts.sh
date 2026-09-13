#!/bin/bash
# @jocky:function jky_compliance_check_hsts
# @jocky:domain compliance
# @jocky:description Check HSTS header presence via curl across HTTPS endpoints and HTTP redirect
# @jocky:inputs target: string, session_dir: string = ""
# @jocky:outputs report: text
# @jocky:capability compliance.web.read
# @jocky:timeout_seconds 120
# @jocky:depends_on
# V-010 -- Missing HSTS header
# Tool: curl -I
# Usage: bash V-010.sh <target> <session_output_dir>
#
# Decision rule: grep_present
#   PASS = Strict-Transport-Security header present
#   FAIL = header missing

TARGET="${1:?Usage: V-010.sh <target> <session_output_dir>}"
SESSION_DIR="${2:-}"

HOST=$(echo "$TARGET" | sed -E 's|https?://||' | sed 's|/.*$||' | sed 's|:.*$||')

echo "=== V-010: Missing HSTS header ==="
echo "Target: $HOST"

# --- DNS Resolution ---
RESOLVED_IP=""
for DNS in 8.8.8.8 1.1.1.1 9.9.9.9; do
    IP=$(timeout 5 dig +short +time=3 +tries=1 "$HOST" "@${DNS}" 2>/dev/null | grep -E '^[0-9]+\.' | head -1)
    if [ -n "$IP" ]; then
        echo "[+] Resolved via $DNS: $IP"
        RESOLVED_IP="$IP"
        break
    fi
done

if [ -z "$RESOLVED_IP" ]; then
    RESOLVED_IP=$(timeout 5 getent hosts "$HOST" 2>/dev/null | awk '{print $1}' | head -1)
    [ -n "$RESOLVED_IP" ] && echo "[+] Resolved via system: $RESOLVED_IP"
fi

if [ -z "$RESOLVED_IP" ]; then
    echo "[-] DNS resolution failed"
    exit 1
fi

FOUND_HSTS=0

check_hsts() {
    local URL="$1"
    local LABEL="$2"

    echo ""
    echo "--- $LABEL ---"
    HEADERS=$(curl -sI -L --max-time 10 --connect-timeout 5 "$URL" 2>&1)
    echo "$HEADERS"

    # Check all responses (redirects included) for HSTS
    HSTS_LINE=$(echo "$HEADERS" | grep -i '^strict-transport-security:')
    if [ -n "$HSTS_LINE" ]; then
        echo "[PASS] HSTS header found: $HSTS_LINE"
        FOUND_HSTS=1
    else
        echo "[FAIL] No Strict-Transport-Security header in any response"
    fi
}

# --- Check HTTPS on common ports ---
if [[ ! "$TARGET" =~ ^https?:// ]]; then
    TARGET="https://$TARGET"
fi

check_hsts "$TARGET" "HTTPS (original target)"

# Also check alternate HTTPS ports
for PORT in 8443; do
    check_hsts "https://${HOST}:${PORT}/" "HTTPS port $PORT"
done

# --- Check HTTP -> HTTPS redirect for HSTS ---
HTTP_URL=$(echo "$TARGET" | sed 's|^https://|http://|')
check_hsts "$HTTP_URL" "HTTP (checking redirect response for HSTS)"

echo ""
if [ "$FOUND_HSTS" -eq 1 ]; then
    echo "=== RESULT: HSTS is configured ==="
else
    echo "=== RESULT: HSTS NOT configured (missing from all responses) ==="
fi

echo ""
echo "=== Scan Complete ==="
exit 0
