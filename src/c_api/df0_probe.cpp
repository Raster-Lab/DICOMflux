// MIT; Copyright (c) 2026 Raster Images.
#include "dicomflux/df0_probe.h"
#include "../foundation/probe_logic.hpp"
#include <cstdlib>
#include <cstring>
#include <limits>
#include <new>
#include <span>
#include <type_traits>

using namespace dicomflux::probe;
struct dicomflux_probe_context {
    dicomflux_probe_options options;
    budget allocations;
    void *scratch = nullptr;
    explicit dicomflux_probe_context(const dicomflux_probe_options &o)
        : options(o), allocations(o.max_tracked_bytes) {}
};
static_assert(std::is_standard_layout_v<dicomflux_probe_options>);
static_assert(sizeof(dicomflux_probe_status) == 4);
namespace {
void *default_allocate(void *, size_t n) { return std::malloc(n); }
void default_deallocate(void *, void *p) { std::free(p); }
dicomflux_probe_status validate(const dicomflux_probe_options &o) {
    if (o.struct_size != sizeof(o) || o.abi_major != 0 || o.flags || o.reserved)
        return DF_PROBE_UNSUPPORTED;
    if (!o.max_metadata_bytes || !o.max_attributes || !o.max_bulk_bytes ||
        !o.chunk_bytes || !o.max_tracked_bytes || o.chunk_bytes > o.max_tracked_bytes ||
        o.chunk_bytes > std::numeric_limits<size_t>::max() ||
        ((o.allocate == nullptr) != (o.deallocate == nullptr))) return DF_PROBE_INVALID;
    std::uint64_t needed;
    if (!add(sizeof(dicomflux_probe_context), o.chunk_bytes, needed)) return DF_PROBE_OVERFLOW;
    if (needed > o.max_tracked_bytes) return DF_PROBE_LIMIT;
    return DF_PROBE_OK;
}
}
extern "C" {
dicomflux_probe_status dicomflux_probe_query_abi(uint32_t major, size_t size,
                                               dicomflux_probe_abi *out) {
    if (!out) return DF_PROBE_INVALID;
    std::memset(out, 0, size);
    if (major != 0 || size != sizeof(*out)) return DF_PROBE_UNSUPPORTED;
    out->struct_size = sizeof(*out);
    out->major = 0;
    out->minor = 1;
    out->pointer_bits = sizeof(void *) * 8;
    out->options_size = sizeof(dicomflux_probe_options);
    out->options_alignment = alignof(dicomflux_probe_options);
#ifdef _LIBCPP_VERSION
    out->libcxx_version = _LIBCPP_VERSION;
#endif
    return DF_PROBE_OK;
}
dicomflux_probe_status dicomflux_probe_create(const dicomflux_probe_options *input,
                                             dicomflux_probe_context **out) {
    if (!out) return DF_PROBE_INVALID;
    *out = nullptr;
    if (!input) return DF_PROBE_INVALID;
    // Only the first word is read until the supplied extent is accepted.
    if (input->struct_size != sizeof(*input)) return DF_PROBE_UNSUPPORTED;
    dicomflux_probe_options o = *input;
    const auto status = validate(o);
    if (status != DF_PROBE_OK) return status;
    if (!o.allocate) { o.allocate = default_allocate; o.deallocate = default_deallocate; }
    void *storage = nullptr;
    dicomflux_probe_context *context = nullptr;
    try {
        storage = o.allocate(o.allocator_user, sizeof(dicomflux_probe_context));
        if (!storage) return DF_PROBE_ALLOCATION;
        context = new(storage) dicomflux_probe_context(o);
        if (!context->allocations.reserve(sizeof(*context)) ||
            !context->allocations.reserve(o.chunk_bytes)) throw std::bad_alloc();
        context->scratch = o.allocate(o.allocator_user, static_cast<size_t>(o.chunk_bytes));
        if (!context->scratch) throw std::bad_alloc();
        // Exercise a C++20 library feature in the actual qualified build.
        auto bytes = std::span<std::byte>(static_cast<std::byte *>(context->scratch),
                                          static_cast<size_t>(o.chunk_bytes));
        bytes.front() = std::byte{0};
        *out = context;
        return DF_PROBE_OK;
    } catch (const std::bad_alloc &) {
        if (context) context->~dicomflux_probe_context();
        if (storage) o.deallocate(o.allocator_user, storage);
        return DF_PROBE_ALLOCATION;
    } catch (...) {
        if (context) context->~dicomflux_probe_context();
        if (storage) o.deallocate(o.allocator_user, storage);
        return DF_PROBE_INTERNAL;
    }
}
dicomflux_probe_status dicomflux_probe_measure(dicomflux_probe_context *c,
    uint64_t rows, uint64_t columns, uint64_t samples, uint64_t metadata,
    uint64_t attributes, dicomflux_probe_measurement *out) {
    if (!out) return DF_PROBE_INVALID;
    *out = {};
    if (!c || !rows || !columns || !samples) return DF_PROBE_INVALID;
    if (metadata > c->options.max_metadata_bytes || attributes > c->options.max_attributes)
        return DF_PROBE_LIMIT;
    uint64_t pixels, logical, encoded;
    if (!multiply(rows, columns, pixels) || !multiply(pixels, samples, logical) ||
        !even_length(logical, encoded)) return DF_PROBE_OVERFLOW;
    if (logical > c->options.max_bulk_bytes || encoded > UINT64_C(0xfffffffe))
        return DF_PROBE_LIMIT;
    out->logical_bytes = logical;
    out->encoded_bytes = encoded;
    out->padding_bytes = encoded - logical;
    out->tracked_bytes = c->allocations.used();
    out->peak_tracked_bytes = c->allocations.peak();
    return DF_PROBE_OK;
}
void dicomflux_probe_release(dicomflux_probe_context *c) {
    if (!c) return;
    const auto o = c->options;
    o.deallocate(o.allocator_user, c->scratch);
    c->~dicomflux_probe_context();
    o.deallocate(o.allocator_user, c);
}
}
