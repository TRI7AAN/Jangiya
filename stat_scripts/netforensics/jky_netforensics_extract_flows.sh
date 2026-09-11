#!/usr/bin/env bash
# @jocky:function jky_netforensics_extract_flows
# @jocky:domain netforensics
# @jocky:description Extract L3/L4 flow records from a pcap file into CSV
# @jocky:inputs pcap_path: path, bpf: string = "", out_csv: path = ""
# @jocky:outputs flows: table<flow>
# @jocky:capability netforensics.pcap.read
# @jocky:timeout_seconds 300
# @jocky:depends_on
#
# jky_netforensics_extract_flows — tshark-backed flow extractor.
# Usage: bash jky_netforensics_extract_flows.sh <pcap_path> [bpf_filter] [out_csv]
#   pcap_path : readable pcap/pcapng capture (opened read-only, never modified)
#   bpf_filter: optional BPF expression (e.g. "tcp port 443"); applied via
#               tcpdump pre-filter (tshark -r takes display filters only),
#               which rejects invalid syntax -> exit 2, never passed unchecked
#   out_csv   : optional output path (atomic write via .tmp + rename);
#               CSV goes to stdout when omitted
# Output CSV columns:
#   start_time,end_time,src_ip,src_port,dst_ip,dst_port,protocol,packets,bytes
# Exit: 0 success | 1 usage/dependency error | 2 extraction failure
set -euo pipefail

PCAP="${1:?Usage: bash jky_netforensics_extract_flows.sh <pcap_path> [bpf_filter] [out_csv]}"
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

run_extraction() {
    if [ -n "$BPF" ]; then
        tcpdump -r "$PCAP" -w - "$BPF" 2>/dev/null \
        | tshark -r - \
            -T fields -E header=y -E separator=, -E occurrence=f -E quote=d \
            -e frame.time_epoch -e ip.src -e ipv6.src -e ip.dst -e ipv6.dst \
            -e tcp.srcport -e tcp.dstport -e udp.srcport -e udp.dstport \
            -e ip.proto -e frame.len 2>/dev/null \
        | python3 /dev/fd/3 3<<'PY'
import sys, csv

PROTO = {"1": "icmp", "6": "tcp", "17": "udp"}

reader = csv.DictReader(sys.stdin)
flows = {}
order = []
for row in reader:
    try:
        ts = float(row["frame.time_epoch"])
    except (TypeError, ValueError):
        continue
    src = row.get("ip.src") or row.get("ipv6.src") or ""
    dst = row.get("ip.dst") or row.get("ipv6.dst") or ""
    if not src or not dst:
        continue
    if row.get("tcp.srcport"):
        sport, dport = row["tcp.srcport"], row.get("tcp.dstport") or ""
    elif row.get("udp.srcport"):
        sport, dport = row["udp.srcport"], row.get("udp.dstport") or ""
    else:
        sport, dport = "0", "0"
    try:
        length = int(float(row.get("frame.len") or 0))
    except ValueError:
        length = 0
    proto = PROTO.get(row.get("ip.proto") or "", "other")
    key = (src, sport, dst, dport, proto)
    if key not in flows:
        flows[key] = [ts, ts, 0, 0]
        order.append(key)
    rec = flows[key]
    rec[0] = min(rec[0], ts)
    rec[1] = max(rec[1], ts)
    rec[2] += 1
    rec[3] += length

writer = csv.writer(sys.stdout, lineterminator="\n")
writer.writerow(["start_time", "end_time", "src_ip", "src_port",
                 "dst_ip", "dst_port", "protocol", "packets", "bytes"])
for key in order:
    s = flows[key]
    writer.writerow([f"{s[0]:.6f}", f"{s[1]:.6f}", key[0], key[1],
                     key[2], key[3], key[4], s[2], s[3]])
PY
    else
        tshark -r "$PCAP" \
            -T fields -E header=y -E separator=, -E occurrence=f -E quote=d \
            -e frame.time_epoch -e ip.src -e ipv6.src -e ip.dst -e ipv6.dst \
            -e tcp.srcport -e tcp.dstport -e udp.srcport -e udp.dstport \
            -e ip.proto -e frame.len 2>/dev/null \
        | python3 /dev/fd/3 3<<'PY'
import sys, csv

PROTO = {"1": "icmp", "6": "tcp", "17": "udp"}

reader = csv.DictReader(sys.stdin)
flows = {}
order = []
for row in reader:
    try:
        ts = float(row["frame.time_epoch"])
    except (TypeError, ValueError):
        continue
    src = row.get("ip.src") or row.get("ipv6.src") or ""
    dst = row.get("ip.dst") or row.get("ipv6.dst") or ""
    if not src or not dst:
        continue
    if row.get("tcp.srcport"):
        sport, dport = row["tcp.srcport"], row.get("tcp.dstport") or ""
    elif row.get("udp.srcport"):
        sport, dport = row["udp.srcport"], row.get("udp.dstport") or ""
    else:
        sport, dport = "0", "0"
    try:
        length = int(float(row.get("frame.len") or 0))
    except ValueError:
        length = 0
    proto = PROTO.get(row.get("ip.proto") or "", "other")
    key = (src, sport, dst, dport, proto)
    if key not in flows:
        flows[key] = [ts, ts, 0, 0]
        order.append(key)
    rec = flows[key]
    rec[0] = min(rec[0], ts)
    rec[1] = max(rec[1], ts)
    rec[2] += 1
    rec[3] += length

writer = csv.writer(sys.stdout, lineterminator="\n")
writer.writerow(["start_time", "end_time", "src_ip", "src_port",
                 "dst_ip", "dst_port", "protocol", "packets", "bytes"])
for key in order:
    s = flows[key]
    writer.writerow([f"{s[0]:.6f}", f"{s[1]:.6f}", key[0], key[1],
                     key[2], key[3], key[4], s[2], s[3]])
PY
    fi
}

if [ -n "$OUT" ]; then
    if ! run_extraction > "$OUT.tmp"; then
        echo "ERROR: flow extraction failed for $PCAP" >&2
        rm -f "$OUT.tmp"
        exit 2
    fi
    mv "$OUT.tmp" "$OUT"
else
    run_extraction || { echo "ERROR: flow extraction failed for $PCAP" >&2; exit 2; }
fi
