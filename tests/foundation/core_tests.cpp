// MIT; Copyright (c) 2026 Raster Images. DF-A002 host foundation assurance.
#include "core.hpp"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <new>
#include <thread>
#include <vector>

namespace core = dicomflux::core;
static unsigned checks = 0;
#define CHECK(condition) do { ++checks; if (!(condition)) { std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); std::abort(); } } while (false)
static void thread_check(bool condition) { if (!condition) std::abort(); }
static dicomflux_versioned header(uint32_t size) { return {size, 0, 1, 0}; }
static dicomflux_context_options options() {
    dicomflux_context_options result{};
    result.header = header(sizeof(result));
    result.allocator.header = header(sizeof(result.allocator));
    result.budgets.header = header(sizeof(result.budgets));
    result.budgets.tracked_bytes = 65536;
    result.budgets.bulk_bytes = UINT64_MAX; // No allocation tied to this declaration.
    return result;
}
static dicomflux_error error_output() {
    dicomflux_error result{}; result.header = header(sizeof(result)); return result;
}
struct CountingAllocator {
    struct Entry { void* returned{}; void* base{}; size_t size{}; };
    std::array<Entry, 128> entries{};
    std::mutex lock;
    unsigned calls{}, frees{}, fail_at{};
    size_t live_bytes{}, peak_bytes{};
    int failure_mode{}; // 0: NULL; 1: bad_alloc; 2: bounded C++ exception; 3: misalignment.
    static void* allocate(void* user, size_t bytes) {
        auto& self = *static_cast<CountingAllocator*>(user);
        std::lock_guard guard(self.lock);
        ++self.calls;
        if (self.fail_at == self.calls) {
            if (self.failure_mode == 0) return nullptr;
            if (self.failure_mode == 1) throw std::bad_alloc();
            if (self.failure_mode == 2) throw 1;
        }
        void* base = std::malloc(bytes + 16);
        if (!base) return nullptr;
        void* returned = (self.fail_at == self.calls && self.failure_mode == 3)
            ? static_cast<void*>(static_cast<unsigned char*>(base) + 1) : base;
        for (auto& entry : self.entries) if (!entry.returned) {
            entry = {returned, base, bytes}; self.live_bytes += bytes;
            if (self.live_bytes > self.peak_bytes) self.peak_bytes = self.live_bytes;
            return returned;
        }
        std::abort();
    }
    static void deallocate(void* user, void* allocation) {
        auto& self = *static_cast<CountingAllocator*>(user);
        std::lock_guard guard(self.lock);
        for (auto& entry : self.entries) if (entry.returned == allocation) {
            ++self.frees; self.live_bytes -= entry.size; std::free(entry.base); entry = {}; return;
        }
        std::abort(); // unknown/double free is a test failure.
    }
    void install(dicomflux_context_options& o) {
        o.allocator.user = this; o.allocator.allocate = allocate; o.allocator.deallocate = deallocate;
    }
};

