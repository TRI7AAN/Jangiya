# JOCKY Grammar Reference (`.jky` Syntax + Usage)

> Authoritative syntax reference for the `.jky` language **as implemented**
> (Phases 1–5; surface syntax frozen since Phase 3). Every construct below
> is grounded in `include/jocky/lexer/lexer.hpp` + `include/jocky/parser/
> parser.hpp`; every example was verified with `jocky check` (exit 0) on
> the Phase 5 build, and every stated rejection was verified to fail with
> the quoted diagnostic. The compact EBNF lives in `wiki/06-api-contracts.md`
> §5; this file is the long form with usage notes. If this file and the
> parser ever disagree, the parser wins — file a drift note.

## 0. Complete example

A program is any sequence of `case`, `evidence`, `rule`, and
`investigate` blocks (order-free; the gate requires exactly one `case` —
see §2). This exercises every construct:

```jky
# line comments start with `#`
case incident_01 {
  allowed_capabilities: ["netforensics.pcap.read", "timeline.correlate"];
}

evidence capture: pcap("evidence/capture.pcap");
evidence authlogs: eventlog("evidence/auth.json");

rule flag_big_flows(min_bytes: int) -> table<flow> {
  let big = source capture | where bytes > min_bytes and protocol == "tcp" | select src_ip, dst_ip as dest, bytes | sort_by bytes desc | limit 100;
  big | emit report;
}

investigate incident_01 {
  let flows = call jky_netforensics_extract_flows(pcap_path: "evidence/capture.pcap", bpf: "tcp port 443");
  let auth = source authlogs | where message contains "failed" and not user in ["root", "admin"];
  let tl = correlate(flows, auth) within 5m on src_ip | group_by src_ip | having events > 3 | select src_ip | sort_by src_ip asc | limit 10;
  tl | write report("out/incident_report.md");
  flows | write "out/flows.json";
}
```

## 1. Lexical rules

- **Comments:** `#` starts a line comment (runs to end of line). No block
  comments.
- **Identifiers:** `[A-Za-z_][A-Za-z0-9_]*` — e.g. `src_ip`, `flow`,
  `_tmp`. Used for binding names, field names, evidence names, type
  names, and (with dots) function paths.
- **Keywords (150, reserved — cannot be binding/field names):** the 42
  `.jky` keywords —
  `case evidence rule investigate report emit where select group_by
  having sort_by limit correlate within on as when score tag call filter
  write let pcap eventlog directory json csv markdown html readonly and
  or not in contains contains_any if else for while int` —
  plus the full C, C++, and Java keyword sets (108 more), reserved as
  future syntax space:
  - C: `auto break char const continue default do double enum extern
    float goto inline long register restrict return short signed sizeof
    static struct switch typedef union unsigned void volatile`
    (+ `_Alignas _Alignof _Atomic _Bool _Complex _Generic _Imaginary
    _Noreturn _Static_assert _Thread_local`; `case else for if while
    int` already above).
  - C++ beyond C: `asm bool catch char8_t char16_t char32_t class compl
    concept consteval constexpr constinit const_cast co_await co_return
    co_yield decltype delete dynamic_cast explicit export friend mutable
    namespace new noexcept nullptr operator private protected public
    reinterpret_cast requires static_assert static_cast template this
    thread_local throw try typeid typename using virtual wchar_t`
    (+ `and_eq bitand bitor not_eq or_eq xor xor_eq`; `and or not`
    already above).
  - Java beyond C/C++: `abstract assert boolean byte extends final
    finally implements import instanceof interface native package
    strictfp super synchronized throws transient`.
  A corpus scan at reservation time found zero real collisions (the only
  matches were inside `#` comments, which never tokenize). Verified live:
  `flag: bool` / `x: float` type annotations still parse (type position
  accepts any keyword — it is unambiguous), while `let new = …` now fails
  with `expected binding name, found 'new'`. Deliberately NOT reserved:
  `true`/`false` (lex as BoolLit first) and `null` (a literal would need
  literal semantics — flagged future, not silently added). Trade-off,
  stated plainly: any future field/binding named exactly like one of
  these 108 words is unusable bare — the price of C-familiar reserved
  space; reversal is one lexer table if it ever bites.
  Two deliberate exceptions: `source` and `count` are *contextual* (plain
  identifiers with meaning only in their special positions — elsewhere
  ordinary fields), and `emit` accepts a keyword as its target, so `emit
  report` is legal (§8).
- **Reserved but unparsed:** `when score tag json csv markdown html
  readonly` (and `report` outside `write report(...)` / `emit report`)
  lex fine but have no grammar rule — using one as an operator fails at
  parse time. They are future syntax space, not usable today.
- **String literals:** double-quoted, single line, escapes `\" \\ \n \t
  \r` only — any other escape or an unterminated string is a lex error.
- **Number literals:** integers (`100`, `0`) and floats (`0.75`, `2.5`).
  No negative literals and no arithmetic: `-5` fails with
  `expected operand, found '-'`, and `1 + 2` fails with
  `expected ';', found '+'`. Thresholds like `0.75` travel as positive
  float literals.
