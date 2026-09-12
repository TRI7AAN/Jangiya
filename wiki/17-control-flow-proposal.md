# Design Proposal: C-Style Control Flow in `.jky` (if/else, repeat, flags)

> **Status: PROPOSAL — not implemented, not decided.** This session writes
> no code: `lexer.hpp`, `parser.hpp`, `ast.hpp`, all resolver/gate/shaker
> headers, and every file under `stat_scripts/` are untouched. The stdlib
> scripts stay untouched under EITHER path below — this is purely about the
> `.jky` orchestration layer gaining `if`/`else`, bounded repetition, and
> boolean flag variables with C/C++/Java-familiar syntax, orchestrating
> calls to unmodified scripts. Implementation is a future prompt once a
> path is explicitly confirmed by the human. Reads for this session:
> `AGENTS.md`, `roadmap.md`, `implementationplan.md` (Phase 5),
> `wiki/06` (grammar), `wiki/13` (gate layering), `wiki/16` (shaker).

## 1. What's being requested

The user wants `.jky` programs to express three things they cannot express
today (verified: `if else for while` appear nowhere in the lexer keyword
set or the parser — `wiki/grammar.md` §12):

1. **Conditionals** — `if`/`else` branching on a predicate, e.g. "only run
   the deep scan if the triage found something."
2. **Bounded repetition** — "run this call N times" (e.g. retry a flaky
   recon probe up to a max count), not open-ended looping.
3. **Flag/boolean variables** — named true/false state driving (1) and (2),
   e.g. `exhausted`, `found_anything`.

With C/C++/Java-familiar surface syntax, and with the hard constraint that
no `.sh` file changes: the scripts keep doing one thing per invocation
(`jky_recon_enum_subdomains.sh` still takes `<target> <session_dir>` and
prints subdomains); all branching and repetition lives in `.jky`, which
decides *whether* and *how many times* to invoke them.

## 2. The static-enumerability trade-off (concrete, with code references)

Everything Phases 3–5 built assumes a Program compiles down to a **finite,
fully known set of calls before anything executes**. That assumption is
load-bearing in exactly two places:

