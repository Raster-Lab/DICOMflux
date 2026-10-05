// MIT; Copyright (c) 2026 Raster Images. Owned DF-A002 bounded hostile-input harness.
#include "core.hpp"
#include <cstdlib>
#include <cstring>
#include <limits>
namespace core = dicomflux::core;
static void require(bool condition) { if (!condition) std::abort(); }
struct Allocations {
    unsigned calls{}, frees{}, successful{}, fail{};
    static void* allocate(void* u, size_t bytes) {
        auto& a = *static_cast<Allocations*>(u); ++a.calls;
        if (a.calls == a.fail) return nullptr;
        require(bytes <= 4096); void* p = std::malloc(bytes);
        if (p) ++a.successful;
        return p;
    }
    static void deallocate(void* u, void* p) { ++static_cast<Allocations*>(u)->frees; std::free(p); }
};
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size > 96) return 0;
    uint64_t a = 0, b = 0;
    if (size >= 8) std::memcpy(&a, data, 8);
    if (size >= 16) std::memcpy(&b, data + 8, 8);
    const uint8_t selector = size > 16 ? data[16] : 0;
    uint64_t result = 0;
    const bool added = core::add(a, b, result);
    require(added ? result >= a && result >= b && result - a == b : result == 0);
    const bool multiplied = core::multiply(a, b, result);
    require(multiplied ? (b == 0 ? result == 0 : result / b == a) : result == 0);
    const bool padded = core::even_length(a, result);
    require(padded ? result >= a && result - a <= 1 && result % 2 == 0 : a == UINT64_MAX);
    uint32_t narrow = 0; const bool narrowed = core::narrow(a, narrow);
    require(narrowed == (a <= UINT32_MAX));
    int64_t signed_value = 0; require(core::to_signed(a, signed_value) == (a <= uint64_t{INT64_MAX}));

    struct { dicomflux_context_options options; unsigned char extra[16]; } storage{};
    auto& o = storage.options;
    o.header = {sizeof(o), 0, 1, 0};
    o.allocator.header = {sizeof(o.allocator), 0, 1, 0};
    o.budgets.header = {sizeof(o.budgets), 0, 1, 0};
    o.budgets.tracked_bytes = b % 4097;
    o.budgets.bulk_bytes = a; // Does not cause payload allocation.
    Allocations allocations; allocations.fail = size > 17 ? data[17] % 4 : 0;
    o.allocator.user = &allocations; o.allocator.allocate = Allocations::allocate; o.allocator.deallocate = Allocations::deallocate;
    auto* h = selector % 3 == 0 ? &o.header : selector % 3 == 1 ? &o.allocator.header : &o.budgets.header;
    switch ((selector / 3) % 6) {
    case 1: h->struct_size = 4; break;
    case 2: h->struct_size += 4; break;
    case 3: h->abi_minor = 0; break;
    case 4: h->flags = 1; break;
    case 5: o.reserved[selector % 4] = a | 1; break;
    default: break;
    }
    dicomflux_error error{}; error.header = {sizeof(error), 0, 1, 0};
    dicomflux_context* ctx = nullptr;
    const auto status = dicomflux_context_create(&o, &ctx, &error);
    require(status == error.status && error.diagnostic_length < 256 && error.diagnostic[error.diagnostic_length] == 0);
    if (status == DICOMFLUX_OK) {
        require(ctx != nullptr); dicomflux_context_retain(ctx); dicomflux_context_release(ctx);
        core::OwnedBlock* block = nullptr;
        const auto bs = core::OwnedBlock::create(ctx, a, block);
        require((bs == DICOMFLUX_OK) == (block != nullptr));
        if (block) { require(block->payload_bytes <= 4096); if (block->payload_bytes) block->data()[0] = selector; block->destroy(); }
        core::Cancellation* token = nullptr;
        if (core::Cancellation::create(ctx, token) == DICOMFLUX_OK) {
            token->retain(); token->request(); require(token->requested()); token->release(); token->release();
        }
        require(ctx->memory.reserved() == sizeof(*ctx) && ctx->memory.live() == sizeof(*ctx));
        dicomflux_context_release(ctx);
    } else require(ctx == nullptr);
    require(allocations.frees == allocations.successful);
    const auto snapshot = error;
    size_t needed = 0;
    require(dicomflux_error_copy(&error, nullptr, 0, &needed) == DICOMFLUX_OK);
    char buffer[257]; std::memset(buffer, 'Z', sizeof(buffer));
    const size_t capacity = size > 18 ? data[18] : 0;
    const auto copied = dicomflux_error_copy(&error, buffer, capacity, &needed);
    require(copied == (capacity < needed ? DICOMFLUX_BUFFER_TOO_SMALL : DICOMFLUX_OK));
    require(buffer[256] == 'Z' && std::memcmp(&snapshot, &error, sizeof(error)) == 0);
    if (copied == DICOMFLUX_BUFFER_TOO_SMALL) require(buffer[0] == 'Z');
    error.detail_flags |= UINT32_C(0x80000000);
    require(dicomflux_error_copy(&error, nullptr, 0, &needed) == DICOMFLUX_INVALID_ARGUMENT && needed == 0);
    return 0;
}