static void abi_and_validation() {
    struct { dicomflux_abi_info info; uint64_t guard; } storage{};
    storage.guard = UINT64_C(0xACED1234);
    CHECK(dicomflux_query_abi(0, 1, sizeof(storage.info), &storage.info) == DICOMFLUX_OK);
    CHECK(storage.info.pointer_bits == sizeof(void*) * 8);
    CHECK(storage.info.native_alignment == alignof(std::max_align_t));
    CHECK(storage.info.diagnostic_capacity == 256 && storage.info.reserved == 0);
    CHECK(storage.info.header.struct_size == sizeof(storage.info));
    CHECK(dicomflux_query_abi(0, 0, sizeof(storage.info), &storage.info) == DICOMFLUX_UNSUPPORTED);
    CHECK(storage.info.pointer_bits == 0 && storage.info.diagnostic_capacity == 0);
    CHECK(dicomflux_query_abi(1, 1, sizeof(storage.info), &storage.info) == DICOMFLUX_UNSUPPORTED);
    std::memset(&storage.info, 0x5a, sizeof(storage.info));
    const auto unchanged = storage.info;
    CHECK(dicomflux_query_abi(0, 1, sizeof(storage.info) - 1, &storage.info) == DICOMFLUX_INVALID_ARGUMENT);
    CHECK(std::memcmp(&unchanged, &storage.info, sizeof(unchanged)) == 0);
    CHECK(dicomflux_query_abi(0, 1, sizeof(storage), &storage.info) == DICOMFLUX_INVALID_ARGUMENT);
    CHECK(std::memcmp(&unchanged, &storage.info, sizeof(unchanged)) == 0);
    CHECK(storage.guard == UINT64_C(0xACED1234));
    CHECK(dicomflux_query_abi(0, 1, sizeof(storage.info), nullptr) == DICOMFLUX_INVALID_ARGUMENT);

    auto original = options(); CountingAllocator counter; counter.install(original);
    for (unsigned which = 0; which < 3; ++which) {
        for (unsigned mutation = 0; mutation < 6; ++mutation) {
            // Extra storage makes an oversized outer extent a truthful capacity.
            struct { dicomflux_context_options o; unsigned char extra[16]; } wrapped{original, {}};
            auto& o = wrapped.o; auto err = error_output(); dicomflux_context* ctx = reinterpret_cast<dicomflux_context*>(uintptr_t{1});
            auto* h = which == 0 ? &o.header : which == 1 ? &o.allocator.header : &o.budgets.header;
            if (mutation == 0) h->struct_size = 0;
            if (mutation == 1) --h->struct_size;
            if (mutation == 2) ++h->struct_size;
            if (mutation == 3) ++h->abi_major;
            if (mutation == 4) --h->abi_minor;
            if (mutation == 5) h->flags = 1;
            const auto expected = (mutation == 3 || mutation == 4) ? DICOMFLUX_UNSUPPORTED : DICOMFLUX_INVALID_ARGUMENT;
            CHECK(dicomflux_context_create(&o, &ctx, &err) == expected);
            CHECK(ctx == nullptr && err.status == expected);
            CHECK(err.component == DICOMFLUX_COMPONENT_ABI && err.operation == DICOMFLUX_OPERATION_CONTEXT_CREATE);
            CHECK(counter.calls == 0);
            CHECK(err.detail_flags == (DICOMFLUX_ERROR_HAS_EXPECTED | DICOMFLUX_ERROR_HAS_OBSERVED));
        }
    }
    for (unsigned i = 0; i < 4; ++i) {
        auto o = original; o.reserved[i] = UINT64_MAX; dicomflux_context* ctx = nullptr;
        CHECK(dicomflux_context_create(&o, &ctx, nullptr) == DICOMFLUX_INVALID_ARGUMENT);
        CHECK(!ctx && counter.calls == 0);
    }
    for (bool missing_allocate : {false, true}) {
        auto o = original;
        if (missing_allocate) o.allocator.allocate = nullptr; else o.allocator.deallocate = nullptr;
        auto e = error_output(); dicomflux_context* ctx = nullptr;
        CHECK(dicomflux_context_create(&o, &ctx, &e) == DICOMFLUX_INVALID_ARGUMENT);
        CHECK(e.component == DICOMFLUX_COMPONENT_CONTEXT && !ctx && counter.calls == 0);
    }
    dicomflux_context* ctx = reinterpret_cast<dicomflux_context*>(uintptr_t{1}); auto e = error_output();
    CHECK(dicomflux_context_create(nullptr, &ctx, &e) == DICOMFLUX_INVALID_ARGUMENT && !ctx);
    CHECK(dicomflux_context_create(&original, nullptr, &e) == DICOMFLUX_INVALID_ARGUMENT);
    for (unsigned mutation = 0; mutation < 4; ++mutation) {
        struct { dicomflux_error e; unsigned char extra[16]; } wrapped{error_output(), {}};
        if (mutation == 0) wrapped.e.header.struct_size = sizeof(wrapped);
        if (mutation == 1) wrapped.e.header.struct_size = 4;
        if (mutation == 2) wrapped.e.header.flags = 1;
        if (mutation == 3) wrapped.e.header.abi_minor = 0;
        std::array<unsigned char, sizeof(wrapped)> before{}; std::memcpy(before.data(), &wrapped, sizeof(wrapped));
        ctx = reinterpret_cast<dicomflux_context*>(uintptr_t{1});
        CHECK(dicomflux_context_create(&original, &ctx, &wrapped.e) != DICOMFLUX_OK);
        CHECK(!ctx && std::memcmp(before.data(), &wrapped, sizeof(wrapped)) == 0 && counter.calls == 0);
    }
    // Allocation is only four bytes: ASan witnesses no speculative full-header read.
    auto* short_record = static_cast<uint32_t*>(std::malloc(sizeof(uint32_t)));
    CHECK(short_record != nullptr); *short_record = sizeof(uint32_t);
    CHECK(dicomflux_context_create(reinterpret_cast<const dicomflux_context_options*>(short_record), &ctx, &e) == DICOMFLUX_INVALID_ARGUMENT);
    CHECK(!ctx);
    CHECK(dicomflux_context_create(&original, &ctx, reinterpret_cast<dicomflux_error*>(short_record)) == DICOMFLUX_INVALID_ARGUMENT);
    CHECK(*short_record == sizeof(uint32_t) && !ctx);
    std::free(short_record);
    auto o = options(); o.budgets.tracked_bytes = 0;
    CHECK(dicomflux_context_create(&o, &ctx, &e) == DICOMFLUX_RESOURCE_LIMIT);
    CHECK(!ctx && e.expected == sizeof(dicomflux_context) && e.observed == 0);
    CHECK(e.tag == DICOMFLUX_NO_TAG && e.source_offset == DICOMFLUX_UNKNOWN_QUANTITY);
    CHECK(e.diagnostic_length > 0 && e.diagnostic[e.diagnostic_length] == 0);
}

