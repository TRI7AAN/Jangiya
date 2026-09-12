# JOCKY API Contracts — Schemas & CLI Surface

These are **internal contracts** — JSON schemas and CLI surface, not
network APIs. JOCKY has no network surface.

## 1. CLI Commands

```text
jockyc <file.jky> [-o output] [--list-used]
jocky <file.jky>
jocky check <file.jky>
jocky resolve <file.jky> [--registry <dir>]
jocky gate <file.jky> [--registry <dir>]
jocky verify <manifest.json>
scan_registry <dir>
```

| Command | Behavior |
| ------- | -------- |
| `jockyc <file.jky> [-o output] [--list-used]` | Compile: parse, check, resolve the tree-shaken script closure, embed it, link to a standalone binary at `output` (default `./a.out`). `--list-used` prints the resolved function list and exits without linking. |
| `jocky <file.jky>` | Interpret: same front end and checks, but execute directly against the filesystem registry with no compile step. Manifest semantics identical to the compiled path (parity). Also supports `jocky check <file.jky>` (parse + check only) for the demo's first beat. |
| `jocky resolve <file.jky> [--registry <dir>]` | Phase 3 diagnostic: parse, then resolve every `call` against the registry index (`stat_scripts/` by default), printing each resolved call with its inferred result type and `let`-binding types. Errors use `file:line:col` diagnostics; exit non-zero. Capability gating is NOT applied here — that is `jocky gate` (Phase 4). |
| `jocky gate <file.jky> [--registry <dir>]` | Phase 4 authorization verdict: lex → parse → resolve → bind (exactly one `case`, `BindingError` otherwise) → fail-closed gate over every resolved call's recorded capability. Prints `ALLOWED: N calls authorized under case '<name>'` with the per-call list (exit 0), or `DENIED: N violation(s) under case '<name>'` with one `<file>:<line>:<col>` denial line per violating call naming function + required capability + case (exit 1). Binder-stage refusals print `error: <file>: <message> (case binding)`. Full record in `wiki/13-phase4-capability-gate.md`. |
| `jocky verify <manifest.json>` | Re-hash declared inputs/outputs, re-check per-execution entries, report PASS/FAIL. Reads only the manifest and referenced artifacts; never executes scripts. |
| `scan_registry <dir>` | Walk `<dir>` for `@jocky:` headers, build (or rebuild) the registry index, report indexed functions and header errors. |

## 2. FIR JSON Schema (Forensic IR)

The serializable execution plan passed from the front end to the policy
validator, resolver, and runtime.

```json
{
  "fir_version": "0.1.0",
  "investigation": "incident_case_01",
  "capabilities": ["netforensics.pcap.read", "timeline.correlate"],
  "inputs": [
    { "name": "ev", "adapter": "pcap", "path": "evidence/capture.pcap" }
  ],
  "operations": [
    { "op": "call", "function": "jky_netforensics_extract_flows",
      "args": { "pcap_path": "$ev", "bpf": "tcp port 443" },
      "bind": "flows" },
    { "op": "filter", "table": "flows", "predicate": "bytes > 1000000", "bind": "big" },
    { "op": "correlate", "left": "big", "right": "dns",
      "within": "5m", "on": ["src_ip"], "bind": "timeline" }
  ],
  "writes": [
    { "path": "out/incident_report.md", "from": "timeline" }
  ],
  "prohibited": ["evidence.write", "exec.ungated", "net.egress"]
}
```

- `investigation`: name from the `investigation` declaration.
- `capabilities`: union of capabilities required by all `call` ops; must
  be a subset of the case's `allowed_capabilities`.
- `inputs`: declared evidence sources with adapter + path.
- `operations`: ordered plan steps (`call`, `filter`, `select`,
  `correlate`, …) with bindings.
- `writes`: declared outputs; none may target an `inputs[].path`.
- `prohibited`: operations the policy validator must reject if present.

## 3. Evidence Integrity Manifest Schema

