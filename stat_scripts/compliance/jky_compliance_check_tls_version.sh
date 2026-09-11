#!/bin/bash
# V-006 -- Weak TLS/SSL version
# Tool: testssl.sh + openssl + nmap fallbacks
# Usage: bash V-006.sh <target> <session_output_dir>
#
# Decision rule: grep_absent
#   PASS = TLS 1.2+ only offered
#   FAIL = SSLv3 / TLS 1.0 / TLS 1.1 accepted

TARGET="${1:?Usage: V-006.sh <target> <session_output_dir>}"
SESSION_DIR="${2:-}"

HOST=$(echo "$TARGET" | sed -E 's|https?://||' | sed 's|/.*$||' | sed 's|:.*$||')
TIMEOUT=30

echo "=== V-006: Weak TLS/SSL version ==="
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

SCAN_TARGET="$RESOLVED_IP"

# --- Method 1: testssl.sh (if available) ---
echo ""
echo "=== Method 1: testssl.sh ==="
TESTSSL=""
for PATH_CANDIDATE in testssl.sh /usr/local/bin/testssl.sh /opt/testssl.sh/testssl.sh; do
    if command -v "$PATH_CANDIDATE" &>/dev/null; then
        TESTSSL="$PATH_CANDIDATE"
        break
    fi
done

if [ -n "$TESTSSL" ]; then
    # --fast: skip non-relevant checks, -p: include some ports, -U: no UNAPPScheck
    timeout 120 "$TESTSSL" --fast -p -U --sneaky -q "$TARGET" 2>&1 || true
else
    echo "[-] testssl.sh not found, skipping"
fi

# --- Method 2: openssl s_client direct TLS version probes ---
echo ""
echo "=== Method 2: openssl TLS version probes ==="

check_tls_version() {
    local version_flag="$1"
    local version_name="$2"
    local port="$3"

    RESULT=$(timeout 10 bash -c "
        echo '' | openssl s_client -connect ${SCAN_TARGET}:${port} \
            -servername $HOST \
            $version_flag \
            2>&1
    " 2>/dev/null)

    if echo "$RESULT" | grep -q "BEGIN CERTIFICATE"; then
        echo "[FAIL] $version_name: ACCEPTED (connection succeeded)"
        return 0
    elif echo "$RESULT" | grep -qiE "no protocols available|wrong version|no cipher|alert protocol|ssl handshake failure|tlsv1 alert"; then
        echo "[PASS] $version_name: REJECTED"
        return 1
    else
        echo "[INFO] $version_name: could not determine"
        return 2
    fi
}

for PORT in 443 8443; do
    echo ""
    echo "--- Port $PORT ---"
    check_tls_version "-ssl3"    "SSLv3"    "$PORT"
    check_tls_version "-tls1"    "TLS 1.0"  "$PORT"
    check_tls_version "-tls1_1"  "TLS 1.1"  "$PORT"
    check_tls_version "-tls1_2"  "TLS 1.2"  "$PORT"
    check_tls_version "-tls1_3"  "TLS 1.3"  "$PORT"
done

# --- Method 3: nmap ssl-enum-ciphers script ---
echo ""
echo "=== Method 3: nmap ssl-enum-ciphers ==="
SAFE_OPTS="-T3 -Pn --open --max-retries 2 --max-rate 100 --host-timeout 120s"
timeout 90 nmap -p 443,8443 $SAFE_OPTS \
    --script ssl-enum-ciphers "$SCAN_TARGET" 2>&1 || true

# --- Method 4: curl TLS version check ---
echo ""
echo "=== Method 4: curl TLS version probes ==="
for FLAG in "--tlsv1.0" "--tlsv1.1" "--tlsv1.2" "--tlsv1.3"; do
    RESULT=$(timeout 10 curl -sk -o /dev/null -w "%{http_code}" \
        $FLAG --connect-timeout 5 "https://${HOST}/" 2>/dev/null)
    if [ "$RESULT" != "000" ]; then
        echo "[FAIL] curl $FLAG: connected (HTTP $RESULT)"
    else
        echo "[PASS] curl $FLAG: rejected"
    fi
done

echo ""
echo "=== Scan Complete ==="
exit 0
