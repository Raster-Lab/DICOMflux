/* MIT; Copyright (c) 2026 Raster Images. Installed-header-only client. */
#include <dicomflux/foundation.h>
#include <stdio.h>
#include <string.h>
#define REQUIRE(x) do { if (!(x)) return __LINE__; } while (0)
static dicomflux_versioned versioned(uint32_t size) {
    dicomflux_versioned result = {size, DICOMFLUX_ABI_MAJOR, DICOMFLUX_ABI_MINOR, 0}; return result;
}
int main(void) {
    dicomflux_abi_info abi = {0};
    dicomflux_context_options options = {0};
    dicomflux_error error = {0};
    dicomflux_context *context = NULL;
    size_t required = 0; char message[256];
    REQUIRE(dicomflux_query_abi(0, 1, sizeof(abi), &abi) == DICOMFLUX_OK);
    REQUIRE(abi.pointer_bits == sizeof(void*) * 8 && abi.diagnostic_capacity == 256);
    options.header = versioned(sizeof(options));
    options.allocator.header = versioned(sizeof(options.allocator));
    options.budgets.header = versioned(sizeof(options.budgets));
    options.budgets.tracked_bytes = UINT64_C(8388608);
    error.header = versioned(sizeof(error));
    REQUIRE(dicomflux_context_create(&options, &context, &error) == DICOMFLUX_OK && context != NULL);
    dicomflux_context_retain(context);
    dicomflux_context_release(context);
    REQUIRE(dicomflux_error_copy(&error, NULL, 0, &required) == DICOMFLUX_OK && required == 1);
    REQUIRE(dicomflux_error_copy(&error, message, sizeof(message), &required) == DICOMFLUX_OK && message[0] == 0);
    dicomflux_context_release(context);
    options.header.flags = 1;
    REQUIRE(dicomflux_context_create(&options, &context, &error) == DICOMFLUX_INVALID_ARGUMENT && context == NULL);
    REQUIRE(error.component == DICOMFLUX_COMPONENT_ABI && error.operation == DICOMFLUX_OPERATION_CONTEXT_CREATE);
    REQUIRE(dicomflux_error_copy(&error, message, sizeof(message), &required) == DICOMFLUX_OK && strlen(message) + 1 == required);
    dicomflux_context_release(NULL);
    puts("installed C11 consumer: five functions called; success and rejection witnessed");
    return 0;
}
