#!/usr/bin/env bash
# @jocky:function jky_netforensics_detect_beaconing
# @jocky:domain netforensics
# @jocky:description Flag periodic callback (beaconing) shapes in a flow CSV
# @jocky:inputs flows_csv: path, min_flows: int = 10, max_cv: float = 0.20
# @jocky:outputs findings: table<indicator>
# @jocky:capability netforensics.flow.analyze
# @jocky:timeout_seconds 120
# @jocky:depends_on jky_netforensics_extract_flows
#
# jky_netforensics_detect_beaconing — inter-arrival regularity analysis.
# Usage: bash jky_netforensics_detect_beaconing.sh <flows_csv> [min_flows] [max_cv]
#   flows_csv : CSV produced by jky_netforensics_extract_flows
#   min_flows : minimum flow count per endpoint pair to judge (default 10)
#   max_cv    : maximum coefficient of variation of inter-arrival gaps to
#               call PERIODIC (default 0.20); validated numeric in (0, 1]
# Method: group by (src_ip, dst_ip, dst_port, protocol), sort flow starts,
# compute gap mean/stdev; low-variation, high-count series are
# callback-shaped. Protocol is part of the key so TCP and UDP series to
# the same port never pollute each other's statistics.
# Output CSV columns:
#   src_ip,dst_ip,dst_port,protocol,flows,span_s,mean_gap_s,stdev_gap_s,cv,bytes,verdict
# Verdict is PERIODIC or OK. Regular polling alone is not proof of
# malice — verdicts are triage leads, corroborate before asserting.
# Exit: 0 success | 1 usage/validation error | 2 analysis failure
set -euo pipefail

FLOWS="${1:?Usage: bash jky_netforensics_detect_beaconing.sh <flows_csv> [min_flows] [max_cv]}"
MIN_FLOWS="${2:-10}"
MAX_CV="${3:-0.20}"

if [ ! -r "$FLOWS" ]; then
    echo "ERROR: flows CSV not readable: $FLOWS" >&2
    exit 1
fi
if ! [[ "$MIN_FLOWS" =~ ^[0-9]+$ ]] || [ "$MIN_FLOWS" -lt 2 ]; then
    echo "ERROR: min_flows must be an integer >= 2, got: $MIN_FLOWS" >&2
    exit 1
fi
if ! [[ "$MAX_CV" =~ ^(0\.[0-9]+|1(\.0+)?)$ ]]; then
    echo "ERROR: max_cv must be numeric in (0, 1], got: $MAX_CV" >&2
    exit 1
fi
command -v python3 >/dev/null 2>&1 || { echo "ERROR: python3 not found" >&2; exit 1; }

python3 - "$FLOWS" "$MIN_FLOWS" "$MAX_CV" <<'PY' || { echo "ERROR: beacon analysis failed" >&2; exit 2; }
import sys, csv, math

path, min_flows, max_cv = sys.argv[1], int(sys.argv[2]), float(sys.argv[3])
series = {}
with open(path, newline="") as f:
    for row in csv.DictReader(f):
        src, dst, dport = row.get("src_ip", ""), row.get("dst_ip", ""), row.get("dst_port", "")
        proto = (row.get("protocol") or "").strip() or "other"
        if not src or not dst:
            continue
        try:
            start = float(row.get("start_time") or 0)
            nbytes = int(float(row.get("bytes") or 0))
        except ValueError:
            continue
        rec = series.setdefault((src, dst, dport, proto), [[], 0])
        rec[0].append(start)
        rec[1] += nbytes

writer = csv.writer(sys.stdout, lineterminator="\n")
writer.writerow(["src_ip", "dst_ip", "dst_port", "protocol", "flows", "span_s",
                 "mean_gap_s", "stdev_gap_s", "cv", "bytes", "verdict"])
for (src, dst, dport, proto) in sorted(series):
    starts, nbytes = series[(src, dst, dport, proto)]
    starts.sort()
    n = len(starts)
    if n < 2:
        writer.writerow([src, dst, dport, proto, n, "0.000000", "", "", "", nbytes, "OK"])
        continue
    gaps = [b - a for a, b in zip(starts, starts[1:])]
    span = starts[-1] - starts[0]
    mean = sum(gaps) / len(gaps)
    var = sum((g - mean) ** 2 for g in gaps) / len(gaps)
    sd = math.sqrt(var)
    cv = (sd / mean) if mean > 0 else 0.0
    verdict = "PERIODIC" if (n >= min_flows and span > 0 and cv <= max_cv) else "OK"
    writer.writerow([src, dst, dport, proto, n, f"{span:.6f}", f"{mean:.6f}",
                     f"{sd:.6f}", f"{cv:.6f}", nbytes, verdict])
PY
