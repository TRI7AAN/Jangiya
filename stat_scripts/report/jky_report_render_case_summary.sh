#!/bin/bash
# @jocky:function jky_report_render_case_summary
# @jocky:domain report
# @jocky:description Render key findings into a readable markdown case summary
# @jocky:inputs findings_csv: path, case_id: string = ""
# @jocky:outputs summary: text
# @jocky:capability report.render
# @jocky:timeout_seconds 60
# @jocky:depends_on
#
# jky_report_render_case_summary — markdown case summary from a findings
# table. Input contract: CSV rows `finding,severity,source` (severity
# free text; recognized levels high/medium/low/info counted separately,
# anything else counted as other). A leading row whose first field is
# the literal string "finding" is treated as a header and skipped.
# Severity tallies use loop counters (no `wc` in the sandbox) and the
# detail section is capped at 20 rows by counter (no `head`).
# Output: markdown document on stdout (see wiki/23 for the template).
# Exit: 0 success | 1 usage/validation error | 2 analysis failure
set -uo pipefail

FINDINGS="${1:?Usage: bash jky_report_render_case_summary.sh <findings_csv> [case_id]}"
CASE_ID="${2:-}"

if [ ! -r "$FINDINGS" ]; then
    echo "ERROR: findings CSV not readable: $FINDINGS" >&2
    exit 1
fi
if [ -z "$CASE_ID" ]; then
    CASE_ID="uncategorized-case"
fi

HIGH=0
MEDIUM=0
LOW=0
INFO=0
OTHER=0
TOTAL=0
DETAIL=""
SHOWN=0
while IFS=, read -r finding severity source || [ -n "$finding" ]; do
    if [ "$finding" = "finding" ] && [ "$TOTAL" -eq 0 ]; then
        continue
    fi
    TOTAL=$((TOTAL + 1))
    case "$severity" in
        high|HIGH|High) HIGH=$((HIGH + 1)) ;;
        medium|MEDIUM|Medium) MEDIUM=$((MEDIUM + 1)) ;;
        low|LOW|Low) LOW=$((LOW + 1)) ;;
        info|INFO|Info) INFO=$((INFO + 1)) ;;
        *) OTHER=$((OTHER + 1)) ;;
    esac
    if [ "$SHOWN" -lt 20 ]; then
        DETAIL="${DETAIL}| ${finding} | ${severity} | ${source} |"$'\n'
        SHOWN=$((SHOWN + 1))
    fi
done < "$FINDINGS"

printf '# Case summary: %s\n\n' "$CASE_ID"
printf 'Findings total: %d (high %d, medium %d, low %d, info %d, other %d)\n\n' \
    "$TOTAL" "$HIGH" "$MEDIUM" "$LOW" "$INFO" "$OTHER"
printf '## Findings (first %d)\n\n' "$SHOWN"
printf '| finding | severity | source |\n| --- | --- | --- |\n'
printf '%s' "$DETAIL"
exit 0
