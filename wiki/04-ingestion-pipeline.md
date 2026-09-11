# JOCKY Ingestion Pipeline — Evidence Adapters

## 1. MVP Adapters (3)

| Adapter    | Source                     | Produces                          |
| ---------- | -------------------------- | --------------------------------- |
| `pcap`     | Network capture file       | `flow`, `dns_event` tables        |
| `eventlog` | Structured JSON/CSV logs   | `log_event` table                 |
| `directory`| Filesystem metadata tree   | `file` table (metadata + hashes)  |

Each adapter follows the same three steps: **open → normalize → hand to
pipeline.** Opening is read-only (see §2); normalization converts raw
source bytes into the typed entities from `wiki/03-graph-schema.md`
(validating field types, coercing timestamps to UTC, hashing file contents
for `sha256`); only then does any query pipeline operator (`filter`,
`select`, `correlate`, `call`, `write` to a non-evidence output) run
against the normalized tables.

```text
source bytes (read-only open)
  -> adapter normalize (type check, UTC timestamps, hashes)
  -> typed entity tables (file / flow / dns_event / log_event)
  -> pipeline operators
  -> outputs (report tables, timeline_event, indicators, manifest)
```

## 2. Read-Only by Construction

**All adapters are read-only by construction; no adapter may write to its
source path.** Concretely:

- Adapters open evidence files/directories with read-only flags and never
  request write handles.
- The type checker rejects any pipeline `write` whose target resolves to a
  declared evidence input path — outputs must go to the case `out/` area.
- There is no language operator for in-place mutation of evidence; the
  prohibited list in the FIR includes evidence-write operations and the
  policy validator rejects FIRs containing them.

This is a hard rule (`AGENTS.md` §2), not an optimization: a forensic tool
that can silently alter its inputs cannot produce a trustworthy manifest.

## 3. The Separate Path: Stdlib Script Execution (Not Ingestion)

**Stdlib scripts (Kalki/Trinetra tools) are NOT evidence sources — they
are callable functions** invoked via `call` expressions:

```jky
flows = call jky_netforensics_extract_flows(pcap_path: ev.pcap, bpf: "tcp port 443")
```

Their **OUTPUT can itself become evidence fed back into a pipeline**
(the returned `table<flow>` binds to a variable and flows through
`filter`/`correlate` like adapter output) — but the mechanism is entirely
different from adapter ingestion:

|                        | Evidence ingestion (this doc, §§1–2) | Script execution (`call`) |
| ---------------------- | ------------------------------------ | ------------------------- |
| Trigger                | `evidence` declaration + adapter     | `call` expression in a rule/investigation |
| Input                  | Source path, opened read-only        | Type-validated args per `@jocky:` input schema |
| Gate                   | Evidence must be declared in `case`  | Capability must be in `allowed_capabilities`; args schema-checked; timeout enforced |
| Logging                | Input hashes in manifest `inputs[]`  | Full per-execution manifest entry (function, script hash, args, exit code, timestamps, stdout hash, timed_out) |
| Failure mode           | Adapter error aborts the run         | Recorded as failure/timeout entry — still logged, never silent |

**Document this distinction clearly because it is easy to conflate
"evidence ingestion" with "script execution."** Ingestion is passive,
read-only normalization of declared sources. Execution is active,
capability-gated, timeout-bounded invocation of code whose behavior must
be proven afterward via its manifest entry. Both feed typed tables into
pipelines; only execution requires the capability gate and per-call
logging.