- **Boolean literals:** `true`, `false` (lex as BoolLit, not identifiers).

## 2. `case` — authorization scope

```jky
case incident_01 {
  allowed_capabilities: ["netforensics.pcap.read", "timeline.correlate"];
}
```

- The block takes `allowed_capabilities`, whose value
  is a `[`-bracketed, comma-separated string list ending with `;`, plus
  (since Phase 7.5) an optional `max_while_iterations: <non-negative
  int>;` per-case while-loop ceiling. Any other field name is rejected:
  `unknown case field 'analyst'`. A repeated `max_while_iterations`
  field is rejected as a duplicate.
- Both the empty block (`case t {}`) and the empty list
  (`allowed_capabilities: [];`) parse — but they mean different things
  downstream: the parser records field *presence* on
  `CaseDecl::capabilities_declared`, so "never written" fails at bind
  time while "written as `[]`" binds fine and fails at gate time
  (`wiki/13` §1, `tests/phase4/` fixtures 3 vs 4).
- Usage: Phase 4's gate requires exactly one `case` per program; every
  rule and investigation is implicitly bound to it, and each resolved
  call's recorded capability must be a member of this list (`jocky gate`).

## 3. `evidence` — read-only sources

```jky
evidence capture: pcap("evidence/capture.pcap");
evidence authlogs: eventlog("evidence/auth.json");
evidence fs: directory("evidence/host/");
```

- Form: `evidence NAME: ADAPTER("path");` with exactly three adapters —
  `pcap`, `eventlog`, `directory`. Anything else fails at parse time.
- Usage: `source NAME` pipeline heads read from these (§5). Adapters open
  sources read-only by construction; no language feature can write to a
  declared evidence path (AGENTS.md §2.4).

## 4. `rule` — reusable typed transforms

```jky
rule flag_big_flows(min_bytes: int) -> table<flow> {
  let big = source capture | where bytes > min_bytes;
  big | emit report;
}
rule no_params() -> table<flow> {
  big | emit report;
}
```

- Form: `rule NAME([name: TYPE, ...]) -> TYPE { statements }`. The
  parameter list may be empty; each parameter is `name: TYPE` with types
  as in §9 (`int`, `table<flow>`, …).
- Usage: named, parameterized pipeline bundles. (Invocation of rules by
  name is not wired to a call syntax in the current grammar — rules
  document reusable transforms; investigations are the runnable units.)

## 5. `investigate` + statements — the runnable unit

```jky
investigate incident_01 {
  let flows = call jky_netforensics_extract_flows(pcap_path: "a.pcap");
  flows | where bytes > 100 | limit 10;
}
```

- Form: `investigate NAME { statements }`; each statement is
  `[let NAME =] HEAD {| OPERATOR} ;` — the `let` binding is optional, the
  trailing `;` is not.
- `let` names a pipeline result for later statements (`flows | …`). A
  bare pipeline without `let` just runs its stages.
- Binding names must be identifiers — reserved keywords are rejected
  (`let report = …` fails with `expected binding name, found 'report'`).

## 6. Expression heads

Each pipeline starts with one head:

| Head | Form | Usage |
|---|---|---|
| Evidence source | `source NAME` | Rows from a declared `evidence` block. `source` is contextual: only special here; elsewhere it is a plain field name. |
| Registry call | `call a.b.c(arg: expr, …)` | Invoke a registry function by dotted path with **named** arguments (`name: value`; values are full expressions — literals, lists, fields, even nested `call`s). Empty parens (`call fn()`) are legal when every input has a default. Arity/types checked by `jocky resolve` against `@jocky:` headers. |
| Correlation join | `correlate(A, B) within 5m on f` | Table-valued join of two bindings; also usable as a pipe operator (§8). Window + join keys per §7. |
| Field reference | `src_ip`, `flow.bytes` | Dotted path; in head position usually a prior `let` binding. |
| Literal | `"tcp"`, `100`, `0.75`, `true` | A one-row constant table head. |
| List | `["root", "admin"]`, `[1, 2.5, "three", true]` | Inline list (elements are full expressions); used with `in` and as call args. Empty `[]` is legal. |

## 7. Predicates (`filter` / `where` / `having`)

One grammar, three keywords (`filter`, `where`, `having` parse identically
— pick by readability: `where` after a source, `having` after `group_by`):

```jky
source logs | where bytes > 1000000 and protocol == "tcp";
source logs | where message contains "failed" and not user in ["root", "admin"];
source logs | where score contains_any ["a", "b"] | having events > 3;
source logs | where flag == true | where ratio <= 0.5 | where proto != "udp";
```

- Precedence: `or` binds loosest, then `and`, then `not`, then comparison
  (`not x or y` parses as `(not x) or y`; `a and b or c` as `(a and b) or c`).
- Comparisons: `== != < <= > >=` plus `in` (membership in a list),
  `contains` (substring/element match), `contains_any` (match against any
  of a list).
- Operands are `call` results, fields, literals, or lists. A bare operand
  with no operator is a truthiness atom (`having events` is legal).

## 8. Pipeline operators (`| op`)

