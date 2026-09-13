#!/usr/bin/env bash
set -euo pipefail

JOCKYC=$1
SOURCE_ROOT=$2
WORK=$3
rm -rf "$WORK"
mkdir -p "$WORK/registry"
cp "$SOURCE_ROOT/tests/phase5/fixtures/registry/jky_recon_pair_leaf.sh" \
   "$WORK/registry/"
cp "$SOURCE_ROOT/tests/phase5/fixtures/registry/jky_recon_pair_top.sh" \
   "$WORK/registry/"

"$JOCKYC" "$SOURCE_ROOT/tests/phase5/fixture1_direct.jky" \
  --registry "$WORK/registry" -o "$WORK/standalone" >"$WORK/build.out"
rm -rf "$WORK/registry"

(
  cd "$WORK"
  ./standalone run --output-root runtime-out >run.out
)
MANIFEST="$WORK/runtime-out/manifest.json"
test -f "$MANIFEST"
grep -q '"status":"success"' "$MANIFEST"
grep -q '"function":"jky_recon_pair_top"' "$MANIFEST"
grep -q '"outcome":"success"' "$MANIFEST"
grep -q '"target":"default"' "$MANIFEST"
grep -q '"script_sha256":"e4a3014d718d8f2d011a74a3ea220a55916d2031041ed455194d43aade04974c"' "$MANIFEST"
test "$(grep -o '"function":' "$MANIFEST" | wc -l)" -eq 1
test ! -e "$WORK/registry"
