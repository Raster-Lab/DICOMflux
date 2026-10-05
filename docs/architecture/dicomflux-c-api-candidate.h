/* MIT; Copyright (c) 2026 Raster Images.
 * DF-G0 DESIGN CANDIDATE. Declarations only; NOT implemented or exported.
 * See c-api-contract.md for lifecycle, version, pointer and callback rules. */
#ifndef DICOMFLUX_C_API_CANDIDATE_H
#define DICOMFLUX_C_API_CANDIDATE_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
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
typedef struct dicomflux_context dicomflux_context;
typedef struct dicomflux_builder dicomflux_builder;
typedef struct dicomflux_dataset dicomflux_dataset;
typedef struct dicomflux_plan dicomflux_plan;
typedef struct dicomflux_cancel_token dicomflux_cancel_token;
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
    uint32_t diagnostic_length, truncated;
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

/* Numeric VR codes are the two ASCII bytes: e.g. UI = 0x5549. No C enums. */
typedef struct dicomflux_value {
    dicomflux_versioned header;
    uint32_t tag, vr;
    const uint8_t *bytes;
    size_t byte_count;
    uint64_t reserved[2];
} dicomflux_value;
#define DICOMFLUX_IO_OK INT32_C(0)
#define DICOMFLUX_IO_EOF INT32_C(1)
#define DICOMFLUX_IO_ERROR INT32_C(2)
#define DICOMFLUX_IO_WOULD_BLOCK INT32_C(3)
#define DICOMFLUX_IO_CANCELLED INT32_C(4)
typedef struct dicomflux_io_result {
    int32_t status;
    uint32_t reserved;
    size_t count;
} dicomflux_io_result;
typedef dicomflux_status (*dicomflux_owner_retain_fn)(void *user);
typedef void (*dicomflux_owner_release_fn)(void *user);
typedef dicomflux_io_result (*dicomflux_read_fn)(void *user, uint8_t *destination, size_t capacity);
typedef dicomflux_io_result (*dicomflux_write_fn)(void *user, const uint8_t *source, size_t length);
typedef struct dicomflux_source {
    dicomflux_versioned header;
    void *user;
    uint64_t logical_length;
    dicomflux_read_fn read;
    dicomflux_owner_retain_fn retain;
    dicomflux_owner_release_fn release;
    uint64_t reserved[2];
} dicomflux_source;
typedef struct dicomflux_sink {
    dicomflux_versioned header;
    void *user;
    dicomflux_write_fn write;
    uint64_t reserved[2];
} dicomflux_sink;
typedef struct dicomflux_write_options {
    dicomflux_versioned header;
    uint32_t profile, transfer_syntax;
    dicomflux_source source;
    uint64_t reserved[4];
} dicomflux_write_options;
#define DICOMFLUX_PROFILE_PHOTO_RGB8_V1 UINT32_C(1)
#define DICOMFLUX_TS_EXPLICIT_VR_LITTLE_ENDIAN UINT32_C(1)
typedef struct dicomflux_plan_info {
    dicomflux_versioned header;
    uint64_t logical_bulk_bytes, encoded_bulk_bytes, padding_bytes, object_bytes;
} dicomflux_plan_info;
typedef struct dicomflux_execution_result {
    dicomflux_versioned header;
    int32_t status;
    uint32_t terminal_state;
    uint64_t source_consumed, sink_consumed, planned_object_bytes;
    uint64_t peak_tracked_bytes;
} dicomflux_execution_result;

dicomflux_status dicomflux_query_abi(uint32_t major, uint32_t minor,
                                    size_t output_size, dicomflux_abi_info *output);
dicomflux_status dicomflux_context_create(const dicomflux_context_options *options,
                                         dicomflux_context **output, dicomflux_error *error);
void dicomflux_context_retain(dicomflux_context *context);
void dicomflux_context_release(dicomflux_context *context);
dicomflux_status dicomflux_capabilities_copy(dicomflux_context *context,
    char *output, size_t capacity, size_t *required, dicomflux_error *error);
dicomflux_status dicomflux_error_copy(const dicomflux_error *error,
    char *output, size_t capacity, size_t *required);
dicomflux_status dicomflux_builder_create(dicomflux_context *context,
    dicomflux_builder **output, dicomflux_error *error);
dicomflux_status dicomflux_builder_set(dicomflux_builder *builder,
    const dicomflux_value *value, dicomflux_error *error);
dicomflux_status dicomflux_builder_set_empty_sequence(dicomflux_builder *builder,
    uint32_t tag, dicomflux_error *error);
dicomflux_status dicomflux_builder_freeze(dicomflux_builder *builder,
    dicomflux_dataset **output, dicomflux_error *error);
void dicomflux_builder_release(dicomflux_builder *builder);
void dicomflux_dataset_retain(dicomflux_dataset *dataset);
void dicomflux_dataset_release(dicomflux_dataset *dataset);
dicomflux_status dicomflux_plan_create(dicomflux_context *context,
    const dicomflux_dataset *dataset, const dicomflux_write_options *options,
    dicomflux_plan **output, dicomflux_error *error);
dicomflux_status dicomflux_plan_query(const dicomflux_plan *plan,
    dicomflux_plan_info *output, dicomflux_error *error);
void dicomflux_plan_retain(dicomflux_plan *plan);
void dicomflux_plan_release(dicomflux_plan *plan);
dicomflux_status dicomflux_cancel_create(dicomflux_context *context,
    dicomflux_cancel_token **output, dicomflux_error *error);
void dicomflux_cancel_request(dicomflux_cancel_token *token);
void dicomflux_cancel_retain(dicomflux_cancel_token *token);
void dicomflux_cancel_release(dicomflux_cancel_token *token);
dicomflux_status dicomflux_plan_execute(dicomflux_plan *plan,
    const dicomflux_sink *sink, dicomflux_cancel_token *cancel,
    dicomflux_execution_result *result, dicomflux_error *error);
#ifdef __cplusplus
}
#endif
#endif