Emitted on every run (success, failure, or timeout — no unlogged
execution).

```json
{
  "manifest_version": "0.1.0",
  "case_id": "incident_case_01",
  "script_sha256": "<sha256 of the .jky source>",
  "runtime_version": "jocky 0.1.0",
  "run_start_utc": "2026-09-11T00:00:00Z",
  "run_end_utc": "2026-09-11T00:01:12Z",
  "authorization": { "case": "incident_case_01",
    "allowed_capabilities": ["netforensics.pcap.read"] },
  "inputs": [
    { "path": "evidence/capture.pcap", "sha256": "<hex>",
      "bytes": 1048576, "adapter": "pcap" }
  ],
  "outputs": [
    { "path": "out/incident_report.md", "sha256": "<hex>", "bytes": 4096 }
  ],
  "executions": [
    { "function": "jky_netforensics_extract_flows",
      "script_sha256": "<hex of embedded/registry script>",
      "args": { "pcap_path": "evidence/capture.pcap", "bpf": "tcp port 443" },
      "exit_code": 0,
      "start_utc": "2026-09-11T00:00:05Z",
      "end_utc": "2026-09-11T00:00:47Z",
      "stdout_sha256": "<hex>",
      "timed_out": false }
  ]
}
```

Per-script-execution entries carry: `function`, `script_sha256`, `args`,
`exit_code`, `start_utc`, `end_utc`, `stdout_sha256`, `timed_out`.
Capability denials are recorded as entries with a denial status rather
than executed. `jocky verify` re-hashes `inputs[]`/`outputs[]` and checks
every entry for completeness.

## 4. @jocky: Script Metadata Header Schema

Every stdlib/registry shell script carries this header (see
`wiki/02-architecture.md` for an example). Fields:

| Key | Required | Meaning |
| --- | -------- | ------- |
| `function` | yes | Callable name; must match `jky_<domain>_<verb>_<object>` |
| `domain` | yes | Exactly one of `recon`, `netforensics`, `hostforensics`, `timeline`, `compliance`, `report` |
| `description` | yes | One-line human description |
| `inputs` | yes | Typed arg list, e.g. `pcap_path: path, bpf: string = ""`. An entry may carry `= <default>` (Phase 3 decision, see `wiki/12-phase3-resolution.md` §1): the scanner stores the default on `InputParam` (`type` kept default-free), validates it against the declared type, and rejects empty or mistyped defaults. Call sites may omit defaulted inputs. The JSON index emits per input `{"name","type","has_default":bool,"default":"..."}`. |
| `outputs` | yes | Typed results, e.g. `flows: table<flow>` |
| `capability` | yes | Required capability, e.g. `netforensics.pcap.read` |
| `timeout_seconds` | yes | Sandbox timeout (positive int) |
| `depends_on` | no | Comma-separated callees included by the tree-shaker |

`scan_registry` rejects scripts with missing/invalid fields, bad
`jky_<domain>_<verb>_<object>` names, unknown domains, or unresolvable
`depends_on`.

## 5. .jky Grammar EBNF (as implemented, Phase 1 skeleton; unchanged by Phases 2–4)

