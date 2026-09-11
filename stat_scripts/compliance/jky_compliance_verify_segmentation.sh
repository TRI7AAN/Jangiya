#!/bin/bash
# V-105 -- Flat network/no segmentation
# Tool: segmentation_probe
# Usage: bash V-105.sh <target> <session_output_dir>

TARGET="${1:?Usage: V-105.sh <target> <session_output_dir>}"
SESSION_DIR="${2:-}"

echo "=== V-105: Flat network/no segmentation ==="; echo "Tool: segmentation_probe"; echo "Target: $TARGET"; which segmentation_probe 2>/dev/null && echo "Tool found" || echo "Tool not found"
exit $?