static void errors_and_copy() {
    auto e = error_output(); core::set_error(&e, DICOMFLUX_OK);
    size_t required = 99; char empty = 'x';
    CHECK(e.status == 0 && e.component == 0 && e.operation == 0 && e.detail_flags == 0);
    CHECK(e.tag == UINT32_MAX && e.source_offset == UINT64_MAX && e.expected == UINT64_MAX && e.observed == UINT64_MAX);
    CHECK(dicomflux_error_copy(&e, nullptr, 0, &required) == DICOMFLUX_OK && required == 1);
    CHECK(dicomflux_error_copy(&e, &empty, 0, &required) == DICOMFLUX_BUFFER_TOO_SMALL && empty == 'x');
    CHECK(dicomflux_error_copy(&e, &empty, 1, &required) == DICOMFLUX_OK && empty == 0);
    std::array<char, 300> text{}; text.fill('A');
    for (size_t length : {size_t{0}, size_t{1}, size_t{254}, size_t{255}, size_t{256}, size_t{300}}) {
        core::set_error(&e, DICOMFLUX_ALLOCATION_FAILURE, DICOMFLUX_COMPONENT_CONTEXT,
                        DICOMFLUX_OPERATION_CONTEXT_CREATE, {text.data(), length});
        const size_t actual = length < 255 ? length : 255;
        CHECK(e.diagnostic_length == actual && e.truncated == (length > 255));
        CHECK(e.diagnostic[actual] == 0);
        const auto snapshot = e; std::array<char, 258> output{}; output.fill('Z');
        CHECK(dicomflux_error_copy(&e, nullptr, 0, &required) == DICOMFLUX_OK && required == actual + 1);
        CHECK(dicomflux_error_copy(&e, output.data(), actual, &required) == DICOMFLUX_BUFFER_TOO_SMALL);
        CHECK(output[0] == 'Z' && output[actual] == 'Z');
        CHECK(dicomflux_error_copy(&e, output.data(), actual + 1, &required) == DICOMFLUX_OK);
        CHECK(output[actual] == 0 && output[actual + 1] == 'Z');
        CHECK(std::memcmp(&snapshot, &e, sizeof(e)) == 0);
    }
    // Authoritative bits permit valid all-ones quantities/tags.
    e.detail_flags = DICOMFLUX_ERROR_DETAIL_MASK; e.tag = UINT32_MAX; e.source_offset = e.expected = e.observed = UINT64_MAX;
    CHECK(dicomflux_error_copy(&e, nullptr, 0, &required) == DICOMFLUX_OK);
    for (unsigned mutation = 0; mutation < 12; ++mutation) {
        auto bad = e;
        switch (mutation) {
        case 0: bad.status = DICOMFLUX_NOT_EXECUTED; break;
        case 1: bad.status = 15; break;
        case 2: bad.component = 9; break;
        case 3: bad.operation = 13; break;
        case 4: bad.detail_flags |= 16; break;
        case 5: bad.truncated = 2; break;
        case 6: bad.diagnostic_length = 256; break;
        case 7: bad.diagnostic[bad.diagnostic_length] = 'X'; break;
        case 8: bad.diagnostic[0] = 0; break;
        case 9: bad.detail_flags = 0; bad.tag = 0; break;
        case 10: bad.detail_flags = 0; bad.observed = 0; break;
        case 11: bad.status = DICOMFLUX_OK; break;
        }
        char output = 'Z'; required = 99;
        CHECK(dicomflux_error_copy(&bad, &output, 1, &required) == DICOMFLUX_INVALID_ARGUMENT);
        CHECK(required == 0 && output == 'Z');
    }
    CHECK(dicomflux_error_copy(&e, nullptr, 1, &required) == DICOMFLUX_INVALID_ARGUMENT && required == 0);
    CHECK(dicomflux_error_copy(&e, nullptr, 0, nullptr) == DICOMFLUX_INVALID_ARGUMENT);
    CHECK(dicomflux_error_copy(nullptr, nullptr, 0, &required) == DICOMFLUX_INVALID_ARGUMENT && required == 0);
    for (unsigned mutation = 0; mutation < 5; ++mutation) {
        struct { dicomflux_error value; unsigned char extra[16]; } wrapped{e, {}};
        if (mutation == 0) wrapped.value.header.struct_size = 4;
        if (mutation == 1) wrapped.value.header.struct_size = sizeof(wrapped);
        if (mutation == 2) wrapped.value.header.abi_major = 1;
        if (mutation == 3) wrapped.value.header.abi_minor = 0;
        if (mutation == 4) wrapped.value.header.flags = 1;
        std::array<unsigned char, sizeof(wrapped)> before{};
        std::memcpy(before.data(), &wrapped, sizeof(wrapped));
        char destination = 'Z'; required = 99;
        CHECK(dicomflux_error_copy(&wrapped.value, &destination, 1, &required) ==
              ((mutation == 2 || mutation == 3) ? DICOMFLUX_UNSUPPORTED : DICOMFLUX_INVALID_ARGUMENT));
        CHECK(destination == 'Z' && required == 0 && std::memcmp(before.data(), &wrapped, sizeof(wrapped)) == 0);
    }
    auto* short_record = static_cast<uint32_t*>(std::malloc(4)); CHECK(short_record); *short_record = 4;
    CHECK(dicomflux_error_copy(reinterpret_cast<dicomflux_error*>(short_record), nullptr, 0, &required) == DICOMFLUX_INVALID_ARGUMENT);
    std::free(short_record);
}

