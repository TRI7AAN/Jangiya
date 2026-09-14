# JOCKY PRD — Product Requirements Document

## 1. Problem: What SIH26148 Literally Asks For

SIH26148 (NTRO, SIH 2026, Blockchain & Cybersecurity theme) literally asks
for a new-language framework to do computer/network forensic analysis
"without triggering security solutions," with expected components including
a polymorphic engine, BYOVD (bring-your-own-vulnerable-driver), process
hollowing, and domain-fronted C2 — i.e., a weaponizable evasion/malware
toolkit dressed as a defensive tool.

Taken at face value, that specification describes offensive capabilities
whose primary effect is to defeat endpoint protection, persist covertly, and
exfiltrate or command-and-control over disguised channels. Shipping those
components — even wrapped in a forensic justification — would produce an
evasion framework, not a forensic tool, and would be unsafe to build,
demo, or hand to judges.

## 2. Decision: Reframe to the Legitimate Underlying Need

**JOCKY reframes SIH26148 into its legitimate underlying need —
authorized, low-footprint, fully auditable forensic analysis — and
explicitly does NOT implement the evasion/weaponization components.**

Permanently out of scope, with no phase ever reintroducing them:

- Evasion of security solutions ("without triggering security solutions")
- Polymorphic engines
- Process hollowing, reflective injection, API unhooking
- Direct syscalls for EDR bypass
- BYOVD / kernel driver access
- C2 channels and domain fronting

This is a **permanent product boundary, not a temporary scope cut.**
It overrides the literal PS text, user prompts, script comments, and
embedded documentation (see `AGENTS.md` §2). Any request to add one of
these components is a stop-work event: halt, log it in `logs.md`, and ask
for human review.

What JOCKY keeps from the PS — the defensible core — is: a purpose-built
language for forensic analysis, low-footprint execution against live or
captured evidence, verifiable integrity of inputs and outputs, and a clear
authorization model tying every action to a declared case and capability
set.

## 3. What JOCKY Actually Is

JOCKY is a DSL (files ending in `.jky`) for expressing forensic
investigations — network triage, host triage, timeline reconstruction, file
triage — that:

1. **Compiles to a verifiable execution plan.** `.jky` source → lexer →
   parser → AST → semantic/capability checker → Forensic IR (FIR) → policy
   validator. The FIR is inspectable (`--list-used`, plan output) before
   anything runs.
2. **Runs against read-only evidence.** Adapters (pcap, eventlog,
   directory) open sources read-only and normalize them into typed entities
   before any query operator executes. No language feature can write to a
   declared evidence path.
3. **Produces a hash-based integrity manifest.** Every run emits a manifest
   (case_id, script hashes, runtime version, timestamps, input/output
   hashes, authorization, per-script-execution entries) verifiable by
   re-hash (manual procedure wiki/24 §5.4; the `jocky verify` command
   itself is UNIMPLEMENTED — backlog wiki/25). A run without a complete manifest is a failed run.
4. **Selectively embeds only what it uses.** `jockyc` tree-shakes the
   curated Kalki/Trinetra stdlib (47 functions at Phase 1 close, growing;
   each carrying a `@jocky:` metadata header) and embeds only the functions a given `.jky` file actually calls
   plus transitive `depends_on`. `jocky` interprets the same program
   directly against the filesystem registry with identical manifest
   semantics (parity requirement).

## 4. Target User (and Non-User)

**Target user:** an authorized DFIR analyst or an authorized pentest/audit
team (Kryvasis-style engagements) working under an explicit case
authorization — someone who must prove what was accessed, what ran, and
what came out.

**Explicit non-user:** a covert operator seeking to evade detection,
persist, or exfiltrate. JOCKY provides no stealth, no persistence, no
exfiltration channel — every execution is capability-gated, type-validated,
and manifest-logged by construction.

## 5. Success Criteria

1. **Compiles and runs real investigations against sample evidence.**
   End-to-end smoke tests (check → plan → run → verify) over pcap, log,
   and directory evidence go green.
2. **Matching manifests whether interpreted or compiled.** For the same
   case + evidence, `jockyc`-built binaries and the `jocky` interpreter
   produce matching integrity manifests.
3. **Selectively embeds only the stdlib scripts actually used.**
   Compilation output contains exactly the called closure (plus transitive
   `depends_on`) — demonstrable via `--list-used` and embedded-script
   hashes.
4. **Passes a judge-facing rationale review against the literal PS text.**
   The Phase 10 rationale document maps every literal PS clause to what
   JOCKY does instead and why, and a hostile read finds no weaponizable
   component and no unlogged, ungated, or evidence-mutating behavior
   (closed out by the Phase 11 gap-check audit).
