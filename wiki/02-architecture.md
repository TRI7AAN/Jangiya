# JOCKY Architecture

## 1. Pipeline Overview

```text
.jky source
  -> Lexer
  -> Parser (recursive descent)
  -> AST
  -> Call resolver (Phase 3, built) + case binder / capability gate
     (Phase 4, built)
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
- **Case binder + capability gate (Phase 4, built):**
  `include/jocky/semantic/case_binder.hpp` binds every program to exactly
  one `case` block (zero/duplicate cases and a case that never declared
  `allowed_capabilities` are `BindingError`s — the parser tracks field
  presence on `CaseDecl::capabilities_declared`, so "never written" and
  "written as `[]`" stay distinct). `include/jocky/policy/capability_gate.hpp`
  then fail-closed checks every resolved call's recorded capability
  against the bound case's `allowed_capabilities`, collecting ALL denials
  (function + capability + case + `file:line:col`) before refusing the
  whole compilation. Surfaced via the `jocky gate` command
  (`ALLOWED`/`DENIED` verdicts, exit 0/1). This is the static half of the
  safety contract only — Phase 7's dispatcher must independently re-check
  at run time (see `wiki/13-phase4-capability-gate.md` §5). Full Phase 4
  record in `wiki/13-phase4-capability-gate.md`.
- **Forensic IR — FIR (Phases 1–4):** The serializable, inspectable
  execution plan: investigation name, capabilities, inputs, operations,
  writes, and the prohibited list. FIR is what `plan` shows, what policy
  validates, and what both back ends execute. Schema in
  `wiki/06-api-contracts.md`.
- **Policy validator (future):** Rejects FIRs that violate hard rules
  (evidence write, missing capability, unvalidated args, prohibited
  operation) before the resolver or runtime ever sees them. Not yet built:
  Phase 4 built only the static capability gate (binder + gate over
  resolved calls, no FIR, no pipeline type-checking, no evidence-write
  rejection) — see `wiki/13-phase4-capability-gate.md` for the exact
  boundary. The `policy/` include directory now exists (home of the gate)
  for future policy checks to live beside.
- **Tree-shaking resolver (Phase 5, current):** Computes the exact stdlib closure
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
gate); `jocky gate <file.jky>` runs the full Phase 4 chain
(resolve → bind → gate) and prints the `ALLOWED`/`DENIED` authorization
verdict; `jocky verify <manifest.json>` re-hashes
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


## Phase 6 implementation status

`jockyc` now implements the static compile/package path through link. Registry
entries carry scan-time SHA-256; the embedder rechecks each shaken source and
refuses drift. The standalone Linux artifact needs no registry to inventory or
extract its exact embedded closure. Script dispatch, runtime capability checks,
evidence-path enforcement, timeouts, and manifests remain Phase 7.
