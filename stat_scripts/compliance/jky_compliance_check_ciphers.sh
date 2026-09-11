#!/bin/bash
# V-007 -- Weak cipher suite
# Tool: testssl.sh + openssl + nmap fallbacks
# Usage: bash V-007.sh <target> <session_output_dir>
#
# Decision rule: grep_absent
#   PASS = strong ciphers only
#   FAIL = weak/NULL/EXPORT cipher offered

TARGET="${1:?Usage: V-007.sh <target> <session_output_dir>}"
SESSION_DIR="${2:-}"

HOST=$(echo "$TARGET" | sed -E 's|https?://||' | sed 's|/.*$||' | sed 's|:.*$||')
TIMEOUT=30

echo "=== V-007: Weak cipher suite ==="
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
    timeout 120 "$TESTSSL" --fast -U --sneaky -q "$TARGET" 2>&1 | grep -iE 'cipher|NULL|EXPORT|anonymous|RC4|DES|MD5|WEAK|strength' || true
else
    echo "[-] testssl.sh not found, skipping"
fi

# --- Method 2: openssl cipher string probes ---
echo ""
echo "=== Method 2: openssl weak cipher probes ==="

for PORT in 443 8443; do
    echo ""
    echo "--- Port $PORT ---"

    for CIPHER_GROUP in "NULL" "EXPORT" "aNULL" "eNULL" "RC4" "DES" "3DES" "MD5" "ADH" "AECDH"; do
        RESULT=$(timeout 10 bash -c "
            echo '' | openssl s_client -connect ${SCAN_TARGET}:${PORT} \
                -servername $HOST \
                -cipher ${CIPHER_GROUP} \
                2>&1
        " 2>/dev/null)

        if echo "$RESULT" | grep -q "BEGIN CERTIFICATE"; then
            echo "[FAIL] $CIPHER_GROUP: ACCEPTED (connection succeeded)"
        else
            echo "[PASS] $CIPHER_GROUP: rejected"
        fi
    done

    # Check for RC4/3DES specifically
    RESULT=$(timeout 10 bash -c "
        echo '' | openssl s_client -connect ${SCAN_TARGET}:${PORT} \
            -servername $HOST \
            -cipher RC4:3DES \
            2>&1
    " 2>/dev/null)

    if echo "$RESULT" | grep -q "BEGIN CERTIFICATE"; then
        echo "[FAIL] RC4/3DES: ACCEPTED"
    else
        echo "[PASS] RC4/3DES: rejected"
    fi
done

# --- Method 3: nmap ssl-enum-ciphers ---
echo ""
echo "=== Method 3: nmap ssl-enum-ciphers ==="
SAFE_OPTS="-T3 -Pn --open --max-retries 2 --max-rate 100 --host-timeout 120s"
timeout 90 nmap -p 443,8443 $SAFE_OPTS \
    --script ssl-enum-ciphers "$SCAN_TARGET" 2>&1 || true

# --- Method 4: openssl full cipher list check ---
echo ""
echo "=== Method 4: openssl full cipher enumeration ==="
for PORT in 443 8443; do
    echo ""
    echo "--- Port $PORT ---"
    CIPHERS=$(timeout 10 bash -c "
        echo '' | openssl s_client -connect ${SCAN_TARGET}:${PORT} \
            -servername $HOST \
            2>&1
    " 2>/dev/null | grep -i 'Cipher is' || true)
    echo "$CIPHERS"

    # Check negotiated cipher for weak properties
    NEGOTIATED=$(echo "$CIPHERS" | awk -F': ' '{print $2}')
    if echo "$NEGOTIATED" | grep -qiE 'RC4|DES|MD5|NULL|EXPORT|anon'; then
        echo "[FAIL] Negotiated cipher is weak: $NEGOTIATED"
    elif [ -n "$NEGOTIATED" ]; then
        echo "[PASS] Negotiated cipher appears strong: $NEGOTIATED"
    fi
done

echo ""
echo "=== Scan Complete ==="
exit 0
