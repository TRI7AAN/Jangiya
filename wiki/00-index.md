# JOCKY Wiki — Index

Start here. The wiki is the design source of truth; code follows it, not the
other way around. Per `AGENTS.md`, no compiler/runtime code lands without
updating the relevant wiki doc.

| File | One-line summary |
| ---- | ---------------- |
| [01-prd.md](01-prd.md) | Product requirements: why the literal SIH26148 PS is reframed as authorized, auditable forensics with evasion/weaponization permanently out of scope. |
| [02-architecture.md](02-architecture.md) | Full compiler/runtime pipeline from `.jky` source to evidence bundle + manifest, covering both the `jockyc` compile and `jocky` interpret paths. |
| [03-graph-schema.md](03-graph-schema.md) | Evidence entity types, their fields, and the query-time `correlate` graph model (no persistent graph store for the MVP). |
| [04-ingestion-pipeline.md](04-ingestion-pipeline.md) | Read-only evidence adapter design (pcap, eventlog, directory) and how it differs from stdlib script execution. |
| [05-frontend-design.md](05-frontend-design.md) | Reporting layer: MVP static Markdown/HTML report, stretch dashboard, and the check → plan → run → verify demo narrative. |
| [06-api-contracts.md](06-api-contracts.md) | Internal contracts: CLI surface, FIR JSON schema, manifest schema, `@jocky:` header schema, and `.jky` grammar EBNF. |
| [07-research.md](07-research.md) | Research notes: PS provenance, NTRO customer, SIH scoring, tool landscape, format-to-schema grounding, integrity standards, 65B admissibility, exclusion cases, and per-phase implementation precedents. |
| [08-ps-analysis.md](08-ps-analysis.md) | Full-text SIH26148 analysis: background/description/expected-solution dissection, per-technique research capsules, D3FEND/CERT-In/DPDP closings, draft gap-check matrix, harvested changes, and the graded residual backlog. |
| [09-kalki-inventory.md](09-kalki-inventory.md) | Phase 1 audit and registry bootstrap: script triage, src/ verdict, prune/rename/netforensics/quarantine records, and Phase 2 handoff. |
| [10-phase1-verification.md](10-phase1-verification.md) | Read-only Phase 1 checkpoint: build evidence, src/ confirmation, count reconciliation, domain/naming audits, root-file inspection, wiki drift check. |
| [11-phase2-drift-check.md](11-phase2-drift-check.md) | Read-only Phase 2 drift checkpoint: build/AST/registry/quarantine/git/grammar re-verification with first quarantine SHA-256 baseline. |
| [12-phase3-resolution.md](12-phase3-resolution.md) | Phase 3 record: `= default` decision, call resolver rules, six fixture runs verbatim, scanner default-validation checks, regression evidence. |
