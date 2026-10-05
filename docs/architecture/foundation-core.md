# DF-A002 implemented foundation contract

This implements DF-W05 and only five functions from DF-W06 under
[effective DF-A002 authority](../evidence/authorisations/DF-A002.json).
The broader [candidate](dicomflux-c-api-candidate.h) and its contract remain
unchanged design/history. No builder, dataset, plan, writer, public cancellation,
capabilities API or consumer integration is implemented by this slice.

## Installed interface and build

The installed header is `dicomflux/foundation.h`, shared library is
`dicomflux_foundation`, package is `DICOMfluxFoundation`, and imported target is
`DICOMflux::foundation`. The separate `DICOMfluxProbe` package and its four-function
prototype remain unchanged. The real library exports exactly:

- `dicomflux_query_abi`
- `dicomflux_context_create`
- `dicomflux_context_retain`
- `dicomflux_context_release`
- `dicomflux_error_copy`

The ABI is experimental major 0/minor 1. Status numbers, applicable record layouts,
validity bits and sentinels match the corrected candidate. No compatibility with
minor 0, packed records, foreign instances or released ABI versions is claimed.
Only natural alignment is used. Installed declarations expose implemented functions;
diagnostic category constants retain the candidate's defined numeric vocabulary,
which does not promise those other operations exist.

`share/dicomflux/foundation-capabilities.json` describes this build and the four
compiled source/header hashes. Its Git HEAD is the commit at configure time; dirty
source hashes and the pre-execution evidence manifest are the content identity.
This is partial DF-R004 build identity, not the excluded capabilities API.

## Validation and outputs

For a versioned record the first `struct_size` field must be accessible. The
implementation reads only that field before requiring the exact known size; only
then does it read the complete header or payload. A pointer that lies about its
allocation extent, is misaligned/stale/foreign, or overlaps another argument is
outside the caller contract. Controlled null arguments, null non-empty spans,
short/oversized records, flags and reserved fields are rejected. No arbitrary
pointer probing, foreign-handle registry or memory-safety claim for hostile hosts
is made.

Bad record sizes, unknown flags, nonzero reserved fields and invalid scalar/span
arguments return INVALID_ARGUMENT. Unsupported major/minor pairs return
UNSUPPORTED. Nested allocator/budget records are validated before use.
`query_abi` uses an explicit capacity equal to `sizeof(dicomflux_abi_info)` and
does not read an input header. A wrong capacity leaves output untouched. A valid
capacity with an unsupported requested version receives the known header and zero
payload. Context creation sets a valid handle-output slot to null before work.
An invalid optional error-output header is rejected without changing that record;
no option processing or allocation follows that rejection.

Successful error output is a populated OK record with NONE component/operation,
zero validity bits, absent sentinels and empty text. Failures identify the actual
entry/subsystem and only available structural quantities. Diagnostic storage is
fixed: at most 255 bytes plus NUL, length excludes NUL, truncation is 0 or 1.
Messages are implementation-authored structural text; no patient value, raw input
byte or address is included. Error construction/copy requires no allocation.

`error_copy` borrows and never mutates the source. It validates the record, defined
status/component/operation ranges, validity bits, absent sentinels and bounded
NUL-terminated text. Validity bits permit legitimate all-ones values. `required`
is mandatory; it is zero on invalid input, or diagnostic length plus one for a
valid size query/copy. NULL/0 queries; NULL with nonzero capacity is invalid.
Insufficient capacity returns BUFFER_TOO_SMALL with no partial text. A sufficient
destination receives precisely the required bytes, not a caller-sized clearing.

## Allocator, accounting and ownership

Context creation copies the allocator descriptor and budgets. Both callbacks or
neither must be supplied; neither selects malloc/free. The host owns the callbacks
and user state until the final legitimate internal/external owner releases them.
Custom storage must be max_align_t aligned; detected misalignment is returned
through the supplied free callback and reported without constructing an object.
The library never frees host buffers or returns a library-owned buffer to callers.

The context charges its own fixed allocation before invoking the allocator. Generic
internal owned blocks and internal cancellation tokens reserve tracked bytes before
allocation, roll back rejected/failed reservations, and retain the context until
their last owner releases them. Live/peak counters describe successful library-owned
allocations. Reserved admission includes in-flight requests; failed requests do not
inflate the successful-allocation peak. Deallocation finishes before removing its
charge and releasing its context reference. No allocation follows the declared
bulk payload size merely because it appears in the copied budget record.

Zero tracked budget cannot create a context. Zero future feature budgets do not
mean unlimited. Only tracked storage is consumed in this slice; metadata, attribute,
item, depth, bulk, fragment, decoded/codec, diagnostic-entry, transfer-chunk and
operation counters are copied settings for unimplemented operations, not exercised
writer/codec features. Caller-owned fixed error records need no library diagnostic
allocation. Qualification/test-case settings are not approved product limits.
Host buffers, allocator bookkeeping, runtime mappings and process RSS are excluded
from tracked storage.

Create returns one reference. Each retain requires a live independently held
reference and a corresponding release. NULL retain/release are no-ops. Stale/double
raw release is invalid; wrapper idempotence is not promised. Atomic reference
counts never wrap or resurrect zero. At the unrepresentable reference limit,
the void retain interface has no status channel: it fails closed by terminating
rather than pretending a new reference exists. Private counter tests exercise the
limit without manufacturing stale pointers. Internal RAII owners and cancellation
references demonstrate state retention without implementing builder/plan objects.

The C boundary is non-throwing. Controlled C++ bad_alloc from allocation is mapped
to ALLOCATION_FAILURE, other caught C++ allocation-callback exceptions to
CALLBACK_FAILURE. This defensive behavior does not permit arbitrary foreign
unwinding: host thunks must contain it. A throwing deallocation callback violates
the host contract and terminates rather than crossing a void release boundary.
Implementation diagnostics never recursively allocate under memory pressure.

## Arithmetic, cancellation and concurrency

Private checked operations cover uint64 addition, multiplication, even-length
padding, narrowing to unsigned capacities and int64/uint64 conversion. Failure
clears numeric outputs. Arithmetic overflow and budget exhaustion remain distinct;
format-specific field/profile rules remain future work.

Internal cancellation is a monotonic atomic request with separately retained token
and context lifetimes. A request does not complete work or release owners. No public
cancellation export, background executor or writer state machine exists. Tests race
legitimate independently held context/token references, cancellation requests and
bounded allocations, and run independent contexts. Shared custom allocators must
support the concurrency the host invokes; callbacks run synchronously on the caller
thread. Caller memory, lifetime and no-reentry/no-foreign-unwinding rules still apply.
These host results imply nothing about JNI, devices or Android packaging.
