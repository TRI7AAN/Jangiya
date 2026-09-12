# Phase 4 Record: Case Binding + Capability Gate (Static Check)

> Phase 3 proves a call is *well-formed*; Phase 4 proves it is *allowed*.
> New passes: `include/jocky/semantic/case_binder.hpp` (exactly-one-case
> binding) and `include/jocky/policy/capability_gate.hpp` (fail-closed
> static gate over resolved calls), wired as `jocky gate <file.jky>`.
> `call_resolver.hpp` untouched; `jocky check` output frozen
> (byte-identical to `wiki/11` §1, re-verified this session); 7/0/42
> registry and 6/6 `tests/phase3/` outcomes unchanged. Build ran in
> `/tmp/jocky-p4` (removed afterward). Toolchain: g++ 15.2.0, cmake 4.3.4.

## 1. PART 0 — `capabilities_declared` (AST correction)

`CaseDecl` gained one field (`include/jocky/ast/ast.hpp`):

```cpp
struct CaseDecl {
    std::string name;
    std::vector<std::string> capabilities;
    bool capabilities_declared = false;
};
```

`parse_case_decl` sets it `true` the moment the `allowed_capabilities`
field header is parsed — regardless of whether the list inside is empty
or non-empty (`include/jocky/parser/parser.hpp`, right after
`expect_symbol("[")`). This is what lets semantic code distinguish
"field never written" (`case t {}` → `false`, binder error) from "field
written as `[]`" (→ `true` with an empty list, gate denial). The frozen
`jocky check` printer does not render the flag, so no baseline changes.

## 2. Binding rule (locked): exactly one case per Program

`bind_program(prog, resolved)` (`case_binder.hpp`) enforces, in order:

1. `prog.cases` empty → `BindingError` ("found 0").
2. `prog.cases` larger than 1 → `BindingError` listing every duplicate
   name. Every rule and investigation is implicitly bound to the single
   case; no `for case X` syntax was added.
3. `bound.capabilities_declared == false` → `BindingError` naming the
   case. A case that never wrote the field fails HERE.

Success yields `BoundProgram{ bound_case, resolved }` — the single case
plus all resolved calls, ready for the gate. `BindingError` carries
line/col (0/0 for these program-level errors — no single token to point
at); the CLI prints them as `error: <file>: <message> (case binding)`.

## 3. Layering rationale — why the gate is a separate pass

The authorization chain is three headers, three error types, three test
scopes, each consuming the previous stage's output and never redoing it:

| Stage | Header | Consumes | Proves | Error |
|---|---|---|---|---|
| Resolution | `semantic/call_resolver.hpp` (Phase 3, untouched) | AST + registry | call is well-formed | `SemanticError` |
| Binding | `semantic/case_binder.hpp` (new) | Program + ResolvedProgram | call has exactly one scope | `BindingError` (thrown, pre-gate) |
| Gating | `policy/capability_gate.hpp` (new) | BoundProgram | call is allowed | `GateResult::Denied` (returned, post-binding) |

Rationale, matching the Phase 2/3 isolated-testability pattern: each
question ("well-formed?", "scoped?", "allowed?") fails with a different
type at a different stage, so a test failure names the broken layer.
Merging the gate into `resolve_program` would conflate lookup/arity/type
errors with authorization denials and force the resolver to know about
cases — a dependency the resolver deliberately does not have (it sees
only calls + registry). `policy/` is a new top-level include directory
because gating is policy over resolved facts, not semantics of the
language; future policy checks (evidence-write rejection in Phase 5's
validator, per roadmap) will live beside it.

## 4. Gate semantics — fail-closed, all denials collected

`check_gate(bound)` compares each `ResolvedCall::capability` against
`CaseDecl::capabilities` (real field name used throughout — no
`allowedCapabilities` alias). Any miss denies the WHOLE compilation
(fail-closed: `allowed = violations.empty()`), but every violation is
collected first — one run shows the complete gap, never just the first
miss. Each `GateViolation` carries function, required capability, case
name, and call-site line:col; the CLI prints
`DENIED: N violation(s) under case '<name>'` plus one
`<file>:<line>:<col>: call '<fn>' requires capability '<cap>' not
granted by case '<case>'` line each (exit 1). The allowed path prints
`ALLOWED: N calls authorized under case '<name>'` with the per-call list
(exit 0); `GateResult::authorized` (source order) is the list Phase 5
tree-shaking operates on.

## 5. Why this isn't sufficient alone (Phase 7 forward note)

This compile-time gate is necessary but NOT sufficient — it must never be
mistaken for enforcement at run time. What the checker guarantees: the
`.jky` source presented at compile time only calls capabilities its case
grants. What it cannot guarantee: that the registry, scripts, or case
file are unchanged at run time; that a re-resolved or hand-built plan
matches the checked one; that dispatch actually refuses a denied call.
Phase 7's runtime dispatcher must therefore INDEPENDENTLY re-check each
call's declared capability against the case's `allowed_capabilities`
immediately before spawning the script, and refuse + manifest-log the
denial on mismatch — defense in depth, with the static gate as the
first layer, not the only one. Do not implement Phase 7 now.

