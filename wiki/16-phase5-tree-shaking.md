# Phase 5 Record: Dependency-Aware Tree-Shaking Resolver

> Phase 4 proves every call is *allowed*; Phase 5 computes exactly what
> must be *shipped* for those allowed calls — nothing more.

## 1. Naming decision — `jocky shake` (decided first, before implementation)

The Phase 5 output command is **`jocky shake <file.jky> [--registry
<dir>]`**. Rationale:

- The existing subcommand pattern is single verbs naming the operation:
  `check` (well-formed?), `resolve` (what do calls mean?), `gate` (are
  they allowed?). `shake` continues the pattern: it names the
  tree-shaking operation directly.
- Rejected alternative 1: `jocky gate --list-used`. The gate's contract
  is a verdict (`ALLOWED`/`DENIED`, exit 0/1); bolting a script listing
  onto it conflates authorization with build planning, and a `--list-used`
  flag on a denied gate would have no coherent meaning (denied calls
  contribute nothing to any closure).
- Rejected alternative 2: `jocky resolve --list-used`. Wrong layer:
  resolution is pre-gate by design (resolver deliberately knows nothing
  about cases), while shaking must consume the authorized-only list —
  tree-shaking over raw resolved calls would ship scripts for denied
  capabilities. A listing flag on `resolve` could not honor that without
  breaking the layering `wiki/13` §3 records.
- `shake` therefore sits at the end of the chain — resolve → bind → gate
  → shake — and only runs on an allowed gate, over `GateResult::authorized`.

## 2. Resolver design (`include/jocky/fir/script_resolver.hpp`)

`resolve_scripts(gate, registry)` walks `GateResult::authorized` (refusing
a denied gate outright — there is no meaningful closure over denied
calls) plus the full registry index:

- **Roots in authorized (source) order** — program intent first; each root
  recorded with reason `"direct call"`.
- **Post-order DFS** — dependencies emitted before dependents, so output
  is topological by construction.
- **Alphabetical edge tiebreak** — each node's `depends_on` entries are
  visited sorted by function name. The registry's storage order is an
  index detail the resolver must not depend on (same lesson as Phase 2's
  sorted-traversal fix); the tiebreak is documented here, not incidental.
- **Deduplication** — three-state marking (unvisited/visiting/done);
  done nodes are never re-emitted, so diamonds appear exactly once with
  their first-discovery reason (`"transitive via <parent>"` keeps the
  parent through which the script was first reached).
- **Cycle error** — hitting a `visiting` node throws `ShakeError` with the
  FULL loop (`a -> b -> c -> a`) sliced from the DFS stack and re-closed,
  never just the repeated name.
- **Unresolvable `depends_on`** — throws naming parent + missing
  reference. Defensive: the Phase 2 scanner already rejects these at scan
  time, so reaching it means the index changed under us; the resolver
  must not silently skip. Same treatment for an authorized call with no
  registry entry (carries the call-site line:col, the one positioned
  shake error).

`ShakeError` mirrors `BindingError`'s convention (line/col 0 for
registry-level errors); the CLI tags them `(shake)`.

## 3. Test registry (12 files, not ~8 — deliberate)

`tests/phase5/fixtures/registry/` holds 12 header-only `.sh` files at the
scan root (folder-check exempt; content is `exit 0`). The brief suggested
~8; 12 was chosen so every fixture asserts an EXACT independent set with
no script shared between shapes:

| Scripts | Shape |
|---|---|
| `pair_top → pair_leaf` (leaf) | fixture 1: one direct edge |
| `chain_a → chain_b → chain_c → chain_d` (leaf) | fixture 2: transitive chain |
| `dia_top → {dia_left, dia_right} → dia_base` (leaf) | fixture 3: diamond |
| `cyc_x ↔ cyc_y` | fixture 4: cycle |
| (none new — reuses diamond) | fixture 5: cross-site dedup |

All twelve share one capability (`recon.fixture.execute`) and
all-defaulted inputs (`target: string = "default"`), keeping the five
`.jky` programs to a case block plus bare `call fn()` sites. Gate
behavior is not under test here (Phase 4 proved it); uniformity is
intentional. Registry scans 12/0/0 on this directory (verified live
during the session runs below — every `shake` below resolved against it
with no REJECT lines).

## 4. Fixture runs (verbatim, `$?` captured immediately per wiki/14 rule)

`jocky shake <fixture> --registry tests/phase5/fixtures/registry`, repo
root as CWD:

Fixture 1 — direct (`fixture1_direct.jky`), `EXIT=0`:

```text
SHAKEN: 2 scripts in dependency order
  jky_recon_pair_leaf (tests/phase5/fixtures/registry/jky_recon_pair_leaf.sh) [transitive via jky_recon_pair_top]
  jky_recon_pair_top (tests/phase5/fixtures/registry/jky_recon_pair_top.sh) [direct call]
```