static void arithmetic_and_budget() {
    uint64_t out = 9; uint32_t small = 9; size_t native = 9; int64_t signed_out = 9;
    CHECK(core::add(0, 0, out) && out == 0);
    CHECK(core::add(UINT64_MAX, 0, out) && out == UINT64_MAX);
    CHECK(!core::add(UINT64_MAX, 1, out) && out == 0);
    CHECK(core::multiply(UINT64_MAX, 0, out) && out == 0);
    CHECK(core::multiply(UINT64_MAX, 1, out) && out == UINT64_MAX);
    CHECK(!core::multiply(UINT64_MAX, 2, out) && out == 0);
    CHECK(core::multiply(UINT32_MAX, UINT32_MAX, out) && out == UINT64_C(18446744065119617025));
    CHECK(core::even_length(0, out) && out == 0);
    CHECK(core::even_length(3, out) && out == 4);
    CHECK(core::even_length(UINT64_MAX - 1, out) && out == UINT64_MAX - 1);
    CHECK(!core::even_length(UINT64_MAX, out) && out == 0);
    CHECK(core::narrow(UINT32_MAX, small) && small == UINT32_MAX);
    CHECK(!core::narrow(uint64_t{UINT32_MAX} + 1, small) && small == 0);
    CHECK(core::narrow(std::numeric_limits<size_t>::max(), native) && native == std::numeric_limits<size_t>::max());
    CHECK(core::to_unsigned(INT64_MAX, out) && out == uint64_t{INT64_MAX});
    CHECK(core::to_unsigned(0, out) && out == 0);
    CHECK(!core::to_unsigned(-1, out) && out == 0);
    CHECK(!core::to_unsigned(INT64_MIN, out) && out == 0);
    CHECK(core::to_signed(INT64_MAX, signed_out) && signed_out == INT64_MAX);
    CHECK(!core::to_signed(uint64_t{INT64_MAX} + 1, signed_out) && signed_out == 0);
    core::Budget budget(10);
    CHECK(budget.reserve(11) == DICOMFLUX_RESOURCE_LIMIT && budget.reserved() == 0);
    CHECK(budget.reserve(10) == DICOMFLUX_OK && budget.live() == 0 && budget.peak() == 0);
    budget.rollback(10); CHECK(budget.reserved() == 0);
    CHECK(budget.reserve(10) == DICOMFLUX_OK); budget.commit(10);
    CHECK(budget.live() == 10 && budget.peak() == 10);
    budget.free(10); CHECK(budget.reserved() == 0 && budget.live() == 0 && budget.peak() == 10);
    core::Budget zero(0); CHECK(zero.reserve(1) == DICOMFLUX_RESOURCE_LIMIT);
    core::Budget maximum(UINT64_MAX);
    CHECK(maximum.reserve(UINT64_MAX) == DICOMFLUX_OK);
    CHECK(maximum.reserve(1) == DICOMFLUX_OVERFLOW);
    maximum.rollback(UINT64_MAX); CHECK(maximum.reserved() == 0);
    core::References refs(UINT64_MAX);
    CHECK(!refs.try_retain() && refs.count() == UINT64_MAX);
    core::References dead(0); CHECK(!dead.try_retain() && dead.release() == core::References::Released::invalid && dead.count() == 0);
    core::References pair; CHECK(pair.try_retain() && pair.release() == core::References::Released::alive);
    CHECK(pair.release() == core::References::Released::last && !pair.try_retain());
}

