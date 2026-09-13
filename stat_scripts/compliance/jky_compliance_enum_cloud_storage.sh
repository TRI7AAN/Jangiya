#!/bin/bash
# @jocky:function jky_compliance_enum_cloud_storage
# @jocky:domain compliance
# @jocky:description Enumerate public cloud storage for a keyword via cloud_enum
# @jocky:inputs target: string, session_dir: string = ""
# @jocky:outputs report: text
# @jocky:capability compliance.cloud.read
# @jocky:timeout_seconds 300
# @jocky:depends_on
# V-106 -- Public cloud storage exposure
# Tool: cloud_enum
# Usage: bash V-106.sh <target> <session_output_dir>

TARGET="${1:?Usage: V-106.sh <target> <session_output_dir>}"
SESSION_DIR="${2:-}"

cloud_enum -k "$TARGET" 2>&1 | head -30
exit $?