```ebnf
program        = { case_decl | evidence_decl | rule_decl | investigation_decl } ;
case_decl      = "case" ident "{" { case_field } "}" ;
case_field     = "allowed_capabilities" ":" "[" string { "," string } "]" ";" ;
evidence_decl  = "evidence" ident ":" adapter "(" string ")" ";" ;
adapter        = "pcap" | "eventlog" | "directory" ;
rule_decl      = "rule" ident "(" [ params ] ")" "->" type block ;
params         = param { "," param } ;
param          = ident ":" type ;
type           = ident [ "<" type { "," type } ">" ] ;
block          = "{" { statement } "}" ;
investigation_decl = "investigate" ident block ;
statement      = [ "let" ident "=" ] pipeline_expr ";" ;
pipeline_expr  = expr { "|" pipeline_op } ;
expr           = source_call | call_expr | correlate_expr | field | literal | list ;
source_call    = "source" ident ;
call_expr      = "call" qualified_name "(" [ arg_list ] ")" ;
qualified_name = ident { "." ident } ;
arg_list       = named_arg { "," named_arg } ;
named_arg      = ident ":" expr ;
correlate_expr = "correlate" "(" ident "," ident ")" "within" duration "on" field_list ;
duration       = int unit ;
unit           = "ms" | "s" | "m" | "h" | "d" ;
field          = ident { "." ident } ;
field_list     = field { "," field } ;
literal        = string | int | float | bool ;
list           = "[" [ expr { "," expr } ] "]" ;
pipeline_op    = filter_op | select_op | correlate_op | write_op | where_op
               | group_by_op | having_op | sort_by_op | limit_op | emit_op ;
filter_op      = "filter" predicate ;
where_op       = "where" predicate ;
having_op      = "having" predicate ;
select_op      = "select" select_item { "," select_item } ;
select_item    = field [ "as" ident ] ;
correlate_op   = correlate_expr ;
write_op       = "write" ( path | "report" "(" path ")" ) ;
group_by_op    = "group_by" field_list ;
sort_by_op     = "sort_by" field_list [ "asc" | "desc" ] ;
limit_op       = "limit" int ;
emit_op        = "emit" ( ident | string ) ;
predicate      = or_expr ;
or_expr        = and_expr { "or" and_expr } ;
and_expr       = unary { "and" unary } ;
unary          = "not" unary | comparison ;
comparison     = operand [ comp_op operand ] ;
comp_op        = "==" | "!=" | "<" | "<=" | ">" | ">=" | "in" | "contains" | "contains_any" ;
operand        = call_expr | field | literal | list ;
```

Phase 1 amendments vs the Phase 0 draft (all implemented in
`include/jocky/`, exercised by `samples/sample.jky`):
- Investigation blocks use the `investigate` keyword (lexer keyword
  set governs); the draft's `"investigation"` keyword is retired.
- `call`, `filter`, `write`, `let` are keywords (required by the
  grammar above though missing from the first keyword sketch).
- New pipeline operators: `where`, `group_by`, `having`, `sort_by`,
  `limit`, `emit`. `correlate` is accepted both as an expression head
  (`let t = correlate(a, b) within 5m on f | ...`, matching
  `03-graph-schema.md` usage) and as a pipe operator.
- New expression forms: `source <evidence>` references, `select f as
  alias`, list literals (for `in`), float/bool literals, dotted field
  paths, `within <int><unit>` durations.
- Lexer extras: `#` line comments; `true`/`false` lex as BoolLit.
  Reserved for later phases (lexed, not yet parsed): `report`
  (partially: `write report(...)`), `when`, `score`, `tag`, `json`,
  `csv`, `markdown`, `html`, `readonly`.

- `case_decl` fixes the authorization scope for the investigation.
- `evidence_decl` binds a name to a read-only adapter + source path.
- `rule_decl` defines reusable typed transforms; `investigation_decl`
  is the top-level runnable sequence.
- `call_expr` invokes a registry function by qualified name with
  schema-checked named arguments.
- Pipeline operators: `filter`/`where`/`having` (predicates), `select`,
  `correlate...within...on...`, `group_by`, `sort_by`, `limit`, `emit`,
  `write` (outputs only; evidence targets rejected).

Phase 4 AST note (surface syntax unchanged — the EBNF above still parses
exactly the same language): the parsed `CaseDecl` carries a parser-set
`capabilities_declared` flag (`false` by default, set `true` when the
`allowed_capabilities` field header is parsed, list empty or not), so the
Phase 4 binder can distinguish "field never written" (bind-time refusal)
from "field written as `[]`" (binds fine, gate denies). The frozen
`jocky check` printer does not render the flag. Details and fixture
proof in `wiki/13-phase4-capability-gate.md`.
