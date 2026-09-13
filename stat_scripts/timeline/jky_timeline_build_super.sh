#!/bin/bash
# @jocky:function jky_timeline_build_super
# @jocky:domain timeline
# @jocky:description Merge two timestamped event streams into one time-ordered supertimeline
# @jocky:inputs a_csv: path, b_csv: path
# @jocky:outputs supertimeline: text
# @jocky:capability timeline.build
# @jocky:timeout_seconds 120
# @jocky:depends_on
#
# jky_timeline_build_super — basic supertimeline: union of two event
# streams sorted ascending by timestamp. Input contract (both files):
# CSV rows `ts,host,event` with epoch-integer ts. Rows whose first
# field is not a non-negative integer (e.g. a header row) are skipped.
# Output CSV columns: ts,source,host,event (source is "a" or "b").
# Exactly two inputs: merge longer chains by repeated application.
# Sorting is a manual insertion sort in pure bash: the sandbox provides
# bash+sleep only, so `sort -t, -k1,1n` is unavailable (see wiki/23).
# O(n^2) — starter-scale inputs only.
# Exit: 0 success | 1 usage/validation error | 2 analysis failure
set -uo pipefail

A_CSV="${1:?Usage: bash jky_timeline_build_super.sh <a_csv> <b_csv>}"
B_CSV="${2:?Usage: bash jky_timeline_build_super.sh <a_csv> <b_csv>}"

if [ ! -r "$A_CSV" ]; then
    echo "ERROR: input CSV not readable: $A_CSV" >&2
    exit 1
fi
if [ ! -r "$B_CSV" ]; then
    echo "ERROR: input CSV not readable: $B_CSV" >&2
    exit 1
fi

TS=()
SRC=()
HOST=()
EVENT=()

ingest() {
    # $1 = file path, $2 = source tag ("a"/"b").
    local file="$1" tag="$2" ts host event
    while IFS=, read -r ts host event || [ -n "$ts" ]; do
        if [[ "$ts" =~ ^[0-9]+$ ]]; then
            TS+=("$ts")
            SRC+=("$tag")
            HOST+=("$host")
            EVENT+=("$event")
        fi
    done < "$file"
}

ingest "$A_CSV" "a"
ingest "$B_CSV" "b"

TOTAL="${#TS[@]}"
if [ "$TOTAL" -eq 0 ]; then
    echo "WARNING: no timestamped rows in either input" >&2
    exit 0
fi

# Insertion sort by ts over parallel arrays (no `sort` in sandbox).
i=1
while [ "$i" -lt "$TOTAL" ]; do
    kts="${TS[i]}"
    ksrc="${SRC[i]}"
    khost="${HOST[i]}"
    kevent="${EVENT[i]}"
    j=$((i - 1))
    while [ "$j" -ge 0 ] && [ "${TS[j]}" -gt "$kts" ]; do
        TS[$((j + 1))]="${TS[j]}"
        SRC[$((j + 1))]="${SRC[j]}"
        HOST[$((j + 1))]="${HOST[j]}"
        EVENT[$((j + 1))]="${EVENT[j]}"
        j=$((j - 1))
    done
    TS[$((j + 1))]="$kts"
    SRC[$((j + 1))]="$ksrc"
    HOST[$((j + 1))]="$khost"
    EVENT[$((j + 1))]="$kevent"
    i=$((i + 1))
done

i=0
while [ "$i" -lt "$TOTAL" ]; do
    printf '%s,%s,%s,%s\n' "${TS[i]}" "${SRC[i]}" "${HOST[i]}" "${EVENT[i]}"
    i=$((i + 1))
done
exit 0
