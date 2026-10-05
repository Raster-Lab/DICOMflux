// MIT; Copyright (c) 2026 Raster Images. Bounded DF-0 logic, not a parser.
#include "dicomflux/df0_probe.h"
#include "probe_defaults.h"
#include "../src/foundation/probe_logic.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
using namespace dicomflux::probe;
static void require(bool condition) { if (!condition) std::abort(); }
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size<32) return 0;
    uint64_t a,b,c,d;
    std::memcpy(&a,data,8); std::memcpy(&b,data+8,8);
    std::memcpy(&c,data+16,8); std::memcpy(&d,data+24,8);
    uint64_t result;
    const bool added=add(a,b,result);
    require(added==(b<=UINT64_MAX-a));
    if (added) require(result-a==b); else require(result==0);
    const bool multiplied=multiply(a,b,result);
    require(multiplied==(a==0 || b<=UINT64_MAX/a));
    if (multiplied && a) require(result/a==b); else if (!multiplied) require(result==0);
    budget limit(a); const bool reserved=limit.reserve(b);
    require(reserved==(b<=a)); require(limit.used()<=a);
    if (reserved) require(limit.release(b) && limit.used()==0);
    uint64_t cursor=0;
    const uint64_t request=a?1+b%a:0;
    auto state=progress(a,request,c,static_cast<io_status>(d%6),true,cursor);
    require(cursor<=a);
    if (c>request || !request) require(state==progress_status::failed && cursor==0);
    if (d%6==5) require(state==progress_status::failed && cursor==0);
    single_use_plan plan;
    bool started=plan.start((d&8)!=0);
    if (started) require(plan.finish(plan_state::succeeded));
    require(!plan.start(false));
    dicomflux_probe_options o{sizeof(o),0,0,0,DF_QUAL_METADATA,DF_QUAL_ATTRIBUTES,
        DF_QUAL_BULK,DF_QUAL_TRACKED,DF_QUAL_CHUNK,nullptr,nullptr,nullptr};
    o.flags=static_cast<uint32_t>(d&1);
    dicomflux_probe_context *context=nullptr;
    const auto created=dicomflux_probe_create(&o,&context);
    require(created==(o.flags?DF_PROBE_UNSUPPORTED:DF_PROBE_OK));
    if (context) {
        dicomflux_probe_measurement measured{};
        const auto status=dicomflux_probe_measure(context,a,b,1,c&0x1fffff,d&0x1fff,&measured);
        if (status==DF_PROBE_OK) {
            require(measured.logical_bytes<=o.max_bulk_bytes);
            require(measured.encoded_bytes<=UINT64_C(0xfffffffe));
            require(measured.encoded_bytes%2==0);
        } else require(measured.logical_bytes==0);
        dicomflux_probe_release(context);
    }
    return 0;
}
