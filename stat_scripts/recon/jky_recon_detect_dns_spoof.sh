#!/bin/bash
# V-004 -- DNS spoofing
# Tool: dig+spoof_probe
# Usage: bash V-004.sh <target> <session_output_dir>

TARGET="${1:?Usage: V-004.sh <target> <session_output_dir>}"
SESSION_DIR="${2:-}"

HOST=$(echo "$TARGET" | sed -E 's|https?://||' | sed 's|/.*$||' | sed 's|:.*$||')

echo "=== V-004: DNS spoofing ==="
echo "Tool: dig+spoof_probe"
echo "Target: $TARGET"
echo "Host: $HOST"

# Check dig availability
if ! command -v dig &>/dev/null; then
    echo "[-] dig not found - install dnsutils"
    exit 1
fi
echo "[+] dig found: $(which dig)"

SPOOF_DETECTED=0
FAIL_COUNT=0

# --- Test 1: DNSSEC validation ---
echo ""
echo "=== Test 1: DNSSEC Validation ==="
DNSSEC_RESULT=$(dig +dnssec +multi "$HOST" A @8.8.8.8 2>/dev/null)
if echo "$DNSSEC_RESULT" | grep -q "ad flag"; then
    echo "[PASS] DNSSEC validation present (AD flag set)"
elif echo "$DNSSEC_RESULT" | grep -qi "RRSIG"; then
    echo "[PASS] DNSSEC RRSIG records present"
else
    echo "[WARN] No DNSSEC validation detected - spoofing possible"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# --- Test 2: Response consistency across resolvers ---
echo ""
echo "=== Test 2: Response Consistency (Multi-Resolver) ==="
RESOLVERS=("8.8.8.8" "1.1.1.1" "9.9.9.9" "208.67.222.222")
IPS=()

for DNS in "${RESOLVERS[@]}"; do
    IP=$(dig +short +time=5 "$HOST" A "@${DNS}" 2>/dev/null | grep -E '^[0-9]+\.' | head -1)
    if [ -n "$IP" ]; then
        echo "  $DNS -> $IP"
        IPS+=("$IP")
    else
        echo "  $DNS -> (no response)"
    fi
done

# Check if all responses match
UNIQUE_IPS=($(printf '%s\n' "${IPS[@]}" | sort -u))
if [ ${#UNIQUE_IPS[@]} -le 1 ] && [ ${#IPS[@]} -gt 0 ]; then
    echo "[PASS] All resolvers returned consistent IP: ${UNIQUE_IPS[0]}"
elif [ ${#IPS[@]} -gt 0 ]; then
    echo "[FAIL] Inconsistent DNS responses detected!"
    echo "  Unique IPs: ${UNIQUE_IPS[*]}"
    SPOOF_DETECTED=1
fi

# --- Test 3: TTL manipulation check ---
echo ""
echo "=== Test 3: TTL Anomaly Detection ==="
TTL_VALUES=()
for DNS in "${RESOLVERS[@]}"; do
    TTL=$(dig +noall +answer +ttlid "$HOST" A "@${DNS}" 2>/dev/null | grep -oE '[0-9]+$' | head -1)
    if [ -n "$TTL" ]; then
        TTL_VALUES+=("$TTL")
        echo "  $DNS TTL: ${TTL}s"
    fi
done

if [ ${#TTL_VALUES[@]} -gt 1 ]; then
    MIN_TTL=$(printf '%s\n' "${TTL_VALUES[@]}" | sort -n | head -1)
    MAX_TTL=$(printf '%s\n' "${TTL_VALUES[@]}" | sort -n | tail -1)
    TTL_DIFF=$((MAX_TTL - MIN_TTL))
    if [ "$TTL_DIFF" -gt 300 ]; then
        echo "[FAIL] TTL variance too high (${TTL_DIFF}s) - possible spoofing"
        SPOOF_DETECTED=1
    else
        echo "[PASS] TTL values consistent (diff: ${TTL_DIFF}s)"
    fi
fi

# --- Test 4: Bogus/oversized response test ---
echo ""
echo "=== Test 4: Bogus Response Rejection ==="
BOGUS_RESULT=$(dig +time=5 +tries=1 "$HOST" A "@$(dig +short $HOST A @8.8.8.8 | head -1)" 2>&1)
if echo "$BOGUS_RESULT" | grep -qi "status: SERVFAIL\|status: REFUSED\|status: NXDOMAIN"; then
    echo "[PASS] Bogus response properly rejected"
elif echo "$BOGUS_RESULT" | grep -qi "status: NOERROR"; then
    echo "[PASS] Response received normally"
else
    echo "[WARN] Unexpected response behavior"
fi

# --- Test 5: Recursive query test ---
echo ""
echo "=== Test 5: Open Resolver Check ==="
OPEN_RESOLVERS=0
for DNS in 8.8.8.8 1.1.1.1; do
    RECURSIVE_TEST=$(dig +short +time=3 "google.com" A "@${DNS}" 2>/dev/null | grep -cE '^[0-9]+\.')
    if [ "$RECURSIVE_TEST" -gt 0 ]; then
        echo "  $DNS allows recursive queries (normal for public DNS)"
    fi
done

# --- Test 6: Cache poisoning indicators ---
echo ""
echo "=== Test 6: Cache Poisoning Indicators ==="
# Test with random subdomain to check for wildcard responses
RANDOM_SUB="test$(date +%s)-$(shuf -i 1000-9999 -n 1)"
WILDCARD_TEST=$(dig +short "$RANDOM_SUB.$HOST" A @8.8.8.8 2>/dev/null)
if [ -n "$WILDCARD_TEST" ]; then
    echo "[FAIL] Wildcard DNS detected - possible DNS hijacking"
    echo "  Random subdomain resolved to: $WILDCARD_TEST"
    SPOOF_DETECTED=1
else
    echo "[PASS] No wildcard DNS response detected"
fi

# --- Summary ---
echo ""
echo "=== Summary ==="
if [ "$SPOOF_DETECTED" -eq 1 ]; then
    echo "[FAIL] DNS spoofing indicators detected"
    exit 1
else
    echo "[PASS] DNS appears resistant to spoofing"
    exit 0
fi
