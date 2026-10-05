# DF-0 C API and resource contract candidate

Status: proposed detail for DF-G0 review. The adjacent candidate header compiles
as C11 and C++20 but none of its engine operations is implemented. The installed
`df0_probe.h` library has a separate, four-symbol prototype interface. It measures
lengths and allocation limits; it cannot read or write DICOM.

## Version and caller-memory rules

The candidate starts at experimental ABI major 0, minor 1. Call `query_abi`
before creating any handle. This candidate accepts only major 0, minor 1,
including every nested record; other major/minor pairs return `UNSUPPORTED`.
It does not promise minor 0 compatibility. Only natural platform alignment is
used; there are no packed structures or wire-structure casts. Integer fields
are fixed width except in-process spans, which use `size_t`. Numeric VR codes
contain the two ASCII characters, independent of host byte order.

For minor 1, every input/output record uses exactly its declared size; flags
and reserved fields are zero. Short, oversized and unknown-option records are
rejected. There are no defaultable trailing fields in this revision. A later
append-only minor needs an explicit minimum-size/default table and new tests;
extra bytes are not silently ignored. Nested records follow the same rule.
Callers initialize each output header; functions validate it before zeroing
the payload. `query_abi` instead takes an explicit writable byte capacity.

Optional error output may be null; other required outputs may not. Handle
outputs are set to null before work; valid result payloads are zero on failure
except documented partial-execution counters and the bounded error record.
On an invalid output header, the function writes no payload. Invalid memory,
misalignment, stale handles, foreign-instance handles, overlapping input/output
spans, or a capacity larger than the actual allocation are caller-contract
violations; native C cannot validate arbitrary pointers. Null non-empty spans
and ordinary invalid scalar options produce controlled statuses.

Valid live handles from the same loaded library but different contexts are
rejected with `INVALID_ARGUMENT` by `plan_create` (context/dataset mismatch)
and `plan_execute` (plan/cancellation-token mismatch), before claiming the plan
or calling source/sink callbacks. This is distinct from a foreign-instance or
stale pointer, which is a caller violation. No cross-context sharing is implied.

## Fixed-width state and diagnostic values

These definitions revise only the unimplemented candidate, not the installed
four-symbol probe ABI. No compatibility with the earlier candidate layout is
claimed. Storage uses uint32_t or int32_t, never compiler-sized C enums.

| Plan constant | Value | Meaning |
| --- | --- | --- |
| PLAN_NONE | 0 | No execution was claimed by this call; never a live plan's state. |
| PLAN_READY | 1 | Created and not yet consumed; query execution_status is NOT_EXECUTED (-1). |
| PLAN_EXECUTING | 2 | Internal execution state; querying concurrently is forbidden. |
| PLAN_SUCCEEDED | 3 | Consumed successfully; query execution_status is OK. |
| PLAN_FAILED | 4 | Consumed with an error other than cancellation. |
| PLAN_CANCELLED | 5 | Consumed by cancellation; query execution_status is CANCELLED. |

Names in these tables have the `DICOMFLUX_` prefix. `plan_query` returns state,
execution_status and the immutable planned lengths in plan_info. Query is allowed
only before execution or after it returns. There is no concurrent polling promise.
For failed/cancelled plans, execution_status retains the terminal status. An
attempt to execute a terminal plan returns INVALID_STATE without changing that
stored status. A caller must synchronize query with execute.

When valid output headers were supplied, execute sets result.status to its return
status. A rejection before claiming the plan sets terminal_state=PLAN_NONE and
all result counters to zero; the ready/terminal plan is unchanged. Once claimed,
every return has terminal_state SUCCEEDED, FAILED or CANCELLED, with its status
and valid partial counters. Pre-requested cancellation consumes a ready plan,
returns CANCELLED with no I/O and still releases its source lease before return.
NOT_EXECUTED is only a plan_info sentinel, never a function return or terminal
result. Unknown states are reserved, never silently interpreted as success.

| Error component | Value | Failure responsibility |
| --- | --- | --- |
| COMPONENT_NONE | 0 | No error (status OK). |
| COMPONENT_ABI | 1 | Version, size, flag or ABI record validation. |
| COMPONENT_CONTEXT | 2 | Context allocator or budget admission. |
| COMPONENT_BUILDER | 3 | Mutable metadata input/validation. |
| COMPONENT_DATASET | 4 | Frozen metadata/profile validation. |
| COMPONENT_PLAN | 5 | Planning, length/state or plan argument validation. |
| COMPONENT_SOURCE | 6 | Source retain/read/release protocol. |
| COMPONENT_SINK | 7 | Sink write/progress protocol. |
| COMPONENT_CANCEL | 8 | Cancellation token/request. |

