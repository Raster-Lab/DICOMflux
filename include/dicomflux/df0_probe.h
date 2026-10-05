#ifndef DICOMFLUX_DF0_PROBE_H
#define DICOMFLUX_DF0_PROBE_H

/* MIT; Copyright (c) 2026 Raster Images.
 * DF-0 qualification prototype only. No DICOM reading/writing capability. */
#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32)
# if defined(DICOMFLUX_PROBE_BUILD)
#  define DF_PROBE_API __declspec(dllexport)
# else
#  define DF_PROBE_API __declspec(dllimport)
# endif
#else
# define DF_PROBE_API __attribute__((visibility("default")))
#endif
#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t dicomflux_probe_status;
#define DF_PROBE_OK INT32_C(0)
#define DF_PROBE_INVALID INT32_C(1)
#define DF_PROBE_UNSUPPORTED INT32_C(2)
#define DF_PROBE_LIMIT INT32_C(3)
#define DF_PROBE_OVERFLOW INT32_C(4)
#define DF_PROBE_ALLOCATION INT32_C(5)
#define DF_PROBE_INTERNAL INT32_C(6)

typedef void *(*dicomflux_probe_allocate)(void *user, size_t bytes);
typedef void (*dicomflux_probe_deallocate)(void *user, void *allocation);
typedef struct dicomflux_probe_context dicomflux_probe_context;

typedef struct dicomflux_probe_abi {
    uint32_t struct_size;
    uint32_t major;
    uint32_t minor;
    uint32_t pointer_bits;
    uint32_t options_size;
    uint32_t options_alignment;
    uint32_t libcxx_version;
    uint32_t reserved;
} dicomflux_probe_abi;

typedef struct dicomflux_probe_options {
    uint32_t struct_size;
    uint32_t abi_major;
    uint32_t flags;
    uint32_t reserved;
    uint64_t max_metadata_bytes;
    uint64_t max_attributes;
    uint64_t max_bulk_bytes;
    uint64_t max_tracked_bytes;
    uint64_t chunk_bytes;
    void *allocator_user;
    dicomflux_probe_allocate allocate;
    dicomflux_probe_deallocate deallocate;
} dicomflux_probe_options;

typedef struct dicomflux_probe_measurement {
    uint64_t logical_bytes;
    uint64_t encoded_bytes;
    uint64_t padding_bytes;
    uint64_t tracked_bytes;
    uint64_t peak_tracked_bytes;
} dicomflux_probe_measurement;

/* Output spans must be valid writable caller memory. query zeroes out_size bytes.
 * Options require the exact DF-0 size; append-only compatibility is not claimed.
 * Allocators return max_align_t-aligned storage; callbacks must not throw.
 * release(NULL) is valid. A stale pointer or a second release is caller UB. */
DF_PROBE_API dicomflux_probe_status dicomflux_probe_query_abi(
    uint32_t requested_major, size_t out_size, dicomflux_probe_abi *out);
DF_PROBE_API dicomflux_probe_status dicomflux_probe_create(
    const dicomflux_probe_options *options, dicomflux_probe_context **out);
DF_PROBE_API dicomflux_probe_status dicomflux_probe_measure(
    dicomflux_probe_context *context, uint64_t rows, uint64_t columns,
    uint64_t samples, uint64_t metadata_bytes, uint64_t attributes,
    dicomflux_probe_measurement *out);
DF_PROBE_API void dicomflux_probe_release(dicomflux_probe_context *context);

#ifdef __cplusplus
}
#endif
#endif
