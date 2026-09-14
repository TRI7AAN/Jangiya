#!/bin/bash
# @jocky:function jky_compliance_audit_drivers
# @jocky:domain compliance
# @jocky:description Enumerate loaded kernel modules and flag entries matching a blocklist file
# @jocky:inputs blocklist: path = ""
# @jocky:outputs findings: text
# @jocky:capability compliance.driver.audit
# @jocky:timeout_seconds 60
# @jocky:depends_on
#
# jky_compliance_audit_drivers — defensive driver-exposure audit
# (inverted E3j shard). Lists loaded kernel modules via /proc/modules
# (read-only enumeration; no driver is loaded, unloaded, touched, or
# signaled) and flags any module whose name appears in the optional
# blocklist file (one name per line, `#` comments honored).
# With no blocklist the full module list is reported unflagged.
# Output CSV rows: `module,size,flag` where flag is CLEAN, FLAGGED, or
# UNKNOWN (when /proc/modules is unreadable, e.g. inside the sandbox —
# reported honestly, never fabricated).
# Exit: 0 success | 1 usage/validation error | 2 analysis failure
set -uo pipefail

BLOCKLIST="${1:-}"

LIST="/proc/modules"
if [ ! -r "$LIST" ]; then
    echo "module,size,flag"
    echo "UNKNOWN,0,UNKNOWN"
    echo "WARNING: /proc/modules not readable here; no modules enumerated" >&2
    exit 0
fi

if [ -n "$BLOCKLIST" ] && [ ! -r "$BLOCKLIST" ]; then
    echo "ERROR: blocklist not readable: $BLOCKLIST" >&2
    exit 1
fi

echo "module,size,flag"
while IFS=' ' read -r name size _rest || [ -n "$name" ]; do
    [ -n "$name" ] || continue
    case "$name" in
        \#*) continue ;;
    esac
    flag="CLEAN"
    if [ -n "$BLOCKLIST" ]; then
        while IFS= read -r entry || [ -n "$entry" ]; do
            case "$entry" in
                ''|\#*) continue ;;
            esac
            if [ "$entry" = "$name" ]; then
                flag="FLAGGED"
                break
            fi
        done < "$BLOCKLIST"
    fi
    printf '%s,%s,%s\n' "$name" "$size" "$flag"
done < "$LIST"
exit 0
