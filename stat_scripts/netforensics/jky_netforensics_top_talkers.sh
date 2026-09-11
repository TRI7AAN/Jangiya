#!/usr/bin/env bash
# @jocky:function jky_netforensics_top_talkers
# @jocky:domain netforensics
# @jocky:description Rank endpoint pairs by transferred bytes from a flow CSV
# @jocky:inputs flows_csv: path, top_n: int = 10
# @jocky:outputs ranking: table<indicator>
# @jocky:capability netforensics.flow.analyze
# @jocky:timeout_seconds 120
# @jocky:depends_on jky_netforensics_extract_flows
#
# jky_netforensics_top_talkers — top-N endpoints by bytes.
# Usage: bash jky_netforensics_top_talkers.sh <flows_csv> [top_n]
#   flows_csv : CSV produced by jky_netforensics_extract_flows
#   top_n     : positive integer, rows to emit (default 10)
# Output CSV columns: rank,src_ip,dst_ip,flows,packets,bytes
# Exit: 0 success | 1 usage/validation error | 2 analysis failure
set -euo pipefail

FLOWS="${1:?Usage: bash jky_netforensics_top_talkers.sh <flows_csv> [top_n]}"
TOP_N="${2:-10}"

if [ ! -r "$FLOWS" ]; then
    echo "ERROR: flows CSV not readable: $FLOWS" >&2
    exit 1
fi
if ! [[ "$TOP_N" =~ ^[0-9]+$ ]] || [ "$TOP_N" -le 0 ]; then
    echo "ERROR: top_n must be a positive integer, got: $TOP_N" >&2
    exit 1
fi
command -v python3 >/dev/null 2>&1 || { echo "ERROR: python3 not found" >&2; exit 1; }

python3 - "$FLOWS" "$TOP_N" <<'PY' || { echo "ERROR: ranking failed" >&2; exit 2; }
import sys, csv

path, top_n = sys.argv[1], int(sys.argv[2])
pairs = {}
with open(path, newline="") as f:
    for row in csv.DictReader(f):
        src, dst = row.get("src_ip", ""), row.get("dst_ip", "")
        if not src or not dst:
            continue
        try:
            packets = int(float(row.get("packets") or 0))
            nbytes = int(float(row.get("bytes") or 0))
        except ValueError:
            continue
        rec = pairs.setdefault((src, dst), [0, 0, 0])
        rec[0] += 1
        rec[1] += packets
        rec[2] += nbytes

ranked = sorted(pairs.items(), key=lambda kv: kv[1][2], reverse=True)[:top_n]
writer = csv.writer(sys.stdout, lineterminator="\n")
writer.writerow(["rank", "src_ip", "dst_ip", "flows", "packets", "bytes"])
for rank, ((src, dst), (flows, packets, nbytes)) in enumerate(ranked, 1):
    writer.writerow([rank, src, dst, flows, packets, nbytes])
PY