Fixture 2 — chain (`fixture2_chain.jky`), `EXIT=0`:

```text
SHAKEN: 4 scripts in dependency order
  jky_recon_chain_d (tests/phase5/fixtures/registry/jky_recon_chain_d.sh) [transitive via jky_recon_chain_c]
  jky_recon_chain_c (tests/phase5/fixtures/registry/jky_recon_chain_c.sh) [transitive via jky_recon_chain_b]
  jky_recon_chain_b (tests/phase5/fixtures/registry/jky_recon_chain_b.sh) [transitive via jky_recon_chain_a]
  jky_recon_chain_a (tests/phase5/fixtures/registry/jky_recon_chain_a.sh) [direct call]
```

Fixture 3 — diamond (`fixture3_diamond.jky`), `EXIT=0`:

```text
SHAKEN: 4 scripts in dependency order
  jky_recon_dia_base (tests/phase5/fixtures/registry/jky_recon_dia_base.sh) [transitive via jky_recon_dia_left]
  jky_recon_dia_left (tests/phase5/fixtures/registry/jky_recon_dia_left.sh) [transitive via jky_recon_dia_top]
  jky_recon_dia_right (tests/phase5/fixtures/registry/jky_recon_dia_right.sh) [transitive via jky_recon_dia_top]
  jky_recon_dia_top (tests/phase5/fixtures/registry/jky_recon_dia_top.sh) [direct call]
```

Base appears exactly once, before both arms; left precedes right by the
alphabetical edge tiebreak.

Fixture 4 — cycle (`fixture4_cycle.jky`), `EXIT=1`:

```text
error: tests/phase5/fixture4_cycle.jky: dependency cycle detected: jky_recon_cyc_x -> jky_recon_cyc_y -> jky_recon_cyc_x (tree-shaking cannot order a cycle) (shake)
```

Resolve, bind, and gate all succeed first (the capability IS allowed) —
the failure is purely the shake-stage cycle error with the full path.

Fixture 5 — mixed sites (`fixture5_mixed.jky`: `dia_top()` then
`dia_left()`), `EXIT=0`:

```text
SHAKEN: 4 scripts in dependency order
  jky_recon_dia_base (tests/phase5/fixtures/registry/jky_recon_dia_base.sh) [transitive via jky_recon_dia_left]
  jky_recon_dia_left (tests/phase5/fixtures/registry/jky_recon_dia_left.sh) [transitive via jky_recon_dia_top]
  jky_recon_dia_right (tests/phase5/fixtures/registry/jky_recon_dia_right.sh) [transitive via jky_recon_dia_top]
  jky_recon_dia_top (tests/phase5/fixtures/registry/jky_recon_dia_top.sh) [direct call]
```

Identical set to fixture 3 with `dia_left` exactly once despite being
both a transitive dep (via top) and a direct call site — cross-site
dedup proven.

Real-registry spot checks (same build): `jocky shake
tests/phase4/allow_exact.jky` → `SHAKEN: 1 scripts` (`extract_flows`,
no deps), `EXIT=0`; `jocky shake tests/phase4/partial_allow.jky` →
`DENIED: 2 violation(s)` with both lines, `EXIT=1` (shake never runs on
a denied gate — fail-closed path exercised, not just unit-reasoned).

## 5. Regression evidence (same build)

- `jocky check samples/sample.jky`: recorded in the session's
  regression re-run step (see logs.md row); `jocky resolve` on all six
  `tests/phase3/` fixtures and `jocky gate` on all six `tests/phase4/`
  fixtures re-run with the Phase 5 binary — outcomes unchanged
  (phase3 `0,0,1,1,1,1`; phase4 `1,1,1,1,0,1`); `scan_registry
  stat_scripts/`: 7/0/42, exit 0.
- One compile fix during implementation (recorded, not hidden):
  `VisitState` lives in `detail::` but was referenced unqualified in
  `resolve_scripts` — fixed by qualifying to `detail::VisitState`; clean
  build with no errors or warnings after.

## 6. What Phase 6 inherits (handoff note — documented, not built)

`jockyc`'s embedding step will consume this resolver's deduplicated,
topologically ordered `ScriptMetadata` list directly — one embedded unit
per entry, embedded in that exact order (dependencies first, so any
order-sensitive bundling is already correct). Phase 6 must NOT
re-implement deduplication or cycle detection — those are solved here,
once, in `resolve_scripts`. What Phase 6 MUST still do itself: hash each
listed `script_path` at embed time and refuse on mismatch (the resolver
guarantees set/order over authorized calls only — it says nothing about
whether the files changed since the scan). Static plan vs embed-time
bytes is exactly the boundary Phase 7's dispatcher re-verification
extends to run time.
