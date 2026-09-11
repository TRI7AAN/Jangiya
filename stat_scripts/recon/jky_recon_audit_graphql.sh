#!/bin/bash
# V-055 -- Insecure GraphQL
# Tool: graphql-cop
# Usage: bash V-055.sh <target> <session_output_dir>

TARGET="${1:?Usage: V-055.sh <target> <session_output_dir>}"
SESSION_DIR="${2:-}"

if [[ ! "$TARGET" =~ ^https?:// ]]; then TARGET="http://$TARGET"; fi
curl -s -X POST "$TARGET/graphql" -H "Content-Type: application/json" -d @- 2>&1 <<EOF
{"query":"{ __schema { types { name } } }"}
EOF
exit $?
