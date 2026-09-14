# JOCKY Roadmap — Full Phase Plan

> **TRACK A CLOSED 2026-09-14.** Phases 0–11 complete; final audit
> `wiki/25-final-ps-coverage.md`, signed off in `logs.md`. Remaining
> Track A work, if any, is corrective and driven by the `wiki/25` §6
> backlog — not by new roadmap phases. Track B (Phases 12–16) is the
> active track going forward if development continues.
>
> Original guidance (kept for history): this file changes rarely. It is
> full-project context, not per-session guidance. Per-session work was
> driven by `implementationplan.md` (one phase at a time).

## Phase 0: Wiki Bootstrap (this prompt)

**Goal:** Establish the documentation-only foundation for the entire project
before any compiler or runtime code is written, so every subsequent agent
session shares one vocabulary (DSL, FIR, manifest, registry, capabilities),
one safety boundary (no evasion/weaponization), and one definition of done.
**Deliverable:** All documentation files in this bootstrap exist with real
content, not placeholders — `AGENTS.md`, `roadmap.md`,
`implementationplan.md`, `logs.md`, and `wiki/00-index.md` through
`wiki/06-api-contracts.md` — confirmed by a file-tree listing showing no
file is empty.

## Phase 1: Kalki Repo Audit + JOCKY Skeleton Build from Scratch

**Goal:** Inventory the Kalki/Trinetra shell-script corpus to ground the
stdlib domain taxonomy (recon, netforensics, hostforensics, timeline,
compliance, report) in reality, and build the minimal JOCKY compiler
skeleton from scratch — lexer, AST, and recursive-descent parser with a
working CLI `check` command that accepts or rejects `.jky` files with useful
diagnostics.
**Deliverable:** Audit note plus a buildable C++20/CMake skeleton where
`jocky check <file.jky>` parses real `.jky` sources.

## Phase 2: Script Metadata Convention (@jocky: headers) + Registry Scanner

**Goal:** Define the machine-readable `@jocky:` header contract every
stdlib script must carry (function, domain, description, inputs, outputs,
capability, timeout_seconds, depends_on) and implement the registry scanner
that discovers annotated scripts on disk and builds the function index the
compiler and runtime resolve against.
**Deliverable:** Header schema doc plus a working `scan_registry <dir>`
that indexes annotated scripts and rejects malformed headers.

## Phase 3: `call` Expression Grammar Extension

**Goal:** Extend the `.jky` grammar with the `call` expression that invokes
registry functions from inside investigations, including argument binding,
return-type plumbing into pipeline operators, and parse-level arity/shape
errors.
**Deliverable:** Parser + AST support for `call` with tests showing calls
resolving to registry entries and clean errors for unknown functions or bad
arguments.

## Phase 4: Case-Level Authorization + Capability Gate + Input-Type Validation

**Goal:** Make unauthorized or unsafe execution unrepresentable: case blocks
declare `allowed_capabilities`; the semantic checker and the runtime both
enforce the capability gate; and arguments flowing into embedded/registry
shell scripts are type-validated against declared input schemas before any
process is spawned, eliminating unvalidated shell interpolation.
**Deliverable:** Checker + runtime gate with tests proving denied
capabilities refuse to run and mistyped arguments are rejected pre-execution.

## Phase 5: Dependency-Aware Tree-Shaking Resolver

**Goal:** Resolve, for any given `.jky` file, the exact closure of stdlib
scripts required — directly called functions plus transitive `depends_on` —
so compilations embed the minimum necessary set and nothing more.
**Deliverable:** Resolver that outputs the used-function list (`--list-used`)
with tests covering direct, transitive, and diamond dependencies.

## Phase 5.5: Control Flow Extension (ran between Phase 5 and Phase 6)

**Goal:** Give `.jky` C-style control flow (`if`/`else`, C-shape `for`
with statically boundable bounds, `while` flagged for runtime ceilings)
without touching any stdlib script and without breaking any static
guarantee Phases 3–5 established: the executed call set stays finite and
fully known at compile time.
**Deliverable:** Ran — lexer keywords (`if else for while int`), `Stmt`
variant AST (`IfStmt`/`ForStmt`/`WhileStmt`/`BoundExpr`), shared
`ast/walker.hpp` recursion adopted by the resolver, `bound_checker.hpp`
(three bound forms; call-derived bounds refused), bound check wired into
`resolve`/`gate`/`shake` (`check` stays parse-only), 6/6
`tests/phase5.5/` fixtures green, full Phase 3/4/5 regression with zero
drift. Record: `wiki/17-control-flow.md` (proposal analysis preserved in
`wiki/17-control-flow-proposal.md`). Phase numbering after this point is
unchanged (Phase 6 follows as planned).

## Phase 6: Script Embedding + jockyc Binary Generation

**Goal:** Implement the `jockyc` ahead-of-time path: embed the tree-shaken
script closure into the compiled artifact and link it to a standalone binary
that no longer needs the filesystem registry at run time.
**Deliverable:** `jockyc <file.jky> [-o output]` producing a runnable
standalone binary with embedded scripts verifiable by hash.

## Phase 7: Sandboxed Runtime Dispatcher + Evidence Manifest Logging

**Goal:** Build the sandboxed execution core shared by both toolchain paths:
timeout-enforced, capability-checked script dispatch with captured stdout,
exit codes, and SHA-256 hashing, where every execution — success, failure,
or timeout — appends a complete entry to the evidence integrity manifest and
no unlogged execution is possible.
**Deliverable:** Runtime dispatcher plus manifest writer with tests for
success, failure, timeout, and capability-denial entries.