## 6. Known gap (flagged, NOT fixed): case-block metadata fields

Session-brief claim checked against reality: the brief asserted that
`wiki/01-prd.md` shows an example case block with
analyst/authorization/timezone metadata fields that the parser rejects.
Verified false as stated — `wiki/01-prd.md` (read in full, 99 lines)
contains NO example case block at all, and the EBNF (`wiki/06` §5,
line 128: `case_field = "allowed_capabilities" ":" ...`) documents
exactly what the parser implements, so spec and code agree with each
other. The genuine, narrower gap: the case block carries ONLY the
capability list — there is no grammar for case-level provenance metadata
(analyst identity, authorization references, timezone) of the kind
Phase 6/7 manifest work expects (`wiki/01` §3 item 3 lists
authorization among manifest contents; `wiki/07` notes case_id/runtime
provenance). If manifests need that metadata, it is a future grammar
extension. Relevant to Phase 6/7, not blocking Phase 4. Flagged here,
not fixed.

## 7. Fixture runs (verbatim, exits captured into `$ec` immediately per the wiki/14 standing rule)

`jocky gate` run from the repo root against the real 7-function
registry (default `stat_scripts/`):

Fixture 1 — `tests/phase4/no_case.jky` (zero cases → binder):

```text
error: tests/phase4/no_case.jky: no case block: a program must declare exactly one case before any investigation can be authorized (found 0) (case binding)
EXIT=1
```

Fixture 2 — `tests/phase4/two_cases.jky` (duplicates named → binder):

```text
error: tests/phase4/two_cases.jky: multiple case blocks ('alpha', 'beta'): a program must declare exactly one case so every call binds to a single authorization scope (case binding)
EXIT=1
```

Fixture 3 — `tests/phase4/no_capabilities_field.jky` (field absent → binder, NO denied verdict):

```text
error: tests/phase4/no_capabilities_field.jky: case 't' declares no allowed_capabilities field: an authorizing case must explicitly list its granted capabilities (write `allowed_capabilities: [...]`, even if empty) (case binding)
EXIT=1
```

Fixture 4 — `tests/phase4/empty_capabilities.jky` (field present-but-empty → GATE):

```text
DENIED: 1 violation(s) under case 't'
  tests/phase4/empty_capabilities.jky:9:15: call 'jky_netforensics_extract_flows' requires capability 'netforensics.pcap.read' not granted by case 't'
EXIT=1
```

Fixtures 3 vs 4 are genuinely different code paths, not different
messages: fixture 3 throws `BindingError` from `case_binder.hpp`
(pre-gate — binding refuses, so no `DENIED` line can appear and the
`(case binding)` tag marks the stage); fixture 4 binds successfully and
`capability_gate.hpp` returns `Denied` (post-binding `DENIED` verdict).
One-line proof: only fixture 4's output contains the word `DENIED`.

Fixture 5 — `tests/phase4/allow_exact.jky` (exact authorization → pass):

```text
ALLOWED: 1 calls authorized under case 't'
  call jky_netforensics_extract_flows -> table<flow> [capability netforensics.pcap.read]
EXIT=0
```

Fixture 6 — `tests/phase4/partial_allow.jky` (one of three capabilities granted → BOTH denials):

```text
DENIED: 2 violation(s) under case 't'
  tests/phase4/partial_allow.jky:9:16: call 'jky_netforensics_top_talkers' requires capability 'netforensics.flow.analyze' not granted by case 't'
  tests/phase4/partial_allow.jky:10:13: call 'jky_compliance_run_testssl' requires capability 'compliance.tls.scan' not granted by case 't'
EXIT=1
```

(Side note, recorded not fixed: `report` is a reserved lexer keyword and
cannot be a `let`-binding name — fixture 6 first failed at parse with
`expected binding name, found 'report'` and the binding was renamed to
`tls`. Parse-level behavior, unchanged by this session.)

## 8. Regression evidence (same build)

- `jocky check samples/sample.jky`: output byte-identical to the
  `wiki/11` §1 block (diffed programmatically), `EXIT=0`.
- `scan_registry stat_scripts/`: `7 registered, 0 rejected, 42 skipped`
  (42 SKIP lines), `EXIT=0`.
- `jocky resolve` on all six `tests/phase3/` fixtures: `0,0,1,1,1,1`
  with the `wiki/12`-specified diagnostics — unchanged.

## 9. Definition-of-Done mapping

- Investigate-to-case binding enforced (exactly-one-case rule; zero and
  duplicate cases are `BindingError`s); missing-field case is a distinct
  binder error; exit non-zero — §7 fixtures 1–3.
- Capability denials carry `file:line:col` naming capability + function +
  case; all denials collected; exit non-zero — §7 fixtures 4, 6.
- Resolver-vs-gate layering decided explicitly (separate pass, resolver
  untouched) and documented — §3.
- Phase 7 handoff written — §5.
- Tests green on the real registry, allow and deny — §7 fixtures 5 vs
  4/6, spanning `netforensics.*` and `compliance.tls.scan`.
- `logs.md` records Phase 4 as complete; `implementationplan.md`
  rewritten for Phase 5.