| Operator | Form | Notes |
|---|---|---|
| `filter` / `where` / `having` | `\| where <predicate>` | Identical grammar; see §7. |
| `select` | `\| select src_ip, dst_ip as dest, bytes` | One or more fields; `field as alias` renames (`as` required for aliases — bare second names are a parse error). |
| `correlate` | `\| correlate(A, B) within 5m on src_ip` | Same spec as the head form (§6). |
| `write` | `\| write "out/f.json"` or `\| write report("out/r.md")` | Plain path writes data; `report(...)` marks report output. The report path may be a string **or** a bare identifier (`write report(outdir)` parses). |
| `group_by` | `\| group_by src_ip` | One or more fields. |
| `sort_by` | `\| sort_by bytes desc` | One or more fields; trailing direction `asc`/`desc` optional (default: unspecified order). |
| `limit` | `\| limit 100` | Non-negative integer literal only. |
| `emit` | `\| emit report` / `\| emit "done"` / `\| emit somename` | Target is a string, an identifier, or a keyword — hence `emit report` (the sample's terminator idiom). |

## 9. `correlate` windows and keys

```jky
correlate(flows, auth) within 5m on src_ip
correlate(a, b) within 90s on src_ip, user
correlate(a, b) within 2h on id
```

- Form: `(LEFT, RIGHT) within NUNIT on f, …` where bindings are prior
  `let` names, `N` is an integer literal, and `UNIT` is one of `ms s m h
  d` (anything else: `unknown duration unit '…'`). One or more join
  fields, comma-separated.

## 10. Types

```jky
int  string  bool  float  path  table<flow>  table<dns_event>
```

- Form: `name` or `name<arg, …>` (nestable). Rule params/returns use
  these; registry `@jocky:` headers declare inputs/outputs in the same
  spelling (`path` travels as a string literal at call sites).
- `call` results normalize to table types: a declared `table<flow>`
  stays as-is; a bare format word (`text`, `csv`) becomes `table<…>`
  for pipeline use (Phase 3 return-type plumbing).

## 11. What each command validates

| Command | Adds over the previous |
|---|---|
| `jocky check` | Syntax only (this whole file). Frozen AST output for flow-free programs. |
| `jocky resolve` | Registry lookup, arity (`= default` fill), literal arg types, return-type plumbing, for-bound check. |
| `jocky gate` | Exactly-one-case binding + fail-closed capability check. |
| `jocky shake` | Dependency closure over authorized calls (cycles/full-path errors). |

A file can be grammatically perfect yet fail at any later stage
(`sample.jky` parses but never resolves — `threshold`/`verbose` are not
real inputs). All diagnostics use `file:line:col` and exit non-zero.

## 12. Deliberately absent (not oversights)

- No arithmetic, no negative literals, no string concatenation (§1).
- No custom functions, return statements, or general variable mutation —
  scoped out in Phase 5.5, flagged as future extension if ever wanted.
- `case` carries only `allowed_capabilities` — no analyst/timezone/
  provenance metadata fields (flagged for Phase 6/7 manifest work,
  `wiki/13` §6).
- Rules are declared but have no invocation syntax from investigations
  in the current grammar (§4).
- No bare `call f(...);` statements — calls live only inside pipelines
  (heads/operands).

## 13. Control flow (Phase 5.5)

Rule and investigation bodies accept `if`/`else`, C-shape `for`, and
`while` with braced blocks, nestable to any depth:

```jky
let N = 5;
let found = false;
if (found == false) {
  let subs = call jky_recon_enum_subdomains(target: "example.com");
} else {
  subs | emit fin;
}
for (int i = 0; i < N; i++) {
  let f = call jky_netforensics_extract_flows(pcap_path: "a.pcap");
  f | emit fin;
}
while (f == f) {
  f | emit fin;
}
```

- `if (predicate) { … } [else { … }]` — predicates are the §7 grammar.
  Both branches are statically collected: the gate authorizes BOTH
  branches' capabilities (fail-closed — the runtime choice is unknowable
  at compile time), and shake embeds the union.
- `for (int i = 0; i < BOUND; i++)` — init/condition/increment names must
  match. `BOUND` is an integer literal, an integer `let`-binding visible
  before the loop (`let N = 5;`), or `count(table)` over a visible
  binding. A bound derived from a call result is rejected at bound-check
  time (`resolve`/`gate`/`shake`, never `check` which stays parse-only).
  Trip count never multiplies the static closure: one presence per
  enclosed call.
- `while (predicate) { … }` — any predicate, but the AST flags every node
  `requires_runtime_ceiling` (visible in `jocky check`); Phase 7 MUST
  enforce a hard iteration cap at execution time. Fulfilled in Phase 7.5:
  the executor re-evaluates the condition per iteration under a hard
  ceiling (default 10000, per-case `max_while_iterations`, per-run
  `--max-iterations`), logging `while_ceiling` on breach
  (`wiki/21-phase7-5-control-flow.md`).
- New keywords: `if else for while int` (`int` remains a valid type name
  — `min_bytes: int` parses as before). Full record:
  `wiki/17-control-flow.md`; EBNF: `wiki/06` §5.