## Phase 7.5: Execution-Time Control-Flow Interpreter + Manifest Corrections

**Why it exists:** the Phase 6/7 audit (`wiki/18-phase6-7-audit.md` §2.3)
found the dispatcher executing a flattened call list with no regard for
`if`/`for`/`while` structure (both branches ran, loops ran once, no
iteration ceiling anywhere despite the `wiki/17` §3 MUST). Parity testing
on top of that would compare two equally broken paths, so this
corrective phase lands before Phase 8 and Phase 8 may not proceed until
its hostile fixtures pass.
**Deliverable:** Runtime predicate evaluator + statement-tree executor
(taken-branch-only, real `for` trips, re-evaluated `while` under a hard
configurable ceiling with a distinct capped outcome) sharing one
dispatch path with flat mode; manifest schema 0.2.0 (`program_sha256`,
per-entry `start/end_utc` + `stdout_sha256`, `registry_version` flag);
`tests/phase7.5/` hostile fixtures green. Full record:
`wiki/21-phase7-5-control-flow.md`.

## Phase 8: jocky Interpreter + Compiled/Interpreted Parity Testing

**Goal:** Implement the `jocky <file.jky>` interpreted path running directly
against the filesystem registry (no compile step) and prove it is
semantically equivalent to the `jockyc` compiled path by running both
against identical sample evidence and diffing the resulting manifests.
**Deliverable:** Working interpreter plus a parity test harness showing
matching manifests for compiled vs. interpreted runs.

## Phase 9: Real Script Registry Bootstrap (Annotate Actual Kalki Scripts) + Smoke-Test Harness

**Goal:** Annotate the current registry corpus (plus greenfield
timeline/report stdlib as needed) with
valid `@jocky:` headers conforming to the `jky_<domain>_<verb>_<object>`
convention, and stand up an end-to-end smoke-test harness that compiles and
runs real investigations against sample evidence.
**Deliverable:** Annotated registry plus a green smoke-test run (check →
plan → run → verify) over sample evidence.

## Phase 10: Judge-Facing Design Rationale Document

**Goal:** Write the document a SIH judge reads: map each clause of the
literal SIH26148 PS text to what JOCKY does instead and why — showing how
the legitimate underlying need (authorized, low-footprint, fully auditable
forensic analysis) is met while the evasion/weaponization components
(polymorphic engine, BYOVD, process hollowing, domain-fronted C2) are
permanently out of scope with explicit justification.
**Deliverable:** A self-contained rationale document that passes a hostile
read against the literal PS text.

## Phase 11: Final Gap-Check Audit Against SIH26148's Literal Text

**Goal:** Perform a line-by-line audit of the finished system against every
requirement in SIH26148's literal text: confirm each legitimate forensic
requirement is demonstrably satisfied, each weaponizable requirement is
demonstrably absent (with pointers to the enforcement mechanism), and every
gap is either closed or documented with a reason.
**Deliverable:** Gap-check matrix (requirement → status → evidence/pointer)
signed off in `logs.md` as the project close-out entry.
**Status: DONE 2026-09-14 — `wiki/25-final-ps-coverage.md`; Track A closed.**

## Track B: Java IDE Shell (parallel to Track A, starts immediately)

Track B builds a Java IDE shell over the Track A CLI. It never blocks
Track A: Phase 12 starts immediately on the existing Phase 3–4 CLI, and
later phases light up richer views as Phase 6/7 artifacts exist. Track B
invokes `jocky`/`jockyc` as subprocesses and reads only their
stdout/stderr and manifest/report artifacts — it never touches evidence
directly, never re-implements gating, and never bypasses the capability
gate (all `AGENTS.md` §2 rules still apply to anything it runs).

## Phase 12: Java Shell + Editor Pane

**Goal:** Window layout mirroring VS Code (left file tree, center
editor, bottom terminal, right/bottom output panel), using JavaFX with an
embedded Monaco editor (via WebView) for real `.jky` syntax highlighting
without hand-rolling a highlighter.
**Deliverable:** Java app shell opening `.jky` files with highlighting,
file tree, and output panel placeholders.
**Depends on:** Nothing from Track A — uses Phase 3–4's existing CLI.

## Phase 13: Integrated Terminal Pane

**Goal:** Spawn `jocky`/`jockyc` as a subprocess, stream stdout/stderr
live into a terminal-style widget, support interactive re-runs without
leaving the app.
**Deliverable:** Terminal pane running real CLI commands with live
output and re-run.
**Depends on:** Phase 12.

## Phase 14: Structured Output/Monitor Panel

**Goal:** Parse `jocky gate` / `jocky shake` output (and later Phase 7's
manifest JSON) into readable panels: a findings table, a dependency tree
view for shaken scripts, colored ALLOWED/DENIED banners, and clickable
`file:line:col` diagnostics that jump the editor cursor to the error.
**Deliverable:** Monitor panel rendering gate/shake results structurally;
richer once Phase 7's manifest exists.
**Depends on:** Phase 12; richer once Phase 7's manifest exists.

## Phase 15: Manifest/Report Visualization

**Goal:** Once Phase 7 exists, render the evidence-integrity manifest as
a proper case report view (hash chain, execution log, timeline) instead
of raw JSON.
**Deliverable:** Case report view over a real Phase 7 manifest.
**Depends on:** Phase 7.

## Phase 16: Packaging

**Goal:** `jpackage` into a distributable installer/jar bundling the
Java app, invoking either a system-installed `jocky`/`jockyc` or a
bundled copy.
**Deliverable:** Installable package that runs the IDE against a real
toolchain.
**Depends on:** Phase 6 (for `jockyc`), Phase 15.
