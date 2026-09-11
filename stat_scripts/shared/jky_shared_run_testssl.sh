#!/usr/bin/env bash
# @jocky:function jky_compliance_run_testssl
# @jocky:domain compliance
# @jocky:description Run the third-party testssl.sh TLS scanner via the shared helper
# @jocky:inputs target: string, session_dir: string = ""
# @jocky:outputs report: text
# @jocky:capability compliance.tls.scan
# @jocky:timeout_seconds 300
# @jocky:depends_on
#
# jky_compliance_run_testssl — thin registry wrapper. Delegates to
# stat_scripts/shared/testssl.sh, which locates and execs the external
# testssl.sh scanner binary. The third-party tool itself is intentionally
# unmodified; registry compliance lives in this wrapper's header.
# (Capability assigned per <domain>.<area>.<verb> pattern; confirm or amend.
#  File/function name split is deliberate: see session report.)
# Usage: bash jky_shared_run_testssl.sh <target> [session_dir]
# Exit: 0 success | 1 usage error | passes through helper exit code otherwise
set -euo pipefail

TARGET="${1:?Usage: bash jky_shared_run_testssl.sh <target> [session_dir]}"
SESSION_DIR="${2:-}"

SELF_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
exec "$SELF_DIR/testssl.sh" "$TARGET" "$SESSION_DIR"
