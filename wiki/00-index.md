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
| [13-phase4-capability-gate.md](13-phase4-capability-gate.md) | Phase 4 record: `capabilities_declared` AST correction, exactly-one-case binder, separate-pass capability gate (fail-closed, all denials), `jocky gate`, layering rationale, Phase 7 forward note, case-metadata gap flag, six fixture runs. |
| [14-phase3-verification.md](14-phase3-verification.md) | Retroactive persistence of the chat-only verification session, re-verified live: clean build, frozen check AST, 7/0/42 registry with SKIP-set identity, 6/6 fixtures, quarantine re-check, shell `$?` self-correction note. |
| [16-phase5-tree-shaking.md](16-phase5-tree-shaking.md) | Phase 5 record: `shake` naming decision, dependency-closure resolver (topo order, alphabetical tiebreak, cycle/full-path errors), throwaway 12-script registry, five fixture runs verbatim, Phase 6 handoff note. |
| [17-control-flow-proposal.md](17-control-flow-proposal.md) | PROPOSAL (undecided, unimplemented): C-style `if`/`else`, bounded `repeat`, flag variables in `.jky` with scripts untouched — static-enumerability trade-off, Path 1 (bounded) vs Path 2 (Turing-complete), recommendation awaiting human confirm. |
| [17-control-flow.md](17-control-flow.md) | Phase 5.5 record: C-style `if`/`for`/`while` in `.jky`, three statically boundable `for` forms, shared AST walker, `requires_runtime_ceiling` Phase 7 commitment, both-branches-gated decision, six fixture runs. |
| [18-project-alignment-audit.md](18-project-alignment-audit.md) | Whole-project SIH26148 alignment audit: verified implementation state, literal-PS mapping, gaps, and recommended order. |
| [19-phase6-embedding.md](19-phase6-embedding.md) | Phase 6 record: scan/embed SHA-256 freshness, exact closure packaging, `jockyc`, standalone artifact behavior, digests, and verification. |
| [grammar.md](grammar.md) | Authoritative `.jky` syntax reference with usage: lexical rules, all four declarations, heads, predicates, all ten pipeline operators, correlate windows, types, control flow, per-command validation, verified examples. |
| [keywords.md](keywords.md) | Reserved-word roll: all 150 keywords (42 `.jky` + 38 C + 52 C++ + 18 Java), usage table, deliberate exclusions, reservation record. |
