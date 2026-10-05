// MIT; Copyright (c) 2026 Raster Images. Independent installed C++20 client.
#include <dicomflux/foundation.h>
#include <array>
#include <cstdio>
#include <type_traits>
#define REQUIRE(x) do { if (!(x)) return __LINE__; } while (false)
static_assert(std::is_same_v<dicomflux_status, int32_t>);
static_assert(std::is_standard_layout_v<dicomflux_context_options>);
int main() {
    dicomflux_abi_info abi{};
    REQUIRE(dicomflux_query_abi(0, 1, sizeof(abi), &abi) == DICOMFLUX_OK);
    dicomflux_context_options options{};
    options.header = {sizeof(options), 0, 1, 0};
    options.allocator.header = {sizeof(options.allocator), 0, 1, 0};
    options.budgets.header = {sizeof(options.budgets), 0, 1, 0};
    options.budgets.tracked_bytes = 8388608;
    dicomflux_error error{}; error.header = {sizeof(error), 0, 1, 0};
    dicomflux_context* context = nullptr;
    REQUIRE(dicomflux_context_create(&options, &context, &error) == DICOMFLUX_OK);
    dicomflux_context_retain(context); dicomflux_context_release(context);
    std::array<char, 256> diagnostic{}; size_t required = 0;
    REQUIRE(dicomflux_error_copy(&error, diagnostic.data(), diagnostic.size(), &required) == DICOMFLUX_OK);
    REQUIRE(required == 1 && diagnostic[0] == '\0' && error.tag == UINT32_MAX);
    dicomflux_context_release(context);
    options.budgets.tracked_bytes = 0;
    REQUIRE(dicomflux_context_create(&options, &context, &error) == DICOMFLUX_RESOURCE_LIMIT && !context);
    REQUIRE(dicomflux_error_copy(&error, nullptr, 0, &required) == DICOMFLUX_OK && required > 1);
    std::puts("installed C++20 consumer: five functions called; zero budget rejected");
    return 0;
}