`operation` identifies the API entry that returned, even if a callback failed.
Component identifies the failing subsystem; argument errors use that entry's
subsystem, version/record errors use ABI, source and sink failures use their
respective components. A cross-context cancellation mismatch uses CANCEL.

| Operation suffix (OPERATION_) | Value | Entry |
| --- | --- | --- |
| NONE | 0 | No error |
| QUERY_ABI | 1 | query_abi (no error record parameter) |
| CONTEXT_CREATE | 2 | context_create |
| CAPABILITIES_COPY | 3 | capabilities_copy |
| ERROR_COPY | 4 | error_copy (does not mutate the supplied error) |
| BUILDER_CREATE | 5 | builder_create |
| BUILDER_SET | 6 | builder_set |
| BUILDER_SET_EMPTY_SEQUENCE | 7 | builder_set_empty_sequence |
| BUILDER_FREEZE | 8 | builder_freeze |
| PLAN_CREATE | 9 | plan_create |
| PLAN_QUERY | 10 | plan_query |
| CANCEL_CREATE | 11 | cancel_create |
| PLAN_EXECUTE | 12 | plan_execute |

Void retain/release/request entries report no error record. Component values
above 8 and operation values above 12 are reserved in this candidate. Unknown
input values fail validation; output producers emit only defined values.

`detail_flags` has HAS_TAG=1, HAS_SOURCE_OFFSET=2, HAS_EXPECTED=4 and
HAS_OBSERVED=8 (mask 15). Absent tag is NO_TAG (UINT32_MAX); absent offset or
expected/observed quantity is UNKNOWN_QUANTITY (UINT64_MAX). Each presence bit
is authoritative, so a legitimate UINT64_MAX remains representable when its bit
is set. A zeroed error payload is not a populated error record. For a valid error
output, success produces status OK, component/operation NONE, zero detail_flags,
the absent sentinels, and empty diagnostic. Failures populate status/component/
operation and only available details; validation never dereferences invalid memory
to fill a diagnostic. An invalid error-output header is rejected before work and
left untouched. The diagnostic contains at most 255 bytes plus a NUL;
diagnostic_length excludes NUL, and truncated is exactly 0 or 1. Error-copy's
required size is diagnostic_length+1. Reserved bits must be zero. No raw values
or patient data appear in diagnostic text.

## Ownership and lifecycle

| Object or argument | Retention and release rule |
| --- | --- |
| Context | Create returns one external reference. Children keep the allocation/budget state alive. Each retain requires one release. |
| Allocator | Context copies callbacks and user pointer. The host keeps them and the user object alive until the last child releases. Pair both callbacks or neither; default is the library allocator. Returned storage is `max_align_t` aligned. |
| Builder | One mutable owner; copies bytes in `builder_set` before return. Duplicate tags fail. Freeze returns an immutable dataset and seals the builder. Further sets/freezes fail; caller still releases the builder. |
| Dataset | Immutable, reference-counted, retains context. No mutable host metadata survives in it. |
| Source | Plan creation copies its descriptor and calls retain exactly once after validation. A failed retain creates no ownership; a successful retain is paired with release on rollback, unexecuted plan destruction, or before terminal execution returns. An executed plan never retains the source owner after return. |
| Plan | Immutable metadata, a source lease until terminal completion, reference-counted, exactly one execution attempt. Retains dataset/context and source. Execution holds an additional internal reference until return. |
| Sink | Descriptor and user object borrowed for synchronous execution only; no durable-store or atomic-file promise. |
| Cancellation token | Reference-counted atomic request flag; request does not establish completion. Execute retains it until terminal return. A null token disables cancellation. |
| Error/result | Caller-owned; no foreign free. Diagnostic text holds structural context only, never supplied patient values or raw bytes. |

Release accepts null. Releasing the same raw pointer twice is invalid; idempotent
close belongs in wrappers. No library-owned buffer is returned to callers.
`capabilities_copy` and `error_copy` use `required` including the final NUL: a
null buffer with zero capacity performs a length query; too-small capacity
returns `BUFFER_TOO_SMALL`, writes no partial text, and sets required. A
sufficient buffer receives the entire string and terminator. Capabilities list
implemented operations by direction, profile, syntax, build and ABI identity;
the prototype must never list writer capability.

## Threading and exceptions

