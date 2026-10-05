// MIT; Copyright (c) 2026 Raster Images. Private implementation, never installed.
#ifndef DICOMFLUX_FOUNDATION_CORE_HPP
#define DICOMFLUX_FOUNDATION_CORE_HPP
#include <dicomflux/foundation.h>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string_view>
#include <type_traits>
#include <utility>

namespace dicomflux::core {
constexpr bool add(uint64_t a, uint64_t b, uint64_t& out) noexcept {
    out = 0;
    if (a > UINT64_MAX - b) return false;
    out = a + b; return true;
}
constexpr bool multiply(uint64_t a, uint64_t b, uint64_t& out) noexcept {
    out = 0;
    if (b && a > UINT64_MAX / b) return false;
    out = a * b; return true;
}
constexpr bool even_length(uint64_t value, uint64_t& out) noexcept {
    return add(value, value & 1, out);
}
template<class T> constexpr bool narrow(uint64_t value, T& out) noexcept {
    static_assert(std::is_integral_v<T> && std::is_unsigned_v<T> && !std::is_same_v<T, bool>);
    out = 0;
    if (value > std::numeric_limits<T>::max()) return false;
    out = static_cast<T>(value); return true;
}
constexpr bool to_unsigned(int64_t value, uint64_t& out) noexcept {
    out = 0;
    if (value < 0) return false;
    out = static_cast<uint64_t>(value); return true;
}
constexpr bool to_signed(uint64_t value, int64_t& out) noexcept {
    out = 0;
    if (value > static_cast<uint64_t>(INT64_MAX)) return false;
    out = static_cast<int64_t>(value); return true;
}

struct Validation {
    dicomflux_status status = DICOMFLUX_OK;
    uint64_t expected = DICOMFLUX_UNKNOWN_QUANTITY;
    uint64_t observed = DICOMFLUX_UNKNOWN_QUANTITY;
    uint32_t details = 0;
};
Validation validate_record(const void* record, uint32_t size) noexcept;
void set_error(dicomflux_error* error, dicomflux_status status,
               uint32_t component = DICOMFLUX_COMPONENT_NONE,
               uint32_t operation = DICOMFLUX_OPERATION_NONE,
               std::string_view diagnostic = {}, Validation detail = {}) noexcept;
dicomflux_status validate_error(const dicomflux_error* error) noexcept;

class References {
public:
    explicit References(uint64_t initial = 1) noexcept : count_(initial) {}
    bool try_retain() noexcept;
    enum class Released { alive, last, invalid };
    Released release() noexcept;
    uint64_t count() const noexcept { return count_.load(std::memory_order_relaxed); }
private:
    std::atomic<uint64_t> count_;
};

// Reservations precede allocation; live/peak count only successful allocations.
class Budget {
public:
    explicit Budget(uint64_t limit, uint64_t initial = 0) noexcept
        : limit_(limit), reserved_(initial), live_(initial), peak_(initial) {}
    dicomflux_status reserve(uint64_t bytes) noexcept;
    void rollback(uint64_t bytes) noexcept;
    void commit(uint64_t bytes) noexcept;
    void free(uint64_t bytes) noexcept;
    uint64_t reserved() const noexcept { return reserved_.load(std::memory_order_relaxed); }
    uint64_t live() const noexcept { return live_.load(std::memory_order_relaxed); }
    uint64_t peak() const noexcept { return peak_.load(std::memory_order_relaxed); }
private:
    const uint64_t limit_;
    std::atomic<uint64_t> reserved_, live_, peak_;
};

struct Allocator {
    void* user;
    dicomflux_allocate_fn allocate;
    dicomflux_free_fn deallocate;
    dicomflux_status get(size_t bytes, void*& output) const noexcept;
    void put(void* allocation) const noexcept;
};
void retain(dicomflux_context* context) noexcept;
void release(dicomflux_context* context) noexcept;
dicomflux_status create(const dicomflux_context_options& options, dicomflux_context*& out) noexcept;

class ContextReference {
public:
    explicit ContextReference(dicomflux_context* context = nullptr) noexcept : context_(context) { retain(context_); }
    ContextReference(const ContextReference& other) noexcept : ContextReference(other.context_) {}
    ContextReference(ContextReference&& other) noexcept : context_(std::exchange(other.context_, nullptr)) {}
    ContextReference& operator=(const ContextReference&) = delete;
    ContextReference& operator=(ContextReference&&) = delete;
    ~ContextReference() { release(context_); }
    dicomflux_context* get() const noexcept { return context_; }
private:
    dicomflux_context* context_;
};

// Internal generic owned storage. It is not a builder, dataset or write plan.
struct alignas(std::max_align_t) OwnedBlock {
    dicomflux_context* owner;
    size_t allocation_bytes;
    uint64_t payload_bytes;
    static dicomflux_status create(dicomflux_context*, uint64_t bytes, OwnedBlock*& output) noexcept;
    void destroy() noexcept;
    unsigned char* data() noexcept { return reinterpret_cast<unsigned char*>(this + 1); }
};

class Cancellation {
public:
    static dicomflux_status create(dicomflux_context*, Cancellation*& output) noexcept;
    void request() noexcept { requested_.store(true, std::memory_order_release); }
    bool requested() const noexcept { return requested_.load(std::memory_order_acquire); }
    void retain() noexcept;
    void release() noexcept;
private:
    explicit Cancellation(dicomflux_context* owner) noexcept : owner_(owner) {}
    References references_;
    std::atomic<bool> requested_{false};
    dicomflux_context* owner_;
};
}

// Definition stays private; installed C clients receive only an opaque handle.
struct dicomflux_context final {
    dicomflux::core::References references;
    const dicomflux::core::Allocator allocator;
    const dicomflux_budgets limits;
    dicomflux::core::Budget memory;
    dicomflux_context(dicomflux::core::Allocator a, const dicomflux_budgets& b) noexcept
        : allocator(a), limits(b), memory(b.tracked_bytes, sizeof(dicomflux_context)) {}
};
static_assert(alignof(dicomflux_context) <= alignof(std::max_align_t));
#endif
