#!/bin/bash
# @jocky:function jky_timeline_correlate_events
# @jocky:domain timeline
# @jocky:description Correlate two timestamped event streams within a time window
# @jocky:inputs left_csv: path, right_csv: path, window: string = "5m"
# @jocky:outputs timeline: text
# @jocky:capability timeline.correlate
# @jocky:timeout_seconds 120
# @jocky:depends_on
#
# jky_timeline_correlate_events — pairwise temporal join of two event
# streams. Input contract (both files): CSV rows `ts,host,event` where
# ts is an epoch-integer timestamp. Rows whose first field is not a
# non-negative integer (e.g. a header row) are skipped, never matched.
# Window contract: plain seconds ("300") or suffixed s/m/h/d
# ("5m" = 300). A left row pairs with every right row whose |delta| is
# within the window. Output CSV columns:
#   ts,left_host,left_event,right_host,right_event
# Pure-bash nested scan (O(n*m)): the sandbox provides bash+sleep only,
# so no join/awk; starter-scale inputs only (see wiki/23).
# Exit: 0 success | 1 usage/validation error | 2 analysis failure
set -uo pipefail

LEFT="${1:?Usage: bash jky_timeline_correlate_events.sh <left_csv> <right_csv> [window]}"
RIGHT="${2:?Usage: bash jky_timeline_correlate_events.sh <left_csv> <right_csv> [window]}"
WINDOW="${3:-5m}"

if [ ! -r "$LEFT" ]; then
    echo "ERROR: left CSV not readable: $LEFT" >&2
    exit 1
fi
if [ ! -r "$RIGHT" ]; then
    echo "ERROR: right CSV not readable: $RIGHT" >&2
    exit 1
fi

# Window suffix parsing without external tools: strip a trailing
# s/m/h/d letter, multiply by its unit. Anything else must be a plain
# non-negative integer number of seconds.
WINDOW_SECS="$WINDOW"
MULT=1
case "$WINDOW" in
    *s) WINDOW_SECS="${WINDOW%s}"; MULT=1 ;;
    *m) WINDOW_SECS="${WINDOW%m}"; MULT=60 ;;
    *h) WINDOW_SECS="${WINDOW%h}"; MULT=3600 ;;
    *d) WINDOW_SECS="${WINDOW%d}"; MULT=86400 ;;
esac
if [[ ! "$WINDOW_SECS" =~ ^[0-9]+$ ]]; then
    echo "ERROR: window must be seconds or Ns/Nm/Nh/Nd, got: $WINDOW" >&2
    exit 1
fi
WINDOW_SECS=$((WINDOW_SECS * MULT))

is_epoch() {
    # Non-negative integer check; doubles as the header-row filter.
    [[ "$1" =~ ^[0-9]+$ ]]
}

LEFT_TS=()
LEFT_HOST=()
LEFT_EVENT=()
while IFS=, read -r ts host event || [ -n "$ts" ]; do
    if is_epoch "$ts"; then
        LEFT_TS+=("$ts")
        LEFT_HOST+=("$host")
        LEFT_EVENT+=("$event")
    fi
done < "$LEFT"

RIGHT_TS=()
RIGHT_HOST=()
RIGHT_EVENT=()
while IFS=, read -r ts host event || [ -n "$ts" ]; do
    if is_epoch "$ts"; then
        RIGHT_TS+=("$ts")
        RIGHT_HOST+=("$host")
        RIGHT_EVENT+=("$event")
    fi
done < "$RIGHT"

MATCHES=0
i=0
while [ "$i" -lt "${#LEFT_TS[@]}" ]; do
    j=0
    while [ "$j" -lt "${#RIGHT_TS[@]}" ]; do
        delta=$((LEFT_TS[i] - RIGHT_TS[j]))
        if [ "$delta" -lt 0 ]; then
            delta=$((-delta))
        fi
        if [ "$delta" -le "$WINDOW_SECS" ]; then
            printf '%s,%s,%s,%s,%s\n' "${LEFT_TS[i]}" "${LEFT_HOST[i]}" \
                "${LEFT_EVENT[i]}" "${RIGHT_HOST[j]}" "${RIGHT_EVENT[j]}"
            MATCHES=$((MATCHES + 1))
        fi
        j=$((j + 1))
    done
    i=$((i + 1))
done

if [ "$MATCHES" -eq 0 ]; then
    echo "WARNING: no correlated pairs within window $WINDOW" >&2
fi
exit 0
