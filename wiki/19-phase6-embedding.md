# Phase 6 — Script Embedding and `jockyc`

**Completed:** 2026-09-12  
**Platform scope:** Linux/WSL

## Result

Phase 6 adds an ahead-of-time `jockyc` target. It runs the existing static
chain in order:

`parse → resolve → bound-check → bind → gate → shake → embed → link`

The embedder consumes `ShakeResult::scripts` directly, preserving Phase 5's
deduplicated topological order. It does not repeat dependency discovery.

The generated binary contains the exact script bytes, function names, byte
lengths, and SHA-256 digests. It can list its inventory and extract a named
script byte-for-byte. It does not execute scripts; dispatch, timeouts, runtime
capability checks, and manifests belong to Phase 7.

## Freshness boundary

The registry scanner now stores `ScriptMetadata::source_sha256` when it
indexes a valid entry. Immediately before generation, `embed_scripts` reads
the source again, recomputes SHA-256, and refuses if the value differs. This
closes the scan-to-embed drift gap identified in `wiki/16`.

SHA-256 is implemented locally in `crypto/sha256.hpp` with no crypto-library
or command dependency. Known vectors for the empty string and `abc` are
tested.

## CLI

```text
jockyc <file.jky> [-o output] [--registry dir] [--list-used]
```

- Default output is `./a.out`.
- `--registry` selects a registry for fixtures or non-default deployments.
- `--list-used` prints the ordered function/digest list and exits without
  generating or linking.
- The host compiler is `c++`, or the single executable path in `JOCKY_CXX`.
  It is invoked with an argv vector through `fork`/`execvp`, never a shell.
- The standalone artifact accepts `--list-embedded` and
  `--extract <function>`.

## Persisted digest evidence

The real registry scan remained **7 registered, 0 rejected, 42 skipped**.

| Function/file | SHA-256 |
| --- | --- |
| `jky_netforensics_check_dns_anomalies` | `8cc0992dc1887a58f1317087f1612f5d2beb88eb874601982551019f88ed972b` |
| `jky_netforensics_detect_beaconing` | `0096bc8803d9653394c4e342aa4dec13b4184b68599ae5359efbdcc8b7972e8c` |
| `jky_netforensics_extract_dns` | `4bce4ce76156e93c6017cfd397cac6ec81a1b3862008af43e81495c45953ac02` |
| `jky_netforensics_extract_flows` | `be7d17fd98292bad0f900d677d29442113471261338c92984c5496107b4f78ea` |
| `jky_netforensics_extract_http` | `54afd01e7129987756e58a5f4bb1e64cdeae6096eaffd8867fcf9a31c4e14f1b` |
| `jky_netforensics_top_talkers` | `ed0383d364a71ecc16151346679fe97e1ce2c1060d9c0836db436bb02063d269` |
| `jky_compliance_run_testssl` wrapper | `38e2d66d594b7b0ef552ebdece678f1b002a20dafdab2124409c6758cce3e65a` |
| Phase 6 pair leaf fixture | `f907ccdfabb50c77cefa02f2125e1631c7c414243a6b51cf296c3506e41abade` |
| Phase 6 pair top fixture | `e4a3014d718d8f2d011a74a3ea220a55916d2031041ed455194d43aade04974c` |
| Stale test, scan-time bytes | `306c6ca7407560340797866e077e053627ad409277d1b9da58106fce4cf717cb` |
| Stale test, changed bytes | `3ca7d7af7229b50f7f79188c24052f3b51a7e44972533d7ad6d6ef055daa7b8b` |
| SHA-256 empty-string vector | `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855` |
| SHA-256 `abc` vector | `ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad` |

## Verification

CMake was unavailable in the active Ubuntu environment, so the three production
targets and the unit test were compiled directly with Ubuntu g++ 13.3.0,
`-std=c++20 -Wall -Wextra -Wpedantic`. They compiled without warnings.

The Phase 6 checks proved:

- known-vector SHA-256 correctness;
- scan-then-mutate source refusal;
- exact `leaf → top` embedded order matching `--list-used`;
- byte-for-byte extraction of the embedded leaf;
- digest agreement with `sha256sum`;
- standalone execution after its temporary registry was deleted;
- denied capabilities produce no output artifact;
- a valid no-call program generates a standard-compliant zero-script artifact.

The full regression matrix was unchanged: sample check 0; Phase 3
`0,1,1,0,1,1`; Phase 4 `0,1,1,1,1,1`; Phase 5 `0,0,0,1,0`; Phase 5.5
resolve `0,0,0,1,0,0`; registry scan 7/0/42. Shell syntax and
`git diff --check` passed.

## Definition-of-done map

- Standalone exact closure: complete.
- Embed-time hashing and drift refusal: complete.
- Denied gate cannot reach embedding: complete.
- Linux/WSL scope: explicit here and in the implementation.
- Registry-independent artifact test: complete.
- Phase advanced to Phase 7: complete.
