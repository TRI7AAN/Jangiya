#!/bin/bash
# @jocky:function jky_recon_audit_graphql
# @jocky:domain recon
# @jocky:description Probe a GraphQL endpoint with a single introspection query via curl
# @jocky:inputs target: string, session_dir: string = ""
# @jocky:outputs graphql_response: text
# @jocky:capability recon.graphql.audit
# @jocky:timeout_seconds 60
# @jocky:depends_on
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
