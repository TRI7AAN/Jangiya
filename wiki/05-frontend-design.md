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
