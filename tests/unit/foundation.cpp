// MIT; Copyright (c) 2026 Raster Images.
#include "dicomflux/df0_probe.h"
#include "probe_defaults.h"
#include "../../src/foundation/probe_logic.hpp"
#include <atomic>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <new>
#include <thread>
#include <vector>

using namespace dicomflux::probe;
static unsigned checks = 0;
#define CHECK(x) do { ++checks; if (!(x)) { std::cerr << "check failed: " << #x << ':' << __LINE__ << '\n'; return 1; } } while (0)
struct allocation_state { size_t calls=0, live=0, fail_at=SIZE_MAX; bool throws=false; };
static void *allocate(void *u, size_t n) {
    auto &s = *static_cast<allocation_state *>(u);
    if (s.calls++ == s.fail_at) {
        if (s.throws) throw std::bad_alloc(); // Deliberate C++ fault injection, not a supported callback contract.
        return nullptr;
    }
    void *p = std::malloc(n);
    if (p) ++s.live;
    return p;
}
static void deallocate(void *u, void *p) {
    if (p) { --static_cast<allocation_state *>(u)->live; std::free(p); }
}
static dicomflux_probe_options options() {
    return {sizeof(dicomflux_probe_options), 0, 0, 0, DF_QUAL_METADATA,
            DF_QUAL_ATTRIBUTES, DF_QUAL_BULK, DF_QUAL_TRACKED, DF_QUAL_CHUNK,
            nullptr, nullptr, nullptr};
}
int main() {
    const uint64_t maximum = std::numeric_limits<uint64_t>::max();
    uint64_t n = 10;
    CHECK(add(0, maximum, n) && n == maximum);
    CHECK(!add(maximum, 1, n) && n == 0);
    CHECK(multiply(maximum, 0, n) && n == 0);
    CHECK(!multiply(maximum, 2, n) && n == 0);
    CHECK(multiply(16, 16, n) && n == 256);
    CHECK(even_length(3, n) && n == 4);
    CHECK(!even_length(maximum, n) && n == 0);
    budget b(maximum);
    CHECK(b.reserve(maximum)); CHECK(!b.reserve(1));
    CHECK(b.release(maximum)); CHECK(!b.release(1));
    CHECK(b.used() == 0 && b.peak() == maximum);

    dicomflux_probe_abi abi{};
    CHECK(dicomflux_probe_query_abi(0, sizeof(abi), &abi) == DF_PROBE_OK);
    CHECK(abi.options_size == sizeof(dicomflux_probe_options));
    CHECK(abi.options_alignment == alignof(dicomflux_probe_options));
    CHECK(dicomflux_probe_query_abi(1, sizeof(abi), &abi) == DF_PROBE_UNSUPPORTED);
    CHECK(abi.options_size == 0);
    CHECK(dicomflux_probe_query_abi(0, sizeof(abi)-1, &abi) == DF_PROBE_UNSUPPORTED);
    CHECK(dicomflux_probe_query_abi(0, sizeof(abi), nullptr) == DF_PROBE_INVALID);
    auto o = options();
    dicomflux_probe_context *c = nullptr;
    CHECK(dicomflux_probe_create(nullptr, &c) == DF_PROBE_INVALID && !c);
    CHECK(dicomflux_probe_create(&o, nullptr) == DF_PROBE_INVALID);
    for (int field=0; field<8; ++field) {
        auto bad = o;
        if (field==0) --bad.struct_size;
        if (field==1) ++bad.struct_size;
        if (field==2) bad.abi_major=1;
        if (field==3) bad.flags=1;
        if (field==4) bad.reserved=1;
        if (field==5) bad.chunk_bytes=0;
        if (field==6) bad.max_tracked_bytes=bad.chunk_bytes;
        if (field==7) bad.allocate=allocate;
        CHECK(dicomflux_probe_create(&bad, &c) != DF_PROBE_OK && !c);
    }
    for (bool throws : {false, true}) {
        for (size_t failure=0; failure<3; ++failure) {
            allocation_state state; state.fail_at=failure; state.throws=throws;
            o=options(); o.allocator_user=&state; o.allocate=allocate; o.deallocate=deallocate;
            auto status=dicomflux_probe_create(&o, &c);
            if (failure<2) { CHECK(status == DF_PROBE_ALLOCATION && !c); }
            else { CHECK(status == DF_PROBE_OK && state.live == 2); dicomflux_probe_release(c); c=nullptr; }
            CHECK(state.live == 0);
        }
    }
    o=options(); CHECK(dicomflux_probe_create(&o,&c)==DF_PROBE_OK);
    dicomflux_probe_measurement m{};
    CHECK(dicomflux_probe_measure(c,16,16,3,1024,50,&m)==DF_PROBE_OK);
    CHECK(m.logical_bytes==768 && m.encoded_bytes==768 && m.padding_bytes==0);
    const auto tracked=m.tracked_bytes;
    CHECK(dicomflux_probe_measure(c,1,1,3,0,0,&m)==DF_PROBE_OK);
    CHECK(m.logical_bytes==3 && m.encoded_bytes==4 && m.padding_bytes==1);
    CHECK(dicomflux_probe_measure(c,1,UINT64_C(0xfffffffe),1,0,0,&m)==DF_PROBE_OK);
    CHECK(m.tracked_bytes==tracked);
    CHECK(dicomflux_probe_measure(c,1,UINT64_C(0xffffffff),1,0,0,&m)==DF_PROBE_LIMIT);
    CHECK(m.logical_bytes==0);
    CHECK(dicomflux_probe_measure(c,maximum,2,3,0,0,&m)==DF_PROBE_OVERFLOW);
    CHECK(dicomflux_probe_measure(c,1,1,1,DF_QUAL_METADATA+1,0,&m)==DF_PROBE_LIMIT);
    CHECK(dicomflux_probe_measure(c,1,1,1,0,DF_QUAL_ATTRIBUTES+1,&m)==DF_PROBE_LIMIT);
    CHECK(dicomflux_probe_measure(c,0,1,1,0,0,&m)==DF_PROBE_INVALID);
    dicomflux_probe_release(c); dicomflux_probe_release(nullptr);

    for (auto terminal : {plan_state::succeeded, plan_state::failed, plan_state::cancelled}) {
        single_use_plan p; CHECK(p.start(false)); CHECK(!p.start(false));
        CHECK(!p.finish(plan_state::ready)); CHECK(p.finish(terminal));
        CHECK(!p.finish(terminal)); CHECK(!p.start(false)); CHECK(p.state()==terminal);
    }
    single_use_plan cancelled; CHECK(!cancelled.start(true)); CHECK(!cancelled.start(false));
    uint64_t cursor=0;
    CHECK(progress(7,7,3,io_status::ok,true,cursor)==progress_status::more && cursor==3);
    CHECK(progress(7,4,4,io_status::eof,true,cursor)==progress_status::done && cursor==7);
    CHECK(progress(7,1,1,io_status::ok,true,cursor)==progress_status::failed && cursor==7);
    for (auto status : {io_status::error,io_status::would_block,io_status::eof}) {
        cursor=0; CHECK(progress(7,7,3,status,false,cursor)==progress_status::failed && cursor==3);
    }
    cursor=0; CHECK(progress(7,7,0,io_status::ok,true,cursor)==progress_status::failed && cursor==0);
    CHECK(progress(7,7,8,io_status::ok,true,cursor)==progress_status::failed && cursor==0);
    CHECK(progress(7,7,3,io_status::eof,true,cursor)==progress_status::failed && cursor==3);
    cursor=0; CHECK(progress(7,7,2,io_status::cancelled,true,cursor)==progress_status::cancelled && cursor==2);
    cursor=0; CHECK(progress(7,7,1,static_cast<io_status>(99),true,cursor)==progress_status::failed && cursor==0);

    std::atomic<unsigned> errors{0};
    std::vector<std::thread> workers;
    for (unsigned i=0;i<4;++i) workers.emplace_back([&] {
        auto local=options(); dicomflux_probe_context *context=nullptr;
        if (dicomflux_probe_create(&local,&context)!=DF_PROBE_OK) { ++errors; return; }
        for (unsigned k=0;k<1000;++k) {
            dicomflux_probe_measurement result{};
            if (dicomflux_probe_measure(context,16,16,3,0,0,&result)!=DF_PROBE_OK || result.logical_bytes!=768) ++errors;
        }
        dicomflux_probe_release(context);
    });
    for (auto &worker:workers) worker.join();
    CHECK(errors==0);
    std::cout << "DF0 prototype checks=" << checks << " allocation_sites=2 fault_modes=2 concurrent_measurements=4000 tracked_bytes=" << tracked << '\n';
    return 0;
}
