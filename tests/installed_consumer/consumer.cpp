#include <dicomflux/df0_probe.h>
#include <dicomflux/probe_defaults.h>
#include <iostream>
#include <memory>
#include <span>
int main() {
    dicomflux_probe_options o{sizeof(o),0,0,0,DF_QUAL_METADATA,DF_QUAL_ATTRIBUTES,
        DF_QUAL_BULK,DF_QUAL_TRACKED,DF_QUAL_CHUNK,nullptr,nullptr,nullptr};
    dicomflux_probe_context *raw=nullptr;
    if (dicomflux_probe_create(&o,&raw)) return 1;
    std::unique_ptr<dicomflux_probe_context,decltype(&dicomflux_probe_release)> owner(raw,dicomflux_probe_release);
    dicomflux_probe_measurement m{};
    if (dicomflux_probe_measure(owner.get(),1,1,3,0,0,&m) || m.padding_bytes!=1) return 2;
    const uint64_t values[]{m.logical_bytes,m.encoded_bytes};
    std::cout << "C++20 installed consumer: " << std::span(values)[0] << " logical, " << values[1] << " encoded\n";
}
