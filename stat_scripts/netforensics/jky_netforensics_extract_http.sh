#!/usr/bin/env bash
# @jocky:function jky_netforensics_extract_http
# @jocky:domain netforensics
# @jocky:description Extract HTTP request/response records from a pcap file into CSV
# @jocky:inputs pcap_path: path, bpf: string = "", out_csv: path = ""
# @jocky:outputs http_events: table<log_event>
# @jocky:capability netforensics.pcap.read
# @jocky:timeout_seconds 300
# @jocky:depends_on
#
# jky_netforensics_extract_http — tshark-backed HTTP extractor.
# Usage: bash jky_netforensics_extract_http.sh <pcap_path> [bpf_filter] [out_csv]
#   pcap_path : readable pcap/pcapng capture (opened read-only, never modified)
#   bpf_filter: optional BPF expression; applied via tcpdump pre-filter
#               (tshark -r takes display filters only), which rejects
#               invalid syntax -> exit 2, never passed unchecked
#   out_csv   : optional output path (atomic write via .tmp + rename);
#               CSV goes to stdout when omitted
# Output CSV columns:
#   timestamp,src_ip,method,uri,host,user_agent,status
# Note: cleartext HTTP only; TLS-encrypted sessions yield no rows.
# Exit: 0 success | 1 usage/dependency error | 2 extraction failure
set -euo pipefail

PCAP="${1:?Usage: bash jky_netforensics_extract_http.sh <pcap_path> [bpf_filter] [out_csv]}"
BPF="${2:-}"
OUT="${3:-}"

if [ ! -r "$PCAP" ]; then
    echo "ERROR: pcap not readable: $PCAP" >&2
    exit 1
fi
command -v tshark >/dev/null 2>&1 || { echo "ERROR: tshark not found" >&2; exit 1; }
command -v python3 >/dev/null 2>&1 || { echo "ERROR: python3 not found" >&2; exit 1; }
if [ -n "$BPF" ]; then
    # BPF applies at capture-filter level via tcpdump (tshark -r accepts
    # display filters only); tcpdump itself rejects invalid syntax.
    command -v tcpdump >/dev/null 2>&1 || { echo "ERROR: tcpdump not found (required for bpf filtering)" >&2; exit 1; }
fi

FIELDS=(-T fields -E header=y -E separator=, -E occurrence=f -E quote=d
    -e frame.time_epoch -e ip.src -e ipv6.src
    -e http.request.method -e http.request.uri -e http.host
    -e http.user_agent -e http.response.code -e http.response.phrase)

run_extraction() {
    if [ -n "$BPF" ]; then
        tcpdump -r "$PCAP" -w - "$BPF" 2>/dev/null \
        | tshark -r - -Y http "${FIELDS[@]}" 2>/dev/null
    else
        tshark -r "$PCAP" -Y http "${FIELDS[@]}" 2>/dev/null
    fi | python3 /dev/fd/3 3<<'PY'
import sys, csv

reader = csv.DictReader(sys.stdin)
writer = csv.writer(sys.stdout, lineterminator="\n")
writer.writerow(["timestamp", "src_ip", "method", "uri", "host",
                 "user_agent", "status"])
for row in reader:
    try:
        ts = float(row["frame.time_epoch"])
    except (TypeError, ValueError):
        continue
    method = (row.get("http.request.method") or "").strip()
    uri = (row.get("http.request.uri") or "").strip()
    host = (row.get("http.host") or "").strip()
    code = (row.get("http.response.code") or "").strip()
    if not method and not uri and not host and not code:
        continue
    src = row.get("ip.src") or row.get("ipv6.src") or ""
    writer.writerow([f"{ts:.6f}", src, method, uri, host,
                     (row.get("http.user_agent") or "").strip(), code])
PY
}

if [ -n "$OUT" ]; then
    if ! run_extraction > "$OUT.tmp"; then
        echo "ERROR: HTTP extraction failed for $PCAP" >&2
        rm -f "$OUT.tmp"
        exit 2
    fi
    mv "$OUT.tmp" "$OUT"
else
    run_extraction || { echo "ERROR: HTTP extraction failed for $PCAP" >&2; exit 2; }
fi
