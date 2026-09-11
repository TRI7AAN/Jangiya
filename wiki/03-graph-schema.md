# JOCKY Graph Schema — Evidence Entities & Correlation Model

This project's closest analog to a graph schema is its **forensic entity
and correlation model**: typed evidence entities plus the query-time joins
that link them into timeline graphs. There is no separate graph DDL — the
entity types below are the schema, enforced by adapters at ingestion and by
the type checker in pipelines.

## 1. Entity Types

### file

Filesystem metadata record (from the `directory` adapter, read-only).

| Field     | Type     | Notes                          |
| --------- | -------- | ------------------------------ |
| path      | path     | Absolute or case-relative path |
| size      | int      | Bytes                          |
| sha256    | string   | Content hash                   |
| mtime     | datetime | Modification time (UTC)        |
| ctime     | datetime | Metadata-change time (UTC)     |
| mode      | string   | Permission bits (e.g. `0644`)  |
| owner     | string   | Owner name/uid                 |
| magic     | string   | File-type identification       |

### flow

Network flow record (from the `pcap` adapter or `call` outputs).

| Field      | Type     | Notes                        |
| ---------- | -------- | ---------------------------- |
| src_ip     | ip       | Source address               |
| dst_ip     | ip       | Destination address          |
| src_port   | int      | Source port                  |
| dst_port   | int      | Destination port             |
| protocol   | string   | e.g. `tcp`, `udp`, `icmp`    |
| bytes      | int      | Total bytes in flow          |
| packets    | int      | Total packets in flow        |
| start_time | datetime | Flow start (UTC)             |
| end_time   | datetime | Flow end (UTC)               |

### dns_event

DNS query event (from `pcap` or structured logs).

| Field      | Type     | Notes                     |
| ---------- | -------- | ------------------------- |
| src_ip     | ip       | Querier address           |
| query_name | string   | Queried name              |
| query_type | string   | e.g. `A`, `AAAA`, `TXT`   |
| timestamp  | datetime | Event time (UTC)          |
| rcode      | string   | Response code, if known   |

### log_event

Structured log record (from the `eventlog` adapter over JSON/CSV).

| Field     | Type     | Notes                                   |
| --------- | -------- | --------------------------------------- |
| timestamp | datetime | Event time (UTC)                        |
| host      | string   | Hostname                                |
| user      | string   | Account, empty if not applicable        |
| channel   | string   | e.g. `syslog`, `Security`, `auth`       |
| event_id  | string   | Provider event code                     |
| message   | string   | Raw message text                        |
| process   | string   | Process/image name, if present          |

### timeline_event

Output of a `correlate` operation — a node in the query-time graph.

| Field      | Type     | Notes                                    |
| ---------- | -------- | ---------------------------------------- |
| timestamp  | datetime | Ordering key (UTC)                       |
| host       | string   | Correlated host, if applicable           |
| entity_ref | string   | Opaque reference to a source entity      |
| kind       | string   | Source kind (`file`, `flow`, …)          |
| summary    | string   | Human-readable one-line description      |

### indicator

Analyst- or script-produced finding.

| Field      | Type     | Notes                              |
| ---------- | -------- | ---------------------------------- |
| kind       | string   | e.g. `hash`, `ip`, `domain`, `yara` |
| value      | string   | Indicator value                    |
| confidence | string   | `low` / `medium` / `high`          |
| source     | string   | Producing rule or function         |

## 2. Correlation Model: Query-Time Graph

Entities are linked with the `correlate` operator:

```jky
correlate(flows, dns) within 5m on src_ip
correlate(logons, flows) within 10m on host, src_ip
```

Semantics: join two entity tables into `timeline_event` nodes where the
listed `<fields>` are equal and timestamps fall within `<duration>` of
each other. Edges are defined by **shared field values within a time
window** (e.g., `host`, `src_ip`) — not by stored pointers.

This is deliberately a **query-time correlation graph, not a stored one
for the MVP**: nothing is persisted between operations except the
pipeline tables and the final manifest. That keeps the runtime stateless,
auditable, and demo-sized. The correlation engine sorts by timestamp,
window-joins on the key fields, and emits ordered `timeline_event`
records ready for the report stage.

## 3. Open Question (Post-MVP)

**Whether a persistent graph store is worth adding post-MVP** — e.g., for
multi-case correlation across investigations (shared indicators, recurring
infrastructure). Candidates would need to preserve the hard guarantees
(read-only evidence, manifest-logged everything, capability gating) while
adding cross-run entity resolution and retention policy. Deferred until
after the hackathon demo; the query-time model above is sufficient for
Phases 1–9.
