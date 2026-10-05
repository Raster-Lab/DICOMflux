# Historical DICOMflux DF-0 report

**Read publication-status.md for later findings and correction status.**

**DF-0 host subset executed; consumer qualification incomplete. Stop at DF-G0.**
No writer, parser, codec, protocol service, DF-1 implementation, commit or push
was performed. This report is engineering evidence, not gate approval or a
platform/clinical support claim.

## Source and authority

The existing Raster-Lab/DICOMflux checkout began clean on main at
`20852fdd83a6443af5e0601ba7435f7c78f2d25a`, exactly the reviewed starting revision.
Actual source path, origin, revision, working-tree state and separate private
control-package path were recorded before implementation in DF-W00. Their
unredacted record accompanies the private evidence. No existing edits needed
to be overwritten. The frozen handover remained outside Git and passed strict
verification of all 32 files before and after work. Approval, baseline, effective
register and work order were readable. DF-A001 is the operative authority.

All nine accepted architecture proposals remain unchanged. Detailed API, profile,
budget and qualification choices below are candidates for human DF-G0 review.

## Changes and observed evidence

| Work | Result | Evidence |
| --- | --- | --- |
| DF-W00 | Source/authority/provenance bound; private controls preserved | [Authority](df0-authority-and-scope.md), private DF-W00 record |
| DF-W01 | Native arm64 auxiliary host C11/C++20 install and Python witnesses passed; target/JNI work blocked | [Tool lock](toolchain-lock.json), [target matrix](target-matrix.json) |
| DF-W02 | 10 mandatory modules, 329 attribute rows, inherited macros, 15 File Meta attributes plus preamble/prefix; fixtures and oracle plan | [Profile](../profiles/README.md), [oracles](../../tests/interop/README.md) |
| DF-W03 | Declaration-only C11 API (later review found state/error gaps), lifecycle/I/O/state/error/budget proposal | [Contract](../architecture/c-api-contract.md), [candidate header](../architecture/dicomflux-c-api-candidate.h) |
| DF-W04 | Arithmetic/options/budget/state harness, allocation faults, independent sanitizers, fuzz and symbol checks executed | [Manifest](evidence-manifest.json), [resources](resource-report.json) |

The installed prototype has four C exports: ABI query, context create, arithmetic
measurement and release. It validates sizes/flags and metadata/count/bulk/chunk
budgets, checks arithmetic and reserves bounded scratch. It has no DICOM I/O.
The separate future-engine header compiles as both C11 and C++20, without any
claim that its declarations are implemented. Candidate source leases release
before terminal execution returns; single-use plans cannot execute again.

The final native run had 89 checks, fault injection at both observed allocation
sites using NULL and bad_alloc modes, and 4000 measurements across four independent
contexts. Host, AddressSanitizer, UndefinedBehaviorSanitizer and ThreadSanitizer
each passed three test executables. Two fresh clients built against a relocated
installed package and passed as C11/C++20. CPython 3.12.14 passed load/call, GC
pressure, explicit/idempotent wrapper close, invalid options and overflow checks.
Actual callback-GC/writer/JNI lifecycle tests remain unrun.

Binary inspection found exactly the four intended exports and only system libc++
and libSystem dependencies on the auxiliary macOS host. No DICOM toolkit, codec,
Python or JNI library was linked into the probe. Android private-static-runtime
packaging and ELF/page-size checks remain unqualified.

| Final libFuzzer campaign | Seed | Requested budget | Observed elapsed | Executions | Final coverage | Peak process RSS |
| --- | --- | --- | --- | --- | --- | --- |
| Short | 20261004 | 10 s | 12.154 s wall / 11 s fuzzer | 6,548,342 | cov 112; ft 122 | 152 MB reported |
| Longer | 20261005 | 30 s | 31.056 s wall / 31 s fuzzer | 18,730,346 | cov 112; ft 122 | 152 MB reported |

No crash was reported. These are bounded observations of the actual arithmetic,
budget, option and state/progress harness, not parser coverage or a memory-safety
proof. Corpus identities and exact commands are retained. Coverage counters are
reported as emitted by libFuzzer, not converted to a project-wide percentage.

The prototype tracked 65,648 bytes (112-byte context and 65,536-byte scratch)
while measuring logical lengths up to 4,294,967,294 without bulk allocation.
The corrected separate child measurement observed 2,162,688 bytes peak RSS.
RSS includes the test/runtime and is not the library budget. Numeric candidate
limits and unimplemented counters are explicit in the budget JSON. This is not
a streaming-writer performance benchmark.

Both owned raw RGB fixtures have deterministic hashes and independent sample
checks. Expected Part 10 layouts specify 1638/880 total bytes, dataset offset
348, Pixel Data value offset 870 and the odd zero pad at offset 879. No DICOM
file was generated. Inventory/layout checks are document checks. **All 24 real
engine acceptance tests remain not run.**

## Preserved failures and unresolved gates

The [discrepancy register](discrepancies.json) retains the initial wrong Ninja
path, missing Apple libFuzzer runtime, Rosetta observation and failed time/sysctl
resource wrapper. Later targeted corrections passed. The historical final suite
returned exit 1 because of that wrapper failure; it was not rewritten as success.
The resource helper now uses getrusage, with a separate successful record.

All 12 requested Pi 4/Pi 5/CM4/CM5 × Bullseye/Bookworm/Trixie combinations remain
blocked for physical qualification. Exact images, board revisions and runtime
locks are missing. CM5/Bullseye conflicts with the official Bookworm-or-newer
guidance; Pi5/Bullseye needs explicit compatible-image resolution. No Linux
host, Android toolchain, emulator or device was available. Android minimum API
is still unspecified by the user; 23 remains only the inherited candidate.

The pinned DCMTK source archive differs from the published digest surfaced by
review and is excluded from execution pending reconciliation. dciodvfy source
is pinned but unbuilt. Oracle standard coverage differs from reviewed 2026d,
and no oracle has validated an engine object. These gaps are not silently
substituted with a permissive parser. Official standards pages have local
content hashes; immutable upstream snapshot and wider dictionary reuse rights
remain explicit review items. No full standards text/dictionary or third-party
implementation is included in the candidate source package.

## Review endpoint

Review the [DF-G0 request](DF-G0-review-request.md) and candidate details. The
recommended disposition is to hold progression while resolving the named target
and provenance gates. A new human disposition/addendum must record decisions.
There is no automatic DF-1 authority, package publication or repository-operation
extension. All changes remain local and reviewable.

See [reproduction instructions](reproduce.md), [machine evidence](evidence-manifest.json),
[source hashes](build-source-files.json), [acceptance status](engine-acceptance-status.json),
and [binding plan](../architecture/binding-qualification.md). Exact commands and
unredacted path bindings are in the private evidence package; public records use
documented aliases. Hashes establish identity, not correctness or approval.
