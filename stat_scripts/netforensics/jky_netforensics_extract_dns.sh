#!/usr/bin/env bash
# @jocky:function jky_netforensics_extract_dns
# @jocky:domain netforensics
# @jocky:description Extract DNS query/response records from a pcap file into CSV
# @jocky:inputs pcap_path: path, bpf: string = "", out_csv: path = ""
# @jocky:outputs dns_events: table<dns_event>
# @jocky:capability netforensics.pcap.read
# @jocky:timeout_seconds 300
# @jocky:depends_on
#
# jky_netforensics_extract_dns — tshark-backed DNS extractor.
# Usage: bash jky_netforensics_extract_dns.sh <pcap_path> [bpf_filter] [out_csv]
#   pcap_path : readable pcap/pcapng capture (opened read-only, never modified)
#   bpf_filter: optional BPF expression; applied via tcpdump pre-filter
#               (tshark -r takes display filters only), which rejects
#               invalid syntax -> exit 2, never passed unchecked
#   out_csv   : optional output path (atomic write via .tmp + rename);
#               CSV goes to stdout when omitted
# Output CSV columns:
#   timestamp,src_ip,query_name,query_type,is_response,rcode
# Note: observes cleartext DNS only; DNS-over-HTTPS/TLS traffic is not
# visible to passive collection (see wiki/07-research.md). Absence of DNS
# rows must never be presented as proof of no name resolution.
# Exit: 0 success | 1 usage/dependency error | 2 extraction failure
set -euo pipefail

PCAP="${1:?Usage: bash jky_netforensics_extract_dns.sh <pcap_path> [bpf_filter] [out_csv]}"
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
    -e dns.qry.name -e dns.qry.type -e dns.flags.response -e dns.flags.rcode)

run_extraction() {
    if [ -n "$BPF" ]; then
        tcpdump -r "$PCAP" -w - "$BPF" 2>/dev/null \
        | tshark -r - -Y dns "${FIELDS[@]}" 2>/dev/null
    else
        tshark -r "$PCAP" -Y dns "${FIELDS[@]}" 2>/dev/null
    fi | python3 /dev/fd/3 3<<'PY'
import sys, csv

QTYPES = {"1": "A", "2": "NS", "5": "CNAME", "6": "SOA", "12": "PTR",
          "15": "MX", "16": "TXT", "28": "AAAA", "33": "SRV",
          "255": "ANY", "257": "CAA"}
RCODES = {"0": "NOERROR", "1": "FORMERR", "2": "SERVFAIL", "3": "NXDOMAIN",
          "4": "NOTIMP", "5": "REFUSED"}

def qtype_name(raw):
    raw = (raw or "").strip()
    if raw in QTYPES:
        return QTYPES[raw]
    return "TYPE" + raw if raw else ""

def rcode_name(raw):
    raw = (raw or "").strip()
    if raw in RCODES:
        return RCODES[raw]
    return "RCODE" + raw if raw else ""

reader = csv.DictReader(sys.stdin)
writer = csv.writer(sys.stdout, lineterminator="\n")
writer.writerow(["timestamp", "src_ip", "query_name", "query_type",
                 "is_response", "rcode"])
for row in reader:
    try:
        ts = float(row["frame.time_epoch"])
    except (TypeError, ValueError):
        continue
    name = (row.get("dns.qry.name") or "").strip()
    if not name:
        continue
    src = row.get("ip.src") or row.get("ipv6.src") or ""
    resp = (row.get("dns.flags.response") or "").strip().lower()
    writer.writerow([f"{ts:.6f}", src, name, qtype_name(row.get("dns.qry.type")),
                     "true" if resp in ("1", "true") else "false",
                     rcode_name(row.get("dns.flags.rcode"))])
PY
}

if [ -n "$OUT" ]; then
    if ! run_extraction > "$OUT.tmp"; then
        echo "ERROR: DNS extraction failed for $PCAP" >&2
        rm -f "$OUT.tmp"
        exit 2
    fi
    mv "$OUT.tmp" "$OUT"
else
    run_extraction || { echo "ERROR: DNS extraction failed for $PCAP" >&2; exit 2; }
fi
