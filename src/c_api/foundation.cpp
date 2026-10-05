// MIT; Copyright (c) 2026 Raster Images.
#include "../foundation/core.hpp"
#include <cstring>
#include <new>

namespace core = dicomflux::core;
extern "C" {
dicomflux_status dicomflux_query_abi(uint32_t major, uint32_t minor, size_t size,
                                    dicomflux_abi_info* output) noexcept {
    if (!output || size != sizeof(*output)) return DICOMFLUX_INVALID_ARGUMENT;
    dicomflux_abi_info result{};
    result.header = {sizeof(result), DICOMFLUX_ABI_MAJOR, DICOMFLUX_ABI_MINOR, 0};
    if (major != DICOMFLUX_ABI_MAJOR || minor != DICOMFLUX_ABI_MINOR) {
        *output = result; return DICOMFLUX_UNSUPPORTED;
    }
    result.pointer_bits = sizeof(void*) * 8;
    result.native_alignment = alignof(std::max_align_t);
    result.diagnostic_capacity = 256;
    *output = result; return DICOMFLUX_OK;
}

dicomflux_status dicomflux_context_create(const dicomflux_context_options* options,
                                         dicomflux_context** output, dicomflux_error* error) noexcept {
    if (output) *output = nullptr;
    if (error) {
        const auto valid = core::validate_record(error, sizeof(*error));
        if (valid.status != DICOMFLUX_OK) return valid.status;
    }
    const auto fail = [&](dicomflux_status status, uint32_t component, const char* message,
                          core::Validation detail = {}) noexcept {
        core::set_error(error, status, component, DICOMFLUX_OPERATION_CONTEXT_CREATE, message, detail);
        return status;
    };
    core::set_error(error, DICOMFLUX_OK);
    try {
        if (!output) return fail(DICOMFLUX_INVALID_ARGUMENT, DICOMFLUX_COMPONENT_CONTEXT, "Missing context output.");
        if (!options) return fail(DICOMFLUX_INVALID_ARGUMENT, DICOMFLUX_COMPONENT_CONTEXT, "Missing context options.");
        auto valid = core::validate_record(options, sizeof(*options));
        if (valid.status != DICOMFLUX_OK) return fail(valid.status, DICOMFLUX_COMPONENT_ABI, "Invalid context record header.", valid);
        valid = core::validate_record(&options->allocator, sizeof(options->allocator));
        if (valid.status != DICOMFLUX_OK) return fail(valid.status, DICOMFLUX_COMPONENT_ABI, "Invalid allocator record header.", valid);
        valid = core::validate_record(&options->budgets, sizeof(options->budgets));
        if (valid.status != DICOMFLUX_OK) return fail(valid.status, DICOMFLUX_COMPONENT_ABI, "Invalid budget record header.", valid);
        for (const auto reserved : options->reserved)
            if (reserved) return fail(DICOMFLUX_INVALID_ARGUMENT, DICOMFLUX_COMPONENT_ABI, "Reserved context field is nonzero.");
        if ((options->allocator.allocate == nullptr) != (options->allocator.deallocate == nullptr))
            return fail(DICOMFLUX_INVALID_ARGUMENT, DICOMFLUX_COMPONENT_CONTEXT, "Allocator callbacks must be paired.");
        const auto status = core::create(*options, *output);
        if (status == DICOMFLUX_RESOURCE_LIMIT)
            return fail(status, DICOMFLUX_COMPONENT_CONTEXT, "Tracked budget cannot admit a context.",
                        {status, sizeof(dicomflux_context), options->budgets.tracked_bytes,
                         DICOMFLUX_ERROR_HAS_EXPECTED | DICOMFLUX_ERROR_HAS_OBSERVED});
        if (status != DICOMFLUX_OK) return fail(status, DICOMFLUX_COMPONENT_CONTEXT, "Context storage allocation failed.");
        return DICOMFLUX_OK;
    } catch (const std::bad_alloc&) {
        return fail(DICOMFLUX_ALLOCATION_FAILURE, DICOMFLUX_COMPONENT_CONTEXT, "Context allocation failed.");
    } catch (...) {
        return fail(DICOMFLUX_INTERNAL_FAILURE, DICOMFLUX_COMPONENT_CONTEXT, "Context creation failed.");
    }
}
void dicomflux_context_retain(dicomflux_context* context) noexcept { core::retain(context); }
void dicomflux_context_release(dicomflux_context* context) noexcept { core::release(context); }

dicomflux_status dicomflux_error_copy(const dicomflux_error* error, char* output,
                                     size_t capacity, size_t* required) noexcept {
    if (!required) return DICOMFLUX_INVALID_ARGUMENT;
    *required = 0;
    const auto valid = core::validate_error(error);
    if (valid != DICOMFLUX_OK) return valid;
    if (!output && capacity) return DICOMFLUX_INVALID_ARGUMENT;
    *required = static_cast<size_t>(error->diagnostic_length) + 1;
    if (!output) return DICOMFLUX_OK;
    if (capacity < *required) return DICOMFLUX_BUFFER_TOO_SMALL;
    std::memcpy(output, error->diagnostic, *required);
    return DICOMFLUX_OK;
}
}
