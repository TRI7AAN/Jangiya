#!/bin/bash
# @jocky:function jky_report_render_timeline
# @jocky:domain report
# @jocky:description Render a timeline table into readable markdown text output
# @jocky:inputs timeline_csv: path, max_rows: int = 20
# @jocky:outputs report: text
# @jocky:capability report.render
# @jocky:timeout_seconds 60
# @jocky:depends_on
#
# jky_report_render_timeline — markdown rendering of an event stream.
# Input contract: CSV rows `ts,host,event` (epoch-integer ts, as
# produced by the jky_timeline_* scripts). A leading row whose first
# field is not a non-negative integer is treated as a header and
# skipped, so timeline outputs render cleanly. At most max_rows data
# rows are rendered, enforced by counter (no `head` in the sandbox).
# Output: markdown table on stdout.
# Exit: 0 success | 1 usage/validation error | 2 analysis failure
set -uo pipefail

TIMELINE="${1:?Usage: bash jky_report_render_timeline.sh <timeline_csv> [max_rows]}"
MAX_ROWS="${2:-20}"

if [ ! -r "$TIMELINE" ]; then
    echo "ERROR: timeline CSV not readable: $TIMELINE" >&2
    exit 1
fi
if [[ ! "$MAX_ROWS" =~ ^[0-9]+$ ]] || [ "$MAX_ROWS" -le 0 ]; then
    echo "ERROR: max_rows must be a positive integer, got: $MAX_ROWS" >&2
    exit 1
fi

printf '# Timeline\n\n'
printf '| ts | host | event |\n| --- | --- | --- |\n'

SHOWN=0
while IFS=, read -r ts host event || [ -n "$ts" ]; do
    if [[ ! "$ts" =~ ^[0-9]+$ ]]; then
        continue
    fi
    if [ "$SHOWN" -ge "$MAX_ROWS" ]; then
        break
    fi
    printf '| %s | %s | %s |\n' "$ts" "$host" "$event"
    SHOWN=$((SHOWN + 1))
done < "$TIMELINE"

printf '\nRendered %d row(s).\n' "$SHOWN"
exit 0
