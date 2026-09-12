#!/usr/bin/env bash
set -euo pipefail

JOCKYC=$1
SOURCE_ROOT=$2
WORK=$3
rm -rf "$WORK"
mkdir -p "$WORK/registry"
cp "$SOURCE_ROOT/tests/phase5/fixtures/registry/jky_recon_pair_leaf.sh" "$WORK/registry/"
cp "$SOURCE_ROOT/tests/phase5/fixtures/registry/jky_recon_pair_top.sh" "$WORK/registry/"

"$JOCKYC" "$SOURCE_ROOT/tests/phase5/fixture1_direct.jky" \
  --registry "$WORK/registry" -o "$WORK/standalone" >"$WORK/build.out"
"$JOCKYC" "$SOURCE_ROOT/tests/phase5/fixture1_direct.jky" \
  --registry "$WORK/registry" --list-used >"$WORK/used.out"
rm -rf "$WORK/registry"
"$WORK/standalone" --list-embedded >"$WORK/embedded.out"

sed -n 's/^\(jky_[^ ]*\) .*/\1/p' "$WORK/used.out" >"$WORK/used.names"
sed -n 's/^EMBEDDED \([^ ]*\) .*/\1/p' "$WORK/embedded.out" >"$WORK/embedded.names"
printf '%s\n' jky_recon_pair_leaf jky_recon_pair_top >"$WORK/expected.names"
cmp "$WORK/expected.names" "$WORK/used.names"
cmp "$WORK/expected.names" "$WORK/embedded.names"

"$WORK/standalone" --extract jky_recon_pair_leaf >"$WORK/extracted.sh"
cmp "$SOURCE_ROOT/tests/phase5/fixtures/registry/jky_recon_pair_leaf.sh" "$WORK/extracted.sh"
grep -q "$(sha256sum "$WORK/extracted.sh" | cut -d' ' -f1)" "$WORK/embedded.out"

if "$JOCKYC" "$SOURCE_ROOT/tests/phase4/empty_capabilities.jky" \
  --registry "$SOURCE_ROOT/stat_scripts" -o "$WORK/denied" \
  >"$WORK/denied.out" 2>"$WORK/denied.err"; then
  echo "denied program compiled" >&2
  exit 1
fi
test ! -e "$WORK/denied"
grep -q 'capability gate denied compilation' "$WORK/denied.err"

"$JOCKYC" "$SOURCE_ROOT/tests/phase6/no_calls.jky" \
  --registry "$SOURCE_ROOT/stat_scripts" -o "$WORK/empty" >"$WORK/empty-build.out"
"$WORK/empty" --list-embedded >"$WORK/empty.out"
grep -q 'scripts=0' "$WORK/empty.out"
test "$(grep -c '^EMBEDDED ' "$WORK/empty.out")" -eq 0