static void allocation_and_lifetime() {
    unsigned exercised_sites = 0;
    for (int mode = 0; mode < 4; ++mode) for (unsigned fail = 0; fail <= 3; ++fail) {
        CountingAllocator counter; counter.fail_at = fail; counter.failure_mode = mode;
        auto o = options(); counter.install(o); auto e = error_output(); dicomflux_context* ctx = nullptr;
        const auto expected_failure = mode == 2 ? DICOMFLUX_CALLBACK_FAILURE : mode == 3 ? DICOMFLUX_INVALID_ARGUMENT : DICOMFLUX_ALLOCATION_FAILURE;
        const auto status = dicomflux_context_create(&o, &ctx, &e);
        if (fail == 1) {
            CHECK(status == expected_failure && !ctx && counter.live_bytes == 0);
            CHECK(e.status == status && e.operation == DICOMFLUX_OPERATION_CONTEXT_CREATE);
            char message[256]; size_t required = 0;
            CHECK(dicomflux_error_copy(&e, message, sizeof(message), &required) == DICOMFLUX_OK && required > 1);
            CHECK(counter.calls == 1); // Error production/copy performed no allocator calls.
            continue;
        }
        CHECK(status == DICOMFLUX_OK && ctx && counter.live_bytes == sizeof(*ctx));
        CHECK(e.status == DICOMFLUX_OK && e.diagnostic_length == 0);
        const auto base = ctx->memory.live();
        core::OwnedBlock* block = nullptr;
        const auto bs = core::OwnedBlock::create(ctx, 32, block);
        CHECK(bs == (fail == 2 ? expected_failure : DICOMFLUX_OK));
        if (!block) CHECK(ctx->memory.live() == base && ctx->memory.reserved() == base && ctx->memory.peak() == base);
        core::Cancellation* token = nullptr;
        const auto before_token = ctx->memory.live();
        const auto ts = core::Cancellation::create(ctx, token);
        CHECK(ts == (fail == 3 ? expected_failure : DICOMFLUX_OK));
        if (!token) CHECK(ctx->memory.live() == before_token && ctx->memory.reserved() == before_token);
        if (block) { std::memset(block->data(), 0x42, 32); block->destroy(); }
        if (token) token->release();
        CHECK(ctx->memory.live() == base && ctx->memory.reserved() == base);
        CHECK(counter.calls == 3); exercised_sites = counter.calls;
        dicomflux_context_release(ctx);
        CHECK(counter.live_bytes == 0 && counter.frees == 3 - (fail && mode != 3 ? 1U : 0U));
    }
    CHECK(exercised_sites == 3);
    CountingAllocator counter; auto o = options(); counter.install(o);
    o.budgets.tracked_bytes = sizeof(dicomflux_context);
    dicomflux_context* ctx = nullptr; CHECK(dicomflux_context_create(&o, &ctx, nullptr) == DICOMFLUX_OK);
    core::OwnedBlock* block = nullptr;
    CHECK(core::OwnedBlock::create(ctx, 0, block) == DICOMFLUX_RESOURCE_LIMIT && !block && counter.calls == 1);
    CHECK(core::OwnedBlock::create(ctx, UINT64_MAX, block) == DICOMFLUX_OVERFLOW && !block && counter.calls == 1);
    CHECK(ctx->memory.live() == sizeof(*ctx) && ctx->memory.reserved() == sizeof(*ctx));
    dicomflux_context_release(ctx); CHECK(counter.live_bytes == 0);
    o = options(); counter.install(o);
    CHECK(dicomflux_context_create(&o, &ctx, nullptr) == DICOMFLUX_OK);
    // Mutating caller descriptors afterward cannot change the copied state.
    o.allocator = {}; o.budgets = {};
    CHECK(core::OwnedBlock::create(ctx, 0, block) == DICOMFLUX_OK && block);
    {
        core::ContextReference owned(ctx); core::ContextReference copy(owned); core::ContextReference moved(std::move(copy));
        CHECK(copy.get() == nullptr && moved.get() == ctx);
        dicomflux_context_retain(ctx); dicomflux_context_release(ctx);
        dicomflux_context_release(ctx); // Internal owners now retain all state.
        CHECK(block->owner == owned.get() && counter.live_bytes > 0);
        block->destroy(); CHECK(counter.live_bytes == sizeof(dicomflux_context));
    }
    CHECK(counter.live_bytes == 0);
    core::Cancellation* token = nullptr;
    o = options(); counter.install(o); CHECK(dicomflux_context_create(&o, &ctx, nullptr) == DICOMFLUX_OK);
    CHECK(core::Cancellation::create(ctx, token) == DICOMFLUX_OK && !token->requested());
    token->retain(); dicomflux_context_release(ctx); token->request(); CHECK(token->requested());
    token->release(); CHECK(counter.live_bytes > 0); token->release(); CHECK(counter.live_bytes == 0);
    CHECK(core::OwnedBlock::create(nullptr, 1, block) == DICOMFLUX_INVALID_ARGUMENT && !block);
    CHECK(core::Cancellation::create(nullptr, token) == DICOMFLUX_INVALID_ARGUMENT && !token);
    dicomflux_context_retain(nullptr); dicomflux_context_release(nullptr);
}

