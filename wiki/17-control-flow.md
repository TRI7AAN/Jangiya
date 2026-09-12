# Phase 5.5 Record: Control-Flow Extension (if/else, for, while)

> `.jky` gains C-style control flow without touching a single stdlib
> script: `if`/`else`, C-shape `for` with statically boundable bounds,
> and syntactically open `while` flagged for runtime ceilings. The
> extension preserves every static guarantee Phases 3–5 established by
> keeping the executed call set finite and fully known at compile time.
> Companion proposal doc: `wiki/17-control-flow-proposal.md` (the
> Path 1 / Path 2 analysis this implements the Path 1 half of).

## 1. C-style syntax choice

Braces for blocks, parenthesized conditions, `for (int i = 0; i < N;
i++)` — the brief locked C/C++/Java familiarity and the grammar follows
it (`wiki/06` §5). New lexer keywords: `if else for while int` (`true` /
`false` and `{` / `}` already existed). The `for` header is syntactic
sugar for familiarity: init/condition/increment variable names must
match (parse-time shape check), but only the bound carries semantic
weight. `++` arrives as two `"+"` symbols (no lexer token); `count` is
contextual like `source` (the count branch only when `count` is
immediately followed by `"("`). Deliberately NOT added (brief §5):
custom functions, return statements, general variable mutation —
flagged as future extension if ever wanted, never snuck in. Likewise no
`CallStmt`: calls live only inside pipelines, so bare `call f(...);`
statements do not exist (deviation from the brief's draft `stmt` rule,
which named an undefined `call_stmt` — dropped, not silently built).

## 2. The three allowed `for`-bound forms (and why)

`semantic/bound_checker.hpp` accepts exactly: (1) integer literal,
(2) a name bound to an integer literal in an enclosing scope,
(3) `count(<table>)` where the target names a visible `let` binding.
Why these three: each is computable without executing anything — a
literal is known, an int-`let` is known (`let N = 5;` records N→5 while
walking scopes inward; the loop variable itself is never a constant),
and `count(t)` pins a statically known *reference* (the table is bound
before the loop; its size is runtime data but the loop is still bounded
by a pre-loop value). Anything else — paradigmatically a bound derived
from a call result (`let r = call …; for (…; i < r; …)`) — is rejected
naming the actual bound, because a trip count flowing from unvalidated
runtime data reintroduces unboundedness. Terminology correction vs the
brief: "case-level declared constants" cannot exist — case blocks
grammatically hold ONLY `allowed_capabilities` — so the implemented rule
is *enclosing integer let-bindings*, documented here instead of
pretended otherwise. The check runs after resolution (a bad bound may
reference a call; unknown-function errors keep precedence) and before
binding, in `resolve`/`gate`/`shake`; `check` stays parse-only.

## 3. `requires_runtime_ceiling` — forward commitment for Phase 7

`while` is syntactically unrestricted (any predicate), so the static
passes cannot bound it: every `WhileStmt` node carries
`requires_runtime_ceiling = true` (set by the parser, always, visible in
`jocky check` output — fixture 5). **Phase 7's runtime dispatcher MUST
enforce a hard iteration cap on any `WhileStmt` at execution time.**
This session implements no enforcement — the flag is the commitment
mechanism, matching the Phase 4 pattern (static gate now, dispatch
re-check later). A `while` without a ceiling at run time is an unbounded
loop, i.e. precisely the construct §2's trade-off analysis bars from
the static chain; the ceiling is what keeps Path 1's promise at
execution.

## 4. Shared walker — one recursion, all passes

