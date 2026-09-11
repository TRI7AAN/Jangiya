#!/bin/bash
# V-005 -- Banner grabbing/fingerprinting
# Tool: nmap -sV + banner probes
# Usage: bash V-005.sh <target> <session_output_dir>
#
# Decision rule: grep_absent
#   PASS = no verbose version banner exposed
#   FAIL = verbose version string exposed (e.g. "Apache/2.4.41", "ISC BIND 9.18.36")

TARGET="${1:?Usage: V-005.sh <target> <session_output_dir>}"
SESSION_DIR="${2:-}"

HOST=$(echo "$TARGET" | sed -E 's|https?://||' | sed 's|/.*$||' | sed 's|:.*$||')
TIMEOUT=60

echo "=== V-005: Banner grabbing / fingerprinting ==="
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
    echo "[-] DNS resolution failed - target may be unreachable"
    exit 1
fi

SCAN_TARGET="$RESOLVED_IP"
SAFE_OPTS="-T3 -Pn --open --max-retries 2 --max-rate 200 --host-timeout 120s"

# --- Phase 1: Service version detection (top 1000) ---
echo ""
echo "=== Phase 1: TCP service version scan (top 1000 ports) ==="
timeout $TIMEOUT nmap -sV --version-intensity 5 $SAFE_OPTS --reason "$SCAN_TARGET" 2>&1 || true

# --- Phase 2: Aggressive version probe on all open ports ---
echo ""
echo "=== Phase 2: Aggressive version probe (-sV --version-all) ==="
timeout $TIMEOUT nmap -sV --version-intensity 9 --version-all $SAFE_OPTS "$SCAN_TARGET" 2>&1 || true

# --- Phase 3: Default scripts that grab banners ---
echo ""
echo "=== Phase 3: Banner-grabbing NSE scripts ==="
timeout $TIMEOUT nmap -sC -sV --version-intensity 5 $SAFE_OPTS \
    --script=banner,http-server-header,ssl-cert,ssh2-enum-algos,ftp-syst,imap-capabilities,smtp-commands "$SCAN_TARGET" 2>&1 || true

# --- Phase 4: HTTP header fingerprinting ---
echo ""
echo "=== Phase 4: HTTP/HTTPS header fingerprinting ==="
for PORT in 80 443 8080 8443 8000 8888; do
    RESP=$(timeout 10 curl -skI -m 5 "http://${HOST}:${PORT}/" 2>/dev/null)
    if [ -n "$RESP" ]; then
        echo "--- Port $PORT HTTP Headers ---"
        echo "$RESP" | grep -iE '^server:|^x-powered-by:|^x-aspnet|^x-generator:|^x-drupal:|^x-varnish:' || echo "(no version headers found)"
        echo ""
    fi
done

# --- Phase 5: Raw banner grab via netcat on common ports ---
echo ""
echo "=== Phase 5: Raw banner grab (netcat) ==="
for PORT in 21 22 23 25 53 110 143 443 993 995 3306 5432 6379 27017; do
    BANNER=$(timeout 5 bash -c "echo '' | nc -w3 $SCAN_TARGET $PORT 2>/dev/null" | head -c 512)
    if [ -n "$BANNER" ] && [ "$BANNER" != $'\x00' ]; then
        echo "Port $PORT: $BANNER"
    fi
done 2>/dev/null || true

# --- Phase 6: SSL/TLS certificate fingerprinting ---
echo ""
echo "=== Phase 6: SSL certificate info ==="
for PORT in 443 8443 993 995; do
    CERT=$(timeout 5 openssl s_client -connect "${HOST}:${PORT}" -servername "$HOST" </dev/null 2>/dev/null | openssl x509 -noout -subject -issuer -dates -text 2>/dev/null | grep -E 'Subject:|Issuer:|Not Before|Not After|Signature Algorithm:')
    if [ -n "$CERT" ]; then
        echo "--- Port $PORT Certificate ---"
        echo "$CERT"
        echo ""
    fi
done

echo ""
echo "=== Scan Complete ==="
exit 0
