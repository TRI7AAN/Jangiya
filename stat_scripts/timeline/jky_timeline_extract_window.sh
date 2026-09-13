#!/bin/bash
# @jocky:function jky_timeline_extract_window
# @jocky:domain timeline
# @jocky:description Filter timestamped events to an inclusive epoch time range
# @jocky:inputs events_csv: path, start_ts: int, end_ts: int
# @jocky:outputs window: text
# @jocky:capability timeline.extract
# @jocky:timeout_seconds 60
# @jocky:depends_on
#
# jky_timeline_extract_window — range filter over an event stream.
# Input contract: CSV rows `ts,host,event` with epoch-integer ts;
# timestamps are compared as integers, so callers pass epoch bounds
# (ISO-8601 strings would need `date -d` parsing, unavailable in the
# sandbox — see wiki/23). Rows whose first field is not a
# non-negative integer (e.g. a header row) are skipped. Output rows
# are the input rows with start_ts <= ts <= end_ts, in input order.
# Exit: 0 success | 1 usage/validation error | 2 analysis failure
set -uo pipefail

EVENTS="${1:?Usage: bash jky_timeline_extract_window.sh <events_csv> <start_ts> <end_ts>}"
START_TS="${2:?Usage: bash jky_timeline_extract_window.sh <events_csv> <start_ts> <end_ts>}"
END_TS="${3:?Usage: bash jky_timeline_extract_window.sh <events_csv> <start_ts> <end_ts>}"

if [ ! -r "$EVENTS" ]; then
    echo "ERROR: events CSV not readable: $EVENTS" >&2
    exit 1
fi
if [[ ! "$START_TS" =~ ^[0-9]+$ ]]; then
    echo "ERROR: start_ts must be an epoch integer, got: $START_TS" >&2
    exit 1
fi
if [[ ! "$END_TS" =~ ^[0-9]+$ ]]; then
    echo "ERROR: end_ts must be an epoch integer, got: $END_TS" >&2
    exit 1
fi
if [ "$START_TS" -gt "$END_TS" ]; then
    echo "ERROR: start_ts ($START_TS) is after end_ts ($END_TS)" >&2
    exit 1
fi

KEPT=0
while IFS=, read -r ts host event || [ -n "$ts" ]; do
    if [[ "$ts" =~ ^[0-9]+$ ]] && [ "$ts" -ge "$START_TS" ] && [ "$ts" -le "$END_TS" ]; then
        printf '%s,%s,%s\n' "$ts" "$host" "$event"
        KEPT=$((KEPT + 1))
    fi
done < "$EVENTS"

if [ "$KEPT" -eq 0 ]; then
    echo "WARNING: no events in [$START_TS, $END_TS]" >&2
fi
exit 0
