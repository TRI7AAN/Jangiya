#!/bin/bash
# V-110 -- CSPM flaw
# Tool: prowler
# Usage: bash V-110.sh <target> <session_output_dir>

TARGET="${1:?Usage: V-110.sh <target> <session_output_dir>}"
SESSION_DIR="${2:-}"

prowler aws --compliance cis_2.0 2>&1 | head -30 || echo "not configured"
exit $?
