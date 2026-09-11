#!/usr/bin/env bash
# @jocky:function jky_netforensics_check_dns_anomalies
# @jocky:domain netforensics
# @jocky:description Flag DNS-tunneling and DGA shapes in a DNS CSV
# @jocky:inputs dns_csv: path, min_txt: int = 5, min_nxdomain: int = 20
# @jocky:outputs findings: table<indicator>
# @jocky:capability netforensics.dns.analyze
# @jocky:timeout_seconds 120
# @jocky:depends_on jky_netforensics_extract_dns
#
# jky_netforensics_check_dns_anomalies — per-source DNS shape analysis.
# Usage: bash jky_netforensics_check_dns_anomalies.sh <dns_csv> [min_txt] [min_nxdomain]
#   dns_csv     : CSV produced by jky_netforensics_extract_dns
#   min_txt     : TXT-query count threshold with ratio check (default 5)
#   min_nxdomain: NXDOMAIN count threshold for DGA suspicion (default 20)
# Signals: TXT/NULL-heavy sources (tunneling shape), NXDOMAIN bursts
# (DGA shape — rule out search-suffix misconfiguration first), longest
# queried name per source. Verdicts are triage leads, not conclusions.
# Output CSV columns:
#   src_ip,queries,distinct_names,txt_count,txt_ratio,nxdomain_count,max_name_len,verdict
# Verdict is TUNNEL_SUSPECT, DGA_SUSPECT, BOTH, or OK.
# Exit: 0 success | 1 usage/validation error | 2 analysis failure
set -euo pipefail

DNSCSV="${1:?Usage: bash jky_netforensics_check_dns_anomalies.sh <dns_csv> [min_txt] [min_nxdomain]}"
MIN_TXT="${2:-5}"
MIN_NX="${3:-20}"

if [ ! -r "$DNSCSV" ]; then
    echo "ERROR: DNS CSV not readable: $DNSCSV" >&2
    exit 1
fi
if ! [[ "$MIN_TXT" =~ ^[0-9]+$ ]] || [ "$MIN_TXT" -le 0 ]; then
    echo "ERROR: min_txt must be a positive integer, got: $MIN_TXT" >&2
    exit 1
fi
if ! [[ "$MIN_NX" =~ ^[0-9]+$ ]] || [ "$MIN_NX" -le 0 ]; then
    echo "ERROR: min_nxdomain must be a positive integer, got: $MIN_NX" >&2
    exit 1
fi
command -v python3 >/dev/null 2>&1 || { echo "ERROR: python3 not found" >&2; exit 1; }

python3 - "$DNSCSV" "$MIN_TXT" "$MIN_NX" <<'PY' || { echo "ERROR: DNS analysis failed" >&2; exit 2; }
import sys, csv

path, min_txt, min_nx = sys.argv[1], int(sys.argv[2]), int(sys.argv[3])
sources = {}
with open(path, newline="") as f:
    for row in csv.DictReader(f):
        src = row.get("src_ip", "")
        name = (row.get("query_name") or "").strip().rstrip(".")
        if not src or not name:
            continue
        qtype = (row.get("query_type") or "").strip().upper()
        rcode = (row.get("rcode") or "").strip().upper()
        rec = sources.setdefault(src, {"n": 0, "names": set(), "txt": 0,
                                       "nx": 0, "maxlen": 0})
        rec["n"] += 1
        rec["names"].add(name.lower())
        if qtype in ("TXT", "NULL"):
            rec["txt"] += 1
        if rcode == "NXDOMAIN":
            rec["nx"] += 1
        rec["maxlen"] = max(rec["maxlen"], len(name))

writer = csv.writer(sys.stdout, lineterminator="\n")
writer.writerow(["src_ip", "queries", "distinct_names", "txt_count",
                 "txt_ratio", "nxdomain_count", "max_name_len", "verdict"])
for src in sorted(sources):
    rec = sources[src]
    ratio = (rec["txt"] / rec["n"]) if rec["n"] else 0.0
    tunnel = rec["txt"] >= min_txt and ratio > 0.10
    dga = rec["nx"] >= min_nx
    if tunnel and dga:
        verdict = "BOTH"
    elif tunnel:
        verdict = "TUNNEL_SUSPECT"
    elif dga:
        verdict = "DGA_SUSPECT"
    else:
        verdict = "OK"
    writer.writerow([src, rec["n"], len(rec["names"]), rec["txt"],
                     f"{ratio:.4f}", rec["nx"], rec["maxlen"], verdict])
PY