**`check_gate` (`include/jocky/policy/capability_gate.hpp`).** It iterates
`bound.resolved.calls` — a plain finite vector — comparing each call's
statically recorded `capability` string against the case list, with
`allowed = violations.empty()` (fail-closed). The guarantee it produces,
which `wiki/13` §5 records as layer one of two, is: *the source presented
at compile time can only execute granted capabilities.* If a branch
condition depends on a runtime value (a call's output table), the compiler
cannot know which branch runs — so the executed call set is not statically
known, and `GateResult::authorized` **cannot be computed before
execution**. The gate would have to become a runtime-only concept, and the
documented two-layer defense (static proof + Phase 7 dispatch re-check)
collapses to one layer: only the dispatcher check remains.

**`resolve_scripts` (`include/jocky/fir/script_resolver.hpp`).** It runs
post-order DFS from `gate.authorized` over the finite registry index, with
done-marking guaranteeing termination and a closed, ordered list. An
unbounded `while` (condition on runtime data, no static trip count) means
the executed call multiset has no static bound: the shaker cannot
enumerate it, cannot order it, cannot close it. `jockyc` would then have
to embed *every possibly-referenced script* rather than the minimal
closure — the exact bloat Phase 5 exists to prevent — because "which
scripts this run uses" becomes undecidable at compile time in general
(not just unknown: unknowable — branch conditions read evidence only
present at the crime scene).

The dividing line, stated once: **anything the compiler can unroll or
union into a finite call set preserves the whole chain; anything whose
executed calls depend on runtime data breaks the static half of the
safety contract.** The two paths below sit on opposite sides of that line.

## 3. Path 1 — Bounded control flow (statically enumerable)

Proposed grammar (new rules only; everything else unchanged):

```ebnf
if_stmt     = "if" "(" predicate ")" "{" stmt* "}" [ "else" "{" stmt* "}" ] ;
repeat_stmt = "repeat" "(" ( int | ident ) ")" "{" stmt* "}" ;
flag_decl   = "let" ident ":" type "=" expr ";" ;
```

(`predicate` reuses the existing §7 grammar verbatim; `stmt` is the
existing statement rule, now also allowed inside the new blocks.)

Constraints that keep the chain intact:

- **Repeat counts are compile-time constants.** Either an integer literal
  (`repeat (3)`) or an identifier bound *before* the block to a
  statically known int (`let max_retries: int = 3; … repeat
  (max_retries)`). A count derived from ANY call output — including the
  loop's own prior iterations — is rejected at check/resolve time. This
  is a sharpening of the brief's "already resolved and bound" wording:
  bound is not enough, the *value* must be compile-time-known, because
  only then can the compiler unroll `repeat (N)` into N straight-line
  statement copies. After unrolling, resolver, binder, gate, and shaker
  see ordinary finite calls — **zero changes to their logic**.
- **`if` conditions MAY reference call results** (e.g.
  `if (empty(found))` over an already-resolved binding — predicates over
  resolved tables are exactly what §7 already expresses). The price,
  stated honestly: the static passes must treat **both branches as
  reachable**, because the runtime value is genuinely unknown at compile
  time.
- **The brief's design question, answered: YES — the gate must authorize
  BOTH branches' capabilities.** Concretely, `resolve_program` walks into
  both `if` and `else` bodies collecting `ResolvedCall`s unconditionally,
  and `check_gate` checks every one against the case list. Fail-closed
  leaves no alternative: authorizing only the taken branch is impossible
  (the taken branch is unknowable statically), and authorizing neither
  would ban all conditionals. So a program whose `else` branch calls a
  capability the case never granted is DENIED in full — even if that
  branch would never run. Same rule for shake: the closure is the UNION
  of both branches plus transitive deps, so `jockyc` may embed a script
  the run never executes. That over-approximation is bounded (at most the
  untaken branch's subtree — small, and auditable via `jocky shake`) and
  is the explicit, accepted cost of static soundness.
- **What changes in code (future prompt, not this one):** new AST nodes
  (`IfStmt`, plus either a `RepeatStmt` or direct unrolling at parse);
  `resolve_*`/`bind`/`check_gate`/DFS walkers extended to descend into
  branch bodies (collection only — no logic changes); the runtime
  (Phase 7 dispatcher / Phase 8 interpreter) evaluates the branch
  predicate and the unrolled/ counted repetition for real. `repeat` with
  a constant count could alternatively unroll fully at parse time, in
  which case only `if` needs downstream walker support.

## 4. Path 2 — Unbounded control flow (full Turing completeness)

`while (predicate)` with runtime-evaluated conditions, counters mutated by
call outputs, early `break` on findings — the full scripting language.
Stated plainly, what changes:

- **`capability_gate.hpp` stops being a pre-execution proof.** With the
  executed call set unknowable statically, `GateResult::authorized`
  cannot be computed before execution; the static gate degrades to (at
  best) a "referenced anywhere in source" lint, and authorization becomes
  a **runtime-only** check inside Phase 7's dispatcher. The two-layer
  defense documented in `wiki/13` §5 becomes one layer, and every static
  guarantee Phases 5–6 inherit (shake-over-authorized-only, embed-exactly-
  the-closure) is voided at the point of introduction.
- **`script_resolver.hpp` can only compute static reachability, not the
  run's closure.** `jockyc` binaries would embed every script referenced
  anywhere in the source (both branches, every loop body, all transitive
  deps thereof) rather than what a run uses. Honest sizing: today's
  registry is 48 scripts (7 annotated) — embedding "everything possibly
  referenced" for a small investigation is tens of scripts, i.e. the
  whole annotated set in the worst case. The size cost is modest *today*
  but structural: it grows with the registry, and the security cost is
  the real one — a wider embedded audit surface including scripts the
  run never needed, with no static proof about which ones those are.
- **Manifest parity gets harder.** Phase 8's compiled/interpreted
  equivalence was designed around identical static plans; with
  data-dependent control flow, both back ends must agree branch-for-branch
  at run time — testable, but a strictly larger harness.
- Nothing about Path 2 violates AGENTS.md §2 (no evasion, still gated —
  just later; still logged; still read-only; still typed args). It is a
  soundness-and-minimality trade, not a safety violation. It is simply
  more machinery for power the stated use cases ("retry N times",
  "skip if empty") do not need.

## 5. Recommendation (for the human to confirm — NOT a decision)

**Recommend Path 1 for the MVP.** It covers the described use cases
(retry-with-bound, skip-if-empty, flag-driven sequencing) with zero
changes to gate/shaker logic, preserves every static guarantee Phases
3–5 established, keeps `jockyc` artifacts minimal, and leaves the
`while`-shaped door visibly open: Path 2 remains available as a
documented future extension if, after using Path 1, genuine
Turing-complete scripting proves necessary. If Path 1 is confirmed, the
natural implementation slot is a Phase 6-adjacent grammar extension
(frontend + walker support + runtime evaluation in Phase 7/8) —
*not* a renumbering of the current roadmap, which stays as-is until
chosen. **Nothing here is decided; implementation waits on an explicit
Path 1 / Path 2 call.**

## 6. Worked example — bounded retry with a flag (Path 1 syntax, illustrative)

The concrete use case: call `jky_recon_enum_subdomains` repeatedly,
stopping early once a prior call's result table is non-empty, never
exceeding a max retry count — without touching `enum_subdomains.sh`
itself (it still takes `<target> <session_dir>` per call; orchestration
is all `.jky`):

```jky
case recon_sweep {
  allowed_capabilities: ["recon.dns.enumerate"];
}

evidence scope: directory("evidence/scope/");

investigate recon_sweep {
  let max_retries: int = 3;
  let found: bool = false;

  # Bounded: unrolls to exactly 3 guarded attempts at compile time.
  # The gate authorizes enum_subdomains once (same call, same
  # capability, all copies); the shaker embeds it once.
  repeat (max_retries) {
    if (found == false) {
      let subs = call jky_recon_enum_subdomains(target: "example.com");
      let found = subs | where domain != "";
    }
  }

  subs | write "out/subdomains.json";
}
```

How to read it under Path 1 semantics: `repeat (3)` unrolls to three
copies of the guarded body (finite — the shaker sees three identical
authorized calls collapsing to one embedded script). Each copy re-runs
the probe only if no earlier copy set `found`; the `if` condition
references an already-resolved binding, which §3 permits. If the case
had granted nothing, the gate denies the (single, unioned) capability
before anything runs — the static proof survives branching.

> Caveats, stated so this snippet is never mistaken for runnable code:
> (a) `if`/`repeat`/typed-`let` do not parse today — this is proposed
> syntax; (b) `jky_recon_enum_subdomains.sh` is currently headerless
> (Phase 9 will annotate it), so the call would not resolve even if the
> syntax existed; (c) flag rebinding semantics (`let found` twice) and
> the empty-table test spelling are illustrative pending the
> implementation prompt's exact decisions.