`ast/walker.hpp` is now the ONLY place that knows how to find every
`CallExpr`: pipeline heads, predicate operands (incl. nested calls and
lists), `if` conditions + both bodies, `for`/`while` bodies, at any
depth. Two entry points over one documented recursive shape:
`for_each_call_in_block` (passes needing calls) and
`for_each_pipeline_stmt_in_block` (passes needing per-statement context
— the resolver's `let`-binding plumbing). `call_resolver.hpp`'s old
ad-hoc traversal (`resolve_value`/`resolve_predicate`/`resolve_op`) was
deleted and re-expressed on the walker with byte-identical Phase 3
semantics (first-collected call per pipeline statement is still the head
call — regression proves it). Truthfully noted vs the brief: `case_binder`
and `capability_gate` needed NO changes — they never walked the AST
(the binder reads `prog.cases`, the gate reads `BoundProgram`'s resolved
calls), so the layering meant only the resolver required recursion. The
brief assumed three AST walks; the code has one, and fixture 6 proves
every downstream pass inherits it: the 3-deep call resolves with
`BOUND`, gates `ALLOWED`, and shakes embedded.

## 5. Conservative both-branches-gated decision (fixture 1)

The gate authorizes BOTH `if`/`else` branches' capabilities even though
only one runs: the runtime choice is unknowable statically, and
fail-closed leaves no alternative (authorizing the taken branch is
impossible; authorizing neither bans conditionals). A program whose
`else` calls an ungranted capability is DENIED in full. Shake likewise
takes the UNION of both branches — `jockyc` may embed an untaken
branch's script. Bounded, auditable via `jocky shake`, and the explicit
accepted cost of static soundness (carried forward from the proposal
doc's §3).

## 6. Fixture runs (verbatim, `$?` captured immediately)

`jocky gate` from the repo root, real registry:

Fixture 1 — both branches authorized → `ALLOWED: 2`, `EXIT=0`:

```text
ALLOWED: 2 calls authorized under case 't'
  call jky_netforensics_extract_flows -> table<flow> [capability netforensics.pcap.read]
  call jky_netforensics_top_talkers -> table<indicator> [capability netforensics.flow.analyze]
```

Fixture 2 — literal bound, call present once → `ALLOWED: 1`, `EXIT=0`
(and `jocky shake` lists exactly 1 script — trip count never multiplies
the static closure):

```text
ALLOWED: 1 calls authorized under case 't'
  call jky_netforensics_extract_flows -> table<flow> [capability netforensics.pcap.read]
```

```text
SHAKEN: 1 scripts in dependency order
  jky_netforensics_extract_flows (stat_scripts/netforensics/jky_netforensics_extract_flows.sh) [direct call]
```

Fixture 3 — `let N = 5;` constant bound → `ALLOWED: 1`, `EXIT=0` (same
shape as fixture 2).

Fixture 4 — call-derived bound → bound-check refusal naming `r`,
`EXIT=1` (resolve itself succeeds; the refusal stage is the proof):

```text
error: tests/phase5.5/fixture4_for_callbound.jky:9:23: loop bound 'r' is bound to a non-constant value (a trip count derived from call results or other runtime data is not statically boundable; use an integer literal, an integer let-binding, or count(name)) (bound check)
```

Fixture 5 — `while` parses; flag visible in the debug dump (`jocky
check`, `EXIT=0`):

```text
    while (f == f) [requires_runtime_ceiling] {
      f | emit fin;
    }
```

Fixture 6 — if→for→if, 3-deep call → `ALLOWED: 3`, `EXIT=0`; `resolve`
shows `BOUND deep`/`BOUND tls` plumbing intact inside nesting; `shake`
embeds all three scripts (top_talkers' transitive dep deduped —
3 scripts, not 4); `check` renders the full nesting with indentation.
Full outputs in the session log row; PART C ran all four commands
(check/resolve/gate/shake) against fixture 6, all exit 0.

## 7. Regression evidence (same build)

- `jocky check samples/sample.jky`: byte-identical to `wiki/11` §1
  (diffed programmatically) — the printer generalization changed
  nothing for flow-free programs.
- `scan_registry stat_scripts/`: 7/0/42, exit 0.
- `tests/phase3/`: `0,0,1,1,1,1` — resolver refactor preserved Phase 3
  semantics exactly (including `= default` fill and BOUND plumbing).
- `tests/phase4/`: `1,1,1,1,0,1` — binder/gate untouched and unaffected.
- `tests/phase5/` (throwaway registry): `0,0,0,1,0` — shaker unaffected
  (it consumes `GateResult`, never the AST).

## 8. Definition-of-Done mapping (brief PARTs)

- PART A: keywords (`if else for while int`), EBNF (`wiki/06` §5),
  `IfStmt`/`ForStmt`/`WhileStmt`/`BoundExpr`/`Stmt`/`Block`,
  `parse_if/for/while/block`, generalized bodies (rules too — shared
  `parse_block`), indented printer. Deviations logged: no `CallStmt`
  (§1), `int`-as-keyword required a `parse_type` accommodation.
- PART B: `bound_checker.hpp` (three forms + scope tracking +
  call-result rejection); walker shared by the resolver (binder/gate/
  shaker provably needed nothing — §4); `requires_runtime_ceiling`
  always true with §3's Phase 7 commitment.
- PART C: no new command; all four existing commands handle control
  flow (fixture 6 × check/resolve/gate/shake, all exit 0).
- PART D: 6/6 `tests/phase5.5/` as specified above.
