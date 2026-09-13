# Phase 7 — Sandboxed Runtime Dispatcher and Integrity Manifest

**Completed:** 2026-09-13  
**Platform scope:** Linux/WSL, x86-64

## Result

Phase 7 adds the shared runtime in `include/jocky/runtime/dispatcher.hpp`.
`jockyc` standalone artifacts now accept `run`, construct a runtime plan
from compile-time authorized calls, and dispatch exact embedded bytes without
a live registry. `load_registry_runtime_call` gives Phase 8's interpreted
path the same dispatcher entry point while preserving the scanner's expected SHA-256;
the dispatcher logs drift as `integrity_denied` before execution.

Every call is appended to the manifest as `started` before any child can be
spawned. Immediately before dispatch the runtime independently rechecks the
case capability, concrete argument type, embedded or registry script SHA-256,
and execution ceiling. Denials and dispatcher failures update that same entry.
The manifest is atomically rewritten after each state transition.

## Isolation model

Each started call receives a new minimal root filesystem containing:

- copied `/bin/bash`, `/usr/bin/sleep`, and their x86-64 runtime libraries;
- the exact hash-checked script at `/script.sh`;
- copied path inputs under `/inputs`, separate from original evidence;
- a private `/outputs` staging tree; and
- empty `/work` and `/tmp` directories.

The child enters user, network, and PID namespaces, then `chroot`s into that
root. It cannot name host evidence or output paths. Path arguments are
canonicalized before translation. Inputs become snapshot paths; output-root
destinations become staging paths. After exit, JOCKY re-hashes every input
snapshot. A changed or removed snapshot produces `evidence_write_denied`,
discards outputs, and leaves original evidence unchanged. Only successful,
integrity-clean staged outputs are promoted to canonical destinations inside
the authorized output root. Symlink artifacts and output-root escapes fail
closed.

This copy-and-promote design is used because this WSL kernel permits
unprivileged user/network/PID namespaces and `chroot`, but rejects
user-namespace bind mounts. The rejected bind attempt returned `EINVAL`;
the runtime does not downgrade to unrestricted execution.

The minimal root deliberately includes only Bash and `sleep` in Phase 7.
A registry function that requires another forensic executable fails with a
captured nonzero exit and a complete manifest entry. Phase 9 expands the
curated runtime tool set alongside real-script smoke tests. Network egress is
absent inside the private network namespace.

## Runtime behavior

The generated artifact supports:

```text
standalone --list-embedded
standalone --extract <function>
standalone run [--output-root dir] [--manifest path] [--max-executions n]
```

Arguments are emitted by the compiler in registry schema order and passed as
individual `argv` entries. No shell command string is constructed. Literal
`$(...)`, spaces, and punctuation remain data.

Each child has a wall timeout. It starts in its own process group; timeout
sends termination and then kill to the whole group. `stdout`, `stderr`,
exit code, elapsed milliseconds, timeout state, capability, function, script
hash, arguments, and final outcome are recorded.

The runtime ceiling bounds dispatched call attempts. Phase 7's generated plan
contains statically authorized direct calls in source order; it does not yet
evaluate table pipelines or choose runtime `if`/loop paths. Phase 8 owns full
interpreted execution and compiled/interpreted semantic parity. Until then,
the ceiling is enforced by the shared dispatcher and dynamic arguments fail
`validation_denied` instead of being guessed.

## Manifest contract

The Phase 7 manifest contains:

- source `.jky` SHA-256 and case authorization;
- canonical evidence paths, adapter/name, byte count, and deterministic
  SHA-256 (directory digests hash the sorted relative-file inventory);
- one execution entry for every attempt, including all denial outcomes;
- embedded or filesystem script SHA-256;
- captured output and process result; and
- promoted output paths, byte counts, and SHA-256.

The writer publishes via a same-directory temporary file followed by rename.
A manifest creation or publication error throws and the run is failed; the
dispatcher never starts a child before the initial manifest exists.

## Persisted verification evidence

The focused run created **11 manifests** with **11 execution entries**:
three `success` entries, two `integrity_denied` entries, and one each
for `failure`, `timeout`, `capability_denied`,
`evidence_write_denied`, `validation_denied`, and
`ceiling_denied`. The output-escape case was
refused during preflight and correctly had zero execution entries. Together
with the registry-deleted standalone integration, the final evidence set
contained **12 manifests** and **12 execution entries**.

| Artifact | SHA-256 | Bytes |
| --- | --- | ---: |
| Focused `.jky` fixture text | `3007e84c72fbc75808dd6d09f4eea107a736d08da19d7b17cfdf63717a6b2a11` | — |
| Original evidence | `62b0c0767a0d936528c5e12e349eb572c57ff271930d84715d43ad1e4599d279` | 18 |
| Literal-argv output | `0f96a470bcfbda465bb5b187531295eb9fbf8a9de476f805ff4832aa67fcb3a6` | 42 |
| Standalone integration `.jky` | `587156763823f8bb198452beafb48871177c150350afc0e4cc3918dec4e3b2e5` | — |
| Embedded pair-top script | `e4a3014d718d8f2d011a74a3ea220a55916d2031041ed455194d43aade04974c` | — |

| Focused script | Manifest SHA-256 |
| --- | --- |
| success | `36797b8a43232c77caeff1519dd8251dccb83c4c99e5733dba4a90fdb36bd405` |
| filesystem registry | `d49fbc92e177287456530117498fb0e67bac21ba2965c631ec3ac56392bd4643` |
| stale filesystem registry actual bytes | `38ff4b2a8be887c6187802c3c9c070706eae3c10fddb41a53a03640c7a857233` |
| nonzero failure | `ba3c641b93bf98a525c1aba7cfe3de81aca0551b9f1888d6f06a8b98848ffbc2` |
| timeout | `4c60f13e4796a203664458421e0ce56f52eb608f948f6eabe94d4b117e6bb26b` |
| capability denial / invalid-arg body | `a52ec387d3751059aca642cae4e3c52f93589c012ee58225f5fe6cd6fc80f4cd` |
| evidence-write attempt | `c6994d9549c751e42d4fdbbb1fadc579626e7ec5ee6fd96fbd5837bf4af8ab94` |
| ceiling scripts / integrity fixture actual bytes | `c87c7a722d59a266b300850637a1cb77b46cc67c5add8a66aa9e79e198955808` |
| integrity fixture deliberately declared value | `0000000000000000000000000000000000000000000000000000000000000000` |

Five targets (`jocky`, `scan_registry`, `jockyc`, Phase 6 embedder test,
Phase 7 runtime test) compiled with
`-std=c++20 -Wall -Wextra -Wpedantic` without warnings. CMake remains
unavailable on this WSL image, so direct compiler commands and both registered
shell integration harnesses were used.

The historical plus current matrix executed **29 checks**: sample 1; Phase 3
6; Phase 4 6; Phase 5 5; Phase 5.5 6; registry 1; Phase 6 unit/integration 2;
Phase 7 focused/integration 2. All returned their expected status. A temporary
runner footer initially printed the hard-coded label `31`; counting the
actually emitted command rows exposed the label error, and the persisted
verified total is 29.

## Definition-of-done map

- Runtime capability denial without spawn: complete.
- Validated argv delivery without interpolation: complete.
- Evidence mutation and canonical output escape refusal: complete.
- Success, failure, timeout, integrity, validation, capability, evidence, and
  ceiling outcomes in complete manifests: complete.
- Process-group timeout cleanup and maximum execution count: complete.
- Source/evidence/script/output reproducible SHA-256 records: complete.
- Embedded execution after registry deletion: complete.
- Shared filesystem-registry adapter: complete.
