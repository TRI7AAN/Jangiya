# JOCKY Architecture

## 1. Pipeline Overview

```text
.jky source
  -> Lexer
  -> Parser (recursive descent)
  -> AST
  -> Call resolver (Phase 3, built) + capability gate (Phase 4, current)
  -> Forensic IR (FIR)
  -> Policy validator
  -> Tree-shaking resolver
  -> C++ runtime (evidence adapters, correlation engine, script dispatcher)
  -> Evidence bundle + integrity manifest
```

- **Lexer / Parser / AST (Phase 1, extended Phase 3):** C++20 from-scratch
  front end. Tokenizes `.jky`, parses case/evidence/rule/investigation
  declarations plus `call` expressions into a typed AST. The CLI `check`
  command surfaces diagnostics without executing anything.
- **Call resolver (Phase 3, built):** `include/jocky/semantic/call_resolver.hpp`
  resolves every `call` against the registry index — unknown functions,
  arity (`= default` inputs stored and filled), and parse-level type
  errors with `file:line:col` diagnostics — and plumbs each declared
  output type into its `let` binding for downstream checking. Surfaced
  via the `jocky resolve` command. Capability gating is NOT applied here.
- **Capability gate (Phase 4, current):** Type-checks pipelines,
  rejects writes to declared evidence paths, and verifies the case's
  `allowed_capabilities` cover every resolved call's declared capability
  before execution is permitted.
- **Forensic IR — FIR (Phases 1–4):** The serializable, inspectable
  execution plan: investigation name, capabilities, inputs, operations,
  writes, and the prohibited list. FIR is what `plan` shows, what policy
  validates, and what both back ends execute. Schema in
  `wiki/06-api-contracts.md`.
- **Policy validator (Phase 4):** Rejects FIRs that violate hard rules
  (evidence write, missing capability, unvalidated args, prohibited
  operation) before the resolver or runtime ever sees them.
- **Tree-shaking resolver (Phase 5):** Computes the exact stdlib closure
  for the FIR — directly called functions plus transitive `depends_on` —
  so compilations embed the minimum set. Surfaced via `--list-used`.
- **C++ runtime (Phases 6–7):** Evidence adapters (read-only
  normalization), the correlation engine (`correlate ... within ... on
  ...`), and the sandboxed script dispatcher (capability gate,
  timeout enforcement, stdout capture, SHA-256 hashing, per-execution
  manifest entries). See `wiki/03-graph-schema.md` and
  `wiki/04-ingestion-pipeline.md`.

## 2. The Two Toolchain Paths

Both paths execute the same FIR with the same capability, validation, and
manifest semantics — parity is a release requirement (Phase 8).

|                    | `jockyc` (compile)                          | `jocky` (interpret)                              |
| ------------------ | ------------------------------------------- | ------------------------------------------------ |
| Command            | `jockyc <file.jky> [-o output] [--list-used]` | `jocky <file.jky>`                             |
| Registry use       | Resolved at compile time                    | Resolved at run time from the filesystem registry |
| Scripts            | Tree-shaken closure **embedded** in the binary (Phase 6) | Loaded from disk per `call`, hash-logged per execution |
| Output             | Standalone binary, no registry needed at run time | Direct execution, no compile step |
| Manifest           | Identical schema and hash semantics         | Identical schema and hash semantics |

Supporting commands: `jocky check <file.jky>` parses and prints the AST
without executing anything; `jocky resolve <file.jky>` additionally
resolves every `call` against the registry (Phase 3, no capability
gate); `jocky verify <manifest.json>` re-hashes
inputs/outputs and checks the manifest; `scan_registry <dir>` rebuilds the
registry index from `@jocky:` headers.

## 3. Stdlib Subsystem: 48 Registry Functions and Growing

The standard library is not C++ — it is a curated corpus of shell
scripts under `stat_scripts/`, organized by domain (`recon/` 10,
`compliance/` 24, `hostforensics/` 7, `netforensics/` 6, plus
`shared/` with the testssl wrapper and the vendored `testssl.sh` it
drives): 41 curated Kalki functions renamed to
`jky_<domain>_<verb>_<object>.sh` in Phase 1, 6 greenfield
netforensics functions written directly in registry form, and 1
`shared/` wrapper (`jky_shared_run_testssl.sh`, registered as
`jky_compliance_run_testssl`). Seven
functions carry complete headers today (the 6 netforensics functions
plus the `shared/` testssl wrapper); the 41 curated functions are
header-annotated in Phase 9. `scan_registry` (built, Phase 2) indexes
headers, rejects malformed ones, and skips headerless files without
error — current verified state: 7 registered / 0 rejected / 42 skipped
(49 files total: 48 `jky_` scripts + vendored `testssl.sh`).

```sh
# @jocky:function jky_netforensics_extract_flows
# @jocky:domain netforensics
# @jocky:description Extract L3/L4 flow records from a pcap file into CSV
# @jocky:inputs pcap_path: path, bpf: string = "", out_csv: path = ""
# @jocky:outputs flows: table<flow>
# @jocky:capability netforensics.pcap.read
# @jocky:timeout_seconds 300
# @jocky:depends_on
```

Header fields: function name (must match `jky_<domain>_<verb>_<object>`,
domain in `recon, netforensics, hostforensics, timeline, compliance,
report`), description, typed inputs, typed outputs, required capability,
timeout, and `depends_on` for the tree-shaker. Full schema in
`wiki/06-api-contracts.md`. The 6 netforensics functions ship with
complete headers today and are the reference batch; the 41 curated
functions are header-annotated in Phase 9. Full triage history in
`wiki/09-kalki-inventory.md`.

The **registry scanner** (`scan_registry`, Phase 2) walks a directory,
parses these headers, builds the function index (rejecting malformed
headers and naming violations), and feeds both the semantic checker and
the resolver. Per compilation (or interpreted run), only the functions the
`.jky` file actually calls — plus their transitive dependencies — are
embedded or loaded; everything else stays out.