static void concurrency() {
    CountingAllocator allocator; auto o = options(); allocator.install(o);
    dicomflux_context* ctx = nullptr; CHECK(dicomflux_context_create(&o, &ctx, nullptr) == DICOMFLUX_OK);
    core::Cancellation* token = nullptr; CHECK(core::Cancellation::create(ctx, token) == DICOMFLUX_OK);
    std::atomic<bool> start{false}; std::vector<std::thread> workers;
    for (unsigned index = 0; index < 4; ++index) {
        dicomflux_context_retain(ctx); token->retain(); // One owned ref per thread.
        workers.emplace_back([&, index] {
            while (!start.load(std::memory_order_acquire)) std::this_thread::yield();
            for (unsigned i = 0; i < 2000; ++i) { dicomflux_context_retain(ctx); dicomflux_context_release(ctx); }
            for (unsigned i = 0; i < 100; ++i) {
                core::OwnedBlock* block = nullptr;
                thread_check(core::OwnedBlock::create(ctx, 32 + index, block) == DICOMFLUX_OK);
                std::memset(block->data(), 0x5A, static_cast<size_t>(block->payload_bytes)); block->destroy();
            }
            token->request(); thread_check(token->requested());
            auto independent_options = options(); dicomflux_context* independent = nullptr;
            thread_check(dicomflux_context_create(&independent_options, &independent, nullptr) == DICOMFLUX_OK);
            dicomflux_context_release(independent); token->release(); dicomflux_context_release(ctx);
        });
    }
    start.store(true, std::memory_order_release);
    for (auto& worker : workers) worker.join();
    CHECK(token->requested()); token->release();
    CHECK(ctx->references.count() == 1 && ctx->memory.reserved() == sizeof(*ctx) && ctx->memory.live() == sizeof(*ctx));
    dicomflux_context_release(ctx); CHECK(allocator.live_bytes == 0 && allocator.calls == allocator.frees);
}
int main() {
    abi_and_validation(); errors_and_copy(); arithmetic_and_budget(); allocation_and_lifetime(); concurrency();
    std::printf("DF-A002 foundation checks=%u allocation_sites=3 fault_modes=4 concurrent_reference_pairs=8000 concurrent_owned_blocks=400 independent_contexts=4 context_bytes=%zu options_size=%zu error_size=%zu\n", checks, sizeof(dicomflux_context), sizeof(dicomflux_context_options), sizeof(dicomflux_error));
    return 0;
}
