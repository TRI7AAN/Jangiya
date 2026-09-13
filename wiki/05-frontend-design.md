# JOCKY Frontend Design — Reporting & Dashboard

## 1. MVP: Static Report, No Live Server

For the hackathon demo the runtime generates a **static Markdown/HTML
report directly** — no live server, no build step, no frontend framework
required:

- Output path: `out/incident_report.md` (and optionally `.html` rendered
  from the same model).
- Contents: case metadata, evidence sources with hash verification status,
  correlated timeline view, high-risk findings (indicators), and the full
  script-execution manifest entries.
- Generation is a `report`-domain concern: a `jky_report_*` stdlib
  function and/or a pipeline `write report(...)` operator renders the
  normalized tables + manifest into the static file as the last step of
  the run.

This keeps the demo air-gapped-friendly and reviewable: the report file
itself is hash-logged in the manifest `outputs[]`, so a judge can verify
the report they are reading is the report the run produced.

## 2. Stretch Goal: Small Dashboard

If time permits post-MVP, a small React or fully static dashboard over the
manifest + report model:

- **Evidence sources panel:** declared inputs, adapter used, SHA-256,
  verification status (re-hash match / mismatch).
- **Hash verification status:** green/red per input and output, tied to
  `jocky verify` results.
- **Timeline view:** ordered `timeline_event` nodes from `correlate`
  operations, filterable by host / entity kind / time window.
- **High-risk findings:** `indicator` table sorted by confidence, each
  linked back to its producing rule or function.
- **Script-execution manifest entries:** per-`call` rows (function, args,
  exit code, duration, timed_out, stdout hash) proving every execution is
  accounted for.

The dashboard reads only the manifest and report artifacts — it never
touches evidence directly and never re-executes anything.

## 3. Demo Narrative: check → plan → run → verify

The live demo is a four-beat story, each beat visible on screen, ending on
the manifest proving integrity:

1. **`check`** — `jocky check case.jky`: the program is well-formed,
   capabilities declared, evidence paths resolve. Diagnostics on failure.
2. **`plan`** — show the FIR / `--list-used`: exactly which evidence,
   which operations, which tree-shaken scripts will run. Nothing hidden.
3. **`run`** — `jockyc case.jky -o case.bin && ./case.bin` (or `jocky
   case.jky` for the interpreted twin): adapters normalize, calls execute
   under the capability gate, the static report lands in `out/`.
4. **`verify`** — `jocky verify manifest.json`: re-hashes inputs/outputs,
   replays manifest entries, prints PASS. The closing slide is the
   manifest itself — the integrity proof is the product.

## 4. Track B: Java IDE Shell (Phases 12–16, parallel to Track A)

The IDE shell is a Java/JavaFX app over the Track A CLI — same demo
story as §3, but hosted in a VS Code-style window instead of a bare
terminal. It starts immediately on the existing CLI and gains richer
views as Phase 6/7 artifacts land. It invokes `jocky`/`jockyc` as
subprocesses and reads only their stdout/stderr plus manifest/report
artifacts: it never touches evidence directly, never re-executes
anything itself, and never bypasses the capability gate (all `AGENTS.md`
§2 rules apply to anything it runs).

| Phase | Focus | Depends on |
| ----- | ----- | ---------- |
| 12 | Java shell + editor pane — window layout mirroring VS Code (left file tree, center editor, bottom terminal, right/bottom output panel), using JavaFX with an embedded Monaco editor (via WebView) for real .jky syntax highlighting without hand-rolling a highlighter | Nothing from Track A — can start immediately, using Phase 3–4's existing CLI |
| 13 | Integrated terminal pane — spawn jocky/jockyc as a subprocess, stream stdout/stderr live into a terminal-style widget, support interactive re-runs without leaving the app | Phase 12 |
| 14 | Structured output/monitor panel — parse jocky gate/jocky shake output (and later Phase 7's manifest JSON) into readable panels: a findings table, a dependency tree view for shaken scripts, colored ALLOWED/DENIED banners, and clickable file:line:col diagnostics that jump the editor cursor to the error | Phase 12; richer once Phase 7's manifest exists |
| 15 | Manifest/report visualization — once Phase 7 exists, render the evidence-integrity manifest as a proper case report view (hash chain, execution log, timeline) instead of raw JSON | Phase 7 |
| 16 | Packaging — jpackage into a distributable installer/jar bundling the Java app, invoking either a system-installed jocky/jockyc or a bundled copy | Phase 6 (for jockyc), Phase 15 |

Plan of record for these phases: `roadmap.md` Phases 12–16. Track A
phases (5–11) are unchanged.
