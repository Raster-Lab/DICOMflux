/* MIT; Copyright (c) 2026 Raster Images.
 * Experimental ABI 0.1: DF-A002 foundation-only implementation.
 * Exactly five functions; no DICOM reader/writer, image or cancellation API.
 * Caller memory must be valid, naturally aligned and non-overlapping.
 * See docs/architecture/foundation-core.md for the implemented contract. */
#ifndef DICOMFLUX_FOUNDATION_H
#define DICOMFLUX_FOUNDATION_H
#include <stddef.h>
#include <stdint.h>
#if defined(_WIN32)
# if defined(DICOMFLUX_FOUNDATION_BUILD)
#  define DICOMFLUX_API __declspec(dllexport)
# else
#  define DICOMFLUX_API __declspec(dllimport)
# endif
#else
# define DICOMFLUX_API __attribute__((visibility("default")))
#endif
#ifdef __cplusplus
# define DICOMFLUX_NOEXCEPT noexcept
extern "C" {
#else
# define DICOMFLUX_NOEXCEPT
#endif
typedef int32_t dicomflux_status;
#define DICOMFLUX_OK INT32_C(0)
#define DICOMFLUX_INVALID_ARGUMENT INT32_C(1)
#define DICOMFLUX_UNSUPPORTED INT32_C(2)
#define DICOMFLUX_MALFORMED INT32_C(3)
#define DICOMFLUX_PROFILE_VIOLATION INT32_C(4)
#define DICOMFLUX_RESOURCE_LIMIT INT32_C(5)
#define DICOMFLUX_OVERFLOW INT32_C(6)
#define DICOMFLUX_IO_FAILURE INT32_C(7)
#define DICOMFLUX_CANCELLED INT32_C(8)
#define DICOMFLUX_CALLBACK_FAILURE INT32_C(9)
#define DICOMFLUX_INTERNAL_FAILURE INT32_C(10)
#define DICOMFLUX_ALLOCATION_FAILURE INT32_C(11)
#define DICOMFLUX_INVALID_STATE INT32_C(12)
#define DICOMFLUX_WOULD_BLOCK_UNSUPPORTED INT32_C(13)
#define DICOMFLUX_BUFFER_TOO_SMALL INT32_C(14)
/* Record-only sentinel; never a returned dicomflux_status. */
#define DICOMFLUX_NOT_EXECUTED INT32_C(-1)
#define DICOMFLUX_ABI_MAJOR UINT32_C(0)
#define DICOMFLUX_ABI_MINOR UINT32_C(1)
#define DICOMFLUX_COMPONENT_NONE UINT32_C(0)
#define DICOMFLUX_COMPONENT_ABI UINT32_C(1)
#define DICOMFLUX_COMPONENT_CONTEXT UINT32_C(2)
#define DICOMFLUX_COMPONENT_BUILDER UINT32_C(3)
#define DICOMFLUX_COMPONENT_DATASET UINT32_C(4)
#define DICOMFLUX_COMPONENT_PLAN UINT32_C(5)
#define DICOMFLUX_COMPONENT_SOURCE UINT32_C(6)
#define DICOMFLUX_COMPONENT_SINK UINT32_C(7)
#define DICOMFLUX_COMPONENT_CANCEL UINT32_C(8)
#define DICOMFLUX_OPERATION_NONE UINT32_C(0)
#define DICOMFLUX_OPERATION_QUERY_ABI UINT32_C(1)
#define DICOMFLUX_OPERATION_CONTEXT_CREATE UINT32_C(2)
#define DICOMFLUX_OPERATION_CAPABILITIES_COPY UINT32_C(3)
#define DICOMFLUX_OPERATION_ERROR_COPY UINT32_C(4)
#define DICOMFLUX_OPERATION_BUILDER_CREATE UINT32_C(5)
#define DICOMFLUX_OPERATION_BUILDER_SET UINT32_C(6)
#define DICOMFLUX_OPERATION_BUILDER_SET_EMPTY_SEQUENCE UINT32_C(7)
#define DICOMFLUX_OPERATION_BUILDER_FREEZE UINT32_C(8)
#define DICOMFLUX_OPERATION_PLAN_CREATE UINT32_C(9)
#define DICOMFLUX_OPERATION_PLAN_QUERY UINT32_C(10)
#define DICOMFLUX_OPERATION_CANCEL_CREATE UINT32_C(11)
#define DICOMFLUX_OPERATION_PLAN_EXECUTE UINT32_C(12)
/* Validity bits disambiguate unavailable details from legitimate zero/maxima. */
#define DICOMFLUX_ERROR_HAS_TAG UINT32_C(1)
#define DICOMFLUX_ERROR_HAS_SOURCE_OFFSET UINT32_C(2)
#define DICOMFLUX_ERROR_HAS_EXPECTED UINT32_C(4)
#define DICOMFLUX_ERROR_HAS_OBSERVED UINT32_C(8)
#define DICOMFLUX_ERROR_DETAIL_MASK UINT32_C(15)
#define DICOMFLUX_NO_TAG UINT32_MAX
#define DICOMFLUX_UNKNOWN_QUANTITY UINT64_MAX
typedef struct dicomflux_context dicomflux_context;
typedef struct dicomflux_versioned {
    uint32_t struct_size, abi_major, abi_minor, flags;
} dicomflux_versioned;
typedef struct dicomflux_abi_info {
    dicomflux_versioned header;
    uint32_t pointer_bits, native_alignment, diagnostic_capacity, reserved;
} dicomflux_abi_info;
typedef struct dicomflux_error {
    dicomflux_versioned header;
    int32_t status;
    uint32_t component, operation, tag;
    uint64_t source_offset, expected, observed;
    uint32_t diagnostic_length, truncated, detail_flags;
    char diagnostic[256];
} dicomflux_error;
typedef void *(*dicomflux_allocate_fn)(void *user, size_t bytes);
typedef void (*dicomflux_free_fn)(void *user, void *allocation);
typedef struct dicomflux_allocator {
    dicomflux_versioned header;
    void *user;
    dicomflux_allocate_fn allocate;
    dicomflux_free_fn deallocate;
} dicomflux_allocator;
typedef struct dicomflux_budgets {
    dicomflux_versioned header;
    uint64_t metadata_bytes, attributes, items, nesting_depth, bulk_bytes;
    uint64_t fragments, decoded_bytes, codec_scratch_bytes, tracked_bytes;
    uint64_t diagnostic_entries, transfer_chunk_bytes, concurrent_operations;
} dicomflux_budgets;
typedef struct dicomflux_context_options {
    dicomflux_versioned header;
    dicomflux_allocator allocator;
    dicomflux_budgets budgets;
    uint64_t reserved[4];
} dicomflux_context_options;


/* Explicit output_size must equal sizeof(dicomflux_abi_info); no header input. */
DICOMFLUX_API dicomflux_status dicomflux_query_abi(uint32_t major, uint32_t minor,
    size_t output_size, dicomflux_abi_info *output) DICOMFLUX_NOEXCEPT;
/* Borrows options during the call, copies allocator/budgets; output owns one ref.
 * error may be NULL; otherwise initialize its exact 0.1 output header first. */
DICOMFLUX_API dicomflux_status dicomflux_context_create(
    const dicomflux_context_options *options, dicomflux_context **output,
    dicomflux_error *error) DICOMFLUX_NOEXCEPT;
/* Each independently held reference owns one release. NULL is a no-op.
 * Stale/double-released/foreign pointers are caller violations. */
DICOMFLUX_API void dicomflux_context_retain(dicomflux_context *context) DICOMFLUX_NOEXCEPT;
DICOMFLUX_API void dicomflux_context_release(dicomflux_context *context) DICOMFLUX_NOEXCEPT;
/* Source is borrowed and unchanged. required is mandatory and includes NUL.
 * NULL/0 queries length. Too-small capacity writes no partial text. */
DICOMFLUX_API dicomflux_status dicomflux_error_copy(const dicomflux_error *error,
    char *output, size_t capacity, size_t *required) DICOMFLUX_NOEXCEPT;
#ifdef __cplusplus
}
#endif
#undef DICOMFLUX_NOEXCEPT
#endif
