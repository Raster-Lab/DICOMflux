#include <dicomflux/df0_probe.h>
#include <dicomflux/probe_defaults.h>
#include <stdio.h>
#include <stdalign.h>
int main(void) {
    dicomflux_probe_abi abi = {0};
    dicomflux_probe_context *context = NULL;
    dicomflux_probe_options options = {sizeof(options), 0, 0, 0,
        DF_QUAL_METADATA, DF_QUAL_ATTRIBUTES, DF_QUAL_BULK, DF_QUAL_TRACKED,
        DF_QUAL_CHUNK, NULL, NULL, NULL};
    dicomflux_probe_measurement result = {0};
    if (dicomflux_probe_query_abi(0, sizeof(abi), &abi) ||
        abi.options_size != sizeof(options) || abi.options_alignment != alignof(dicomflux_probe_options)) return 1;
    if (dicomflux_probe_create(&options, &context)) return 2;
    int status = dicomflux_probe_measure(context,16,16,3,1024,50,&result);
    dicomflux_probe_release(context);
    if (status || result.logical_bytes != 768) return 3;
    printf("C11 installed consumer: pointer_bits=%u options_size=%u alignment=%u libcxx=%u bytes=768\n",
           abi.pointer_bits, abi.options_size, abi.options_alignment, abi.libcxx_version);
    return 0;
}
