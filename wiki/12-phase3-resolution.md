# Phase 3 — `call` Resolution + Arity/Type Checking + Return-Type Plumbing

> Phase 3 record. New semantic unit: `include/jocky/semantic/call_resolver.hpp`
> over the Phase 1 AST + Phase 2 registry index. Runner: `jocky resolve
> <file.jky> [--registry <dir>]` (parse-only `jocky check` output frozen —
> see §5). Fixtures live at `tests/phase3/`; every output below is pasted
> verbatim from the session's real runs.

## 1. The Queued `= default` Decision — Decided Explicitly

Header syntax (extends `wiki/06` §4): an `@jocky:inputs` entry may carry
`= <default>`, e.g. `target:string, ports:string=1-1024`. Live registry
examples: `bpf: string = ""`, `min_txt: int = 5`, `max_cv: float = 0.20`.

Decision: **defaults are stored, not dropped.** `ScriptMetadata::inputs`
is now `vector<InputParam>` (`name`, default-free `type`, `has_default`,
`default_value` with one pair of surrounding double quotes stripped).
Call sites may omit defaulted inputs; the resolver fills
`ResolvedArg{used_default: true, default_value}`. Inputs without a
default are required — missing them is an error naming the argument.

Scanner validation (new rejections, proven in §4): a present-but-empty
default (`name: type =`) is rejected; a default that does not satisfy
its own declared type is rejected (`string`/`path` accept anything,
`int`/`float` must scan numeric, `bool` must be `true`/`false`, unknown
type names never match). The JSON index now emits per input
`{"name","type","has_default":bool,"default":"..."}` — additive,
deterministic; the resolver consumes the scanner's index struct (same
library the JSON serializes), never re-parses headers.

## 2. Resolver Rules

`resolve_call` (throws `SemanticError` with `line:col`, never partial):

1. **Lookup** — dotted call name against registry function names.
   Unknown → `unknown function '<name>' (no registry entry; run
   scan_registry to rebuild the index)` at the `call` keyword position.
2. **Arity** — each schema input matched by name. Missing + no default
   → `missing required argument '<a>' for function '<f>' (no default
   declared)` at the call position. Provided but undeclared →
   `unknown argument '<a>' for function '<f>' (declared inputs: ...)`
   at the argument-name position.
3. **Types (parse-level)** — String accepts `string`|`path`; Int→`int`;
   Float→`float`; Bool→`bool`; List never matches scalar schema types.
   Mismatch → `type mismatch for argument '<a>' of function '<f>':
   declared '<t>' but got <kind>` at the argument position, naming both
   types. Field/source/correlate/nested-call references are dynamically
   typed: accepted here, re-validated at runtime (Phase 7).
4. **Plumbing** — `normalize_output_type`: `table<X>` as-is; bare
   `text`/`json`/`csv` → `table<W>`; anything else → `unknown output
   type` error. `resolve_program` walks rule + investigation bodies
   (including calls nested in predicates and list literals) and records
   every `let x = call ...` as `binding_types[x]` = result TypeRef, so
   `x | where ...` type-checks later without re-deriving.

Deferred by design: capability gating (Phase 4), tree-shaking (Phase 5).
Source locations: `CallExpr`/`NamedArg` now carry `line`/`col` set by the
parser (`call` keyword / argument name); no grammar change.

## 3. Fixture Runs (all six, verbatim)

`$ ./build/jocky resolve tests/phase3/call_unknown.jky` —
`investigate t { let x = call jky_netforensics_frob_widgets(pcap_path: "a.pcap"); }`:

```text
error: tests/phase3/call_unknown.jky:2:11: unknown function 'jky_netforensics_frob_widgets' (no registry entry; run scan_registry to rebuild the index)
EXIT=1
```

`$ ./build/jocky resolve tests/phase3/call_missing_required.jky` —
`investigate t { let flows = call jky_netforensics_extract_flows(bpf: "tcp port 443"); }`:

```text
error: tests/phase3/call_missing_required.jky:2:15: missing required argument 'pcap_path' for function 'jky_netforensics_extract_flows' (no default declared)
EXIT=1
```

`$ ./build/jocky resolve tests/phase3/call_default_ok.jky` —
`investigate t { let flows = call jky_netforensics_extract_flows(pcap_path: "evidence/capture.pcap"); }`:

```text
RESOLVED call jky_netforensics_extract_flows -> table<flow> [capability netforensics.pcap.read]
  arg pcap_path: path
  arg bpf: string = <default "">
  arg out_csv: path = <default "">
BOUND flows: table<flow>
EXIT=0
```

`$ ./build/jocky resolve tests/phase3/call_type_mismatch.jky` —
`investigate t { let r = call jky_netforensics_top_talkers(flows_csv: "f.csv", top_n: "ten"); }`:

```text
error: tests/phase3/call_type_mismatch.jky:2:65: type mismatch for argument 'top_n' of function 'jky_netforensics_top_talkers': declared 'int' but got string
EXIT=1
```

`$ ./build/jocky resolve tests/phase3/call_extra_arg.jky` —
`investigate t { let flows = call jky_netforensics_extract_flows(pcap_path: "a.pcap", threshold: 0.75); }`:

```text
error: tests/phase3/call_extra_arg.jky:2:72: unknown argument 'threshold' for function 'jky_netforensics_extract_flows' (declared inputs: pcap_path, bpf, out_csv)
EXIT=1
```

`$ ./build/jocky resolve tests/phase3/call_ok.jky` —
`investigate t { let flows = call jky_netforensics_extract_flows(pcap_path: "evidence/capture.pcap", bpf: "tcp port 443", out_csv: "out/flows.csv"); }`:

```text
RESOLVED call jky_netforensics_extract_flows -> table<flow> [capability netforensics.pcap.read]
  arg pcap_path: path
  arg bpf: string
  arg out_csv: path
BOUND flows: table<flow>
EXIT=0
```

Note: `samples/sample.jky` itself would NOT resolve — its call passes
`threshold`/`verbose`, which the real `extract_flows` schema never
declared (Phase 1 fixture written against an imagined schema). The
sample stays a parse-level fixture; resolution strictness is intended,
not a regression. No change made to it.

## 4. Scanner Default-Validation Spot Checks (verbatim, /tmp hostile)

```text
REJECT /tmp/jky-hostile/bad_default.sh: default 'notanumber' for input 'count' does not match declared type 'int'
REJECT /tmp/jky-hostile/empty_default.sh: empty default for input 'target' (write `name: type` with no `=` when there is no default)
SUMMARY 0 registered, 2 rejected, 0 skipped
EXIT=1
```

## 5. Regression Evidence (verbatim)

`$ ./build/scan_registry stat_scripts/` → `SUMMARY 7 registered,
0 rejected, 42 skipped`, `EXIT=0` — counts unchanged by the metadata
extension. JSON now carries defaults, e.g. `extract_flows` inputs:

```json
[{"name": "pcap_path", "type": "path", "has_default": false, "default": ""}, {"name": "bpf", "type": "string", "has_default": true, "default": ""}, {"name": "out_csv", "type": "path", "has_default": true, "default": ""}]
```

`$ ./build/jocky check samples/sample.jky` → AST identical to the
`wiki/11` §1 block, `EXIT=0` (check path untouched by the refactor).

Quarantine: untouched this session — baseline stays as published in
`wiki/11` §3.