No worker pool or scheduler. A caller runs synchronous execution on its worker;
callbacks run on that thread. Builders and probe contexts are single-threaded.
Independent contexts may run concurrently. For the proposed engine, immutable
dataset sharing and reference counts must be tested before being advertised.
Only cancellation request and retain/release of an independently held reference
may race execution. Plan state query is allowed before/after execution, not
during it. No callback re-enters a mutable engine operation.

Every future C/JNI entry catches allocation and other C++ exceptions and returns
the corresponding bounded error. Host callback thunks must contain exceptions
and return an error; foreign unwinding is forbidden. Diagnostics are fixed
storage so allocation failure cannot recursively allocate a message. Native
status codes distinguish invalid data, unsupported capability, profile error,
overflow, budget exhaustion, allocation failure, callback failure, I/O failure,
cancellation and internal failure.

## Values and profile

`builder_set` accepts canonical unpadded ASCII bytes for text VRs, little-endian
binary values for numeric VRs, and explicit length. It never normalizes UIDs,
converts character sets, or silently truncates values. Only VRs enumerated by
the reviewed profile inventory are candidates for DF-1. Empty Type 2 sequences
use the dedicated setter. Non-empty sequences, arbitrary dictionary lookup,
unknown transfer syntaxes, codecs, video and networking are unsupported.
Pixel data is supplied only by the retained source at plan creation, not as a
builder value. The actual wire encoder remains DF-1 work.

## Plan and I/O state tables

| State | Allowed transition and observation |
| --- | --- |
| Builder | Validate, freeze dataset, create plan. Any error creates no plan and calls no sink. |
| Ready plan | One execute claims the plan atomically. A pre-requested cancellation consumes it and returns cancelled without I/O. |
| Executing | Advance bounded source/sink chunks; finish as succeeded, failed or cancelled. |
| Terminal | All execute attempts return invalid state; no reads/writes occur. Release retained objects when references reach zero. |

The single-use state model is tested in DF-0. Its results do not demonstrate a
writer, concurrent plan execution, actual callback lifetimes or wrapper races.
Validation before claiming a plan includes supported profile/TS, all lengths,
budget admission and stable known source length. The caller guarantees source
bytes are immutable; retaining a file descriptor alone does not ensure that.

| Callback result | Cursor and terminal behavior |
| --- | --- |
| OK with positive count | Count must not exceed request. Advance once and continue until the planned count is reached. |
| Short positive progress | Retry only the unconsumed portion; never replay accepted sink bytes. |
| Zero progress with bytes remaining | Fail immediately. No polling or buffering loop. |
| Source EOF | May accompany the last positive bytes if the declared total is exactly reached. Earlier EOF fails. Never read after the declared region. |
| Sink EOF | Invalid for a sink; fails after accounting valid partial progress. |
| Error with positive count | Account the accepted count once, then fail. Partial output is unsuccessful. |
| Would block | Account any valid partial progress, then return unsupported backpressure. No resumable plan in DF-1. |
| Cancelled | Account valid partial progress; terminal cancelled. |
| Count larger than request, unknown status or reserved data | Callback failure; do not accept an invalid count. |

No source or sink callback, including source-owner release, follows terminal
execution return. The source lease is released exactly once before returning
success, failure or cancellation; final plan destruction cannot release it again.
If a ready plan is never executed, its final release performs owner release
synchronously before returning. Allocator cleanup for separately retained objects
is tied to their later explicit release, outside the completed I/O operation.
A callback that never returns prevents any hard cancellation
deadline; the host must supply cooperative timeouts or process isolation.

Lengths are distinct: source logical bytes exclude padding; encoded Pixel Data
includes at most one zero pad byte; total object size includes all headers and
metadata. Native finite even OB length must be at most `0xfffffffe`. Check each
DICOM field separately from uint64 arithmetic and size_t conversion. Undefined
length is not an oversized-native-value escape.

## Budget configuration and acceptance

`qualification-budgets.json` holds the candidate settings. Zero disables the
corresponding feature; it never means unlimited. The no-codec prototype reserves
context plus one transfer chunk before allocating, includes transient owned
storage, and rejects arithmetic/limit violations without allocating bulk bytes.
It does not implement every future budget counter. Allocation peak excludes
allocator bookkeeping, host buffers, platform mappings, runtimes and process RSS.

DF-0 checks cover known option sizes/flags, checked arithmetic boundaries,
allocation failure at both observed allocation sites, counter rollback,
independent contexts, short/zero/partial I/O models and single-use state models.
Future DF-1 acceptance must additionally cover actual retained callbacks,
writer preflight/no-output, error-copy capacity, cancellation races, stale
wrapper tokens, full profile validation and real output/oracle comparisons.
All those engine acceptance tests remain not run at DF-G0 submission.
