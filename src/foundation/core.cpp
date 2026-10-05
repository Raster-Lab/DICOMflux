// MIT; Copyright (c) 2026 Raster Images.
#include "core.hpp"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <new>

namespace dicomflux::core {
Validation validate_record(const void* record, uint32_t size) noexcept {
    if (!record) return {DICOMFLUX_INVALID_ARGUMENT};
    uint32_t supplied = 0;
    // Only this field must be accessible before the extent check succeeds.
    std::memcpy(&supplied, record, sizeof(supplied));
    constexpr auto both = DICOMFLUX_ERROR_HAS_EXPECTED | DICOMFLUX_ERROR_HAS_OBSERVED;
    if (supplied != size) return {DICOMFLUX_INVALID_ARGUMENT, size, supplied, both};
    dicomflux_versioned header{};
    std::memcpy(&header, record, sizeof(header));
    if (header.abi_major != DICOMFLUX_ABI_MAJOR || header.abi_minor != DICOMFLUX_ABI_MINOR)
        return {DICOMFLUX_UNSUPPORTED, (uint64_t{DICOMFLUX_ABI_MAJOR} << 32) | DICOMFLUX_ABI_MINOR,
                (uint64_t{header.abi_major} << 32) | header.abi_minor, both};
    if (header.flags) return {DICOMFLUX_INVALID_ARGUMENT, 0, header.flags, both};
    return {};
}

void set_error(dicomflux_error* error, dicomflux_status status, uint32_t component,
               uint32_t operation, std::string_view message, Validation detail) noexcept {
    if (!error) return;
    dicomflux_error result{};
    result.header = {sizeof(result), DICOMFLUX_ABI_MAJOR, DICOMFLUX_ABI_MINOR, 0};
    result.status = status;
    result.tag = DICOMFLUX_NO_TAG;
    result.source_offset = DICOMFLUX_UNKNOWN_QUANTITY;
    result.expected = DICOMFLUX_UNKNOWN_QUANTITY;
    result.observed = DICOMFLUX_UNKNOWN_QUANTITY;
    if (status != DICOMFLUX_OK) {
        result.component = component; result.operation = operation;
        result.detail_flags = detail.details;
        if (detail.details & DICOMFLUX_ERROR_HAS_EXPECTED) result.expected = detail.expected;
        if (detail.details & DICOMFLUX_ERROR_HAS_OBSERVED) result.observed = detail.observed;
        const size_t length = std::min(message.size(), sizeof(result.diagnostic) - 1);
        if (length) std::memcpy(result.diagnostic, message.data(), length);
        result.diagnostic_length = static_cast<uint32_t>(length);
        result.truncated = message.size() > length ? 1U : 0U;
    }
    // Fixed local object, never a memset using caller-controlled struct_size.
    *error = result;
}

dicomflux_status validate_error(const dicomflux_error* error) noexcept {
    auto result = validate_record(error, sizeof(dicomflux_error));
    if (result.status != DICOMFLUX_OK) return result.status;
    if (error->status < DICOMFLUX_OK || error->status > DICOMFLUX_BUFFER_TOO_SMALL ||
        error->component > DICOMFLUX_COMPONENT_CANCEL ||
        error->operation > DICOMFLUX_OPERATION_PLAN_EXECUTE ||
        error->detail_flags & ~DICOMFLUX_ERROR_DETAIL_MASK || error->truncated > 1 ||
        error->diagnostic_length >= sizeof(error->diagnostic)) return DICOMFLUX_INVALID_ARGUMENT;
    if ((!(error->detail_flags & DICOMFLUX_ERROR_HAS_TAG) && error->tag != DICOMFLUX_NO_TAG) ||
        (!(error->detail_flags & DICOMFLUX_ERROR_HAS_SOURCE_OFFSET) && error->source_offset != DICOMFLUX_UNKNOWN_QUANTITY) ||
        (!(error->detail_flags & DICOMFLUX_ERROR_HAS_EXPECTED) && error->expected != DICOMFLUX_UNKNOWN_QUANTITY) ||
        (!(error->detail_flags & DICOMFLUX_ERROR_HAS_OBSERVED) && error->observed != DICOMFLUX_UNKNOWN_QUANTITY))
        return DICOMFLUX_INVALID_ARGUMENT;
    if (error->diagnostic[error->diagnostic_length] != '\0' ||
        std::memchr(error->diagnostic, '\0', error->diagnostic_length)) return DICOMFLUX_INVALID_ARGUMENT;
    if (error->status == DICOMFLUX_OK) {
        if (error->component || error->operation || error->detail_flags || error->diagnostic_length || error->truncated)
            return DICOMFLUX_INVALID_ARGUMENT;
    } else if (!error->component || !error->operation) return DICOMFLUX_INVALID_ARGUMENT;
    return DICOMFLUX_OK;
}

bool References::try_retain() noexcept {
    auto current = count_.load(std::memory_order_relaxed);
    while (current && current != UINT64_MAX) {
        if (count_.compare_exchange_weak(current, current + 1, std::memory_order_relaxed)) return true;
    }
    return false;
}
References::Released References::release() noexcept {
    auto current = count_.load(std::memory_order_relaxed);
    while (current) {
        if (count_.compare_exchange_weak(current, current - 1, std::memory_order_acq_rel,
                                        std::memory_order_relaxed))
            return current == 1 ? Released::last : Released::alive;
    }
    return Released::invalid;
}

dicomflux_status Budget::reserve(uint64_t bytes) noexcept {
    auto current = reserved_.load(std::memory_order_relaxed);
    do {
        uint64_t desired = 0;
        if (!add(current, bytes, desired)) return DICOMFLUX_OVERFLOW;
        if (desired > limit_) return DICOMFLUX_RESOURCE_LIMIT;
        if (reserved_.compare_exchange_weak(current, desired, std::memory_order_relaxed)) return DICOMFLUX_OK;
    } while (true);
}
void Budget::rollback(uint64_t bytes) noexcept { reserved_.fetch_sub(bytes, std::memory_order_relaxed); }
void Budget::commit(uint64_t bytes) noexcept {
    const uint64_t live = live_.fetch_add(bytes, std::memory_order_relaxed) + bytes;
    auto peak = peak_.load(std::memory_order_relaxed);
    while (peak < live && !peak_.compare_exchange_weak(peak, live, std::memory_order_relaxed)) {}
}
void Budget::free(uint64_t bytes) noexcept {
    live_.fetch_sub(bytes, std::memory_order_relaxed);
    rollback(bytes);
}

dicomflux_status Allocator::get(size_t bytes, void*& output) const noexcept {
    output = nullptr;
    try {
        output = allocate ? allocate(user, bytes) : std::malloc(bytes);
    } catch (const std::bad_alloc&) { return DICOMFLUX_ALLOCATION_FAILURE; }
      catch (...) { return DICOMFLUX_CALLBACK_FAILURE; }
    if (!output) return DICOMFLUX_ALLOCATION_FAILURE;
    if (reinterpret_cast<uintptr_t>(output) % alignof(std::max_align_t)) {
        put(output); output = nullptr; return DICOMFLUX_INVALID_ARGUMENT;
    }
    return DICOMFLUX_OK;
}
void Allocator::put(void* allocation) const noexcept {
    try {
        if (deallocate) deallocate(user, allocation); else std::free(allocation);
    } catch (...) {
        // No error channel exists during release. A throwing host free violates
        // its contract; fail closed rather than unwind across a C boundary.
        std::terminate();
    }
}

dicomflux_status create(const dicomflux_context_options& options, dicomflux_context*& out) noexcept {
    out = nullptr;
    if (options.budgets.tracked_bytes < sizeof(dicomflux_context)) return DICOMFLUX_RESOURCE_LIMIT;
    Allocator allocator{options.allocator.user, options.allocator.allocate, options.allocator.deallocate};
    void* storage = nullptr;
    auto status = allocator.get(sizeof(dicomflux_context), storage);
    if (status != DICOMFLUX_OK) return status;
    out = ::new (storage) dicomflux_context(allocator, options.budgets);
    return DICOMFLUX_OK;
}
void retain(dicomflux_context* context) noexcept {
    if (context && !context->references.try_retain()) std::terminate();
}
void release(dicomflux_context* context) noexcept {
    if (!context) return;
    const auto result = context->references.release();
    if (result == References::Released::invalid) std::terminate();
    if (result == References::Released::last) {
        const auto allocator = context->allocator;
        context->~dicomflux_context();
        allocator.put(context);
    }
}

static dicomflux_status allocate_owned(dicomflux_context* owner, size_t bytes, void*& storage) noexcept {
    storage = nullptr;
    auto status = owner->memory.reserve(bytes);
    if (status != DICOMFLUX_OK) return status;
    status = owner->allocator.get(bytes, storage);
    if (status != DICOMFLUX_OK) { owner->memory.rollback(bytes); return status; }
    owner->memory.commit(bytes);
    retain(owner);
    return DICOMFLUX_OK;
}
static void free_owned(dicomflux_context* owner, void* storage, size_t bytes) noexcept {
    // Keep the reservation until the successful allocation is actually freed.
    owner->allocator.put(storage);
    owner->memory.free(bytes);
    release(owner);
}
dicomflux_status OwnedBlock::create(dicomflux_context* owner, uint64_t bytes, OwnedBlock*& out) noexcept {
    out = nullptr;
    if (!owner) return DICOMFLUX_INVALID_ARGUMENT;
    uint64_t total = 0; size_t allocation = 0;
    if (!add(sizeof(OwnedBlock), bytes, total) || !narrow(total, allocation)) return DICOMFLUX_OVERFLOW;
    void* storage = nullptr;
    const auto status = allocate_owned(owner, allocation, storage);
    if (status != DICOMFLUX_OK) return status;
    out = ::new (storage) OwnedBlock{owner, allocation, bytes};
    return DICOMFLUX_OK;
}
void OwnedBlock::destroy() noexcept {
    auto* context = owner; const auto bytes = allocation_bytes;
    this->~OwnedBlock(); free_owned(context, this, bytes);
}
dicomflux_status Cancellation::create(dicomflux_context* owner, Cancellation*& out) noexcept {
    out = nullptr;
    if (!owner) return DICOMFLUX_INVALID_ARGUMENT;
    void* storage = nullptr;
    const auto status = allocate_owned(owner, sizeof(Cancellation), storage);
    if (status != DICOMFLUX_OK) return status;
    out = ::new (storage) Cancellation(owner);
    return DICOMFLUX_OK;
}
void Cancellation::retain() noexcept {
    if (!references_.try_retain()) std::terminate();
}
void Cancellation::release() noexcept {
    const auto result = references_.release();
    if (result == References::Released::invalid) std::terminate();
    if (result == References::Released::last) {
        auto* owner = owner_;
        this->~Cancellation(); free_owned(owner, this, sizeof(Cancellation));
    }
}
}
