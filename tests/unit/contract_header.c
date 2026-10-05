#include "../../docs/architecture/dicomflux-c-api-candidate.h"
#include <stdio.h>
#include <string.h>
#ifdef __cplusplus
# define DF_ALIGNOF(T) alignof(T)
static_assert(sizeof(dicomflux_status)==4);
#else
# define DF_ALIGNOF(T) _Alignof(T)
_Static_assert(sizeof(dicomflux_status)==4, "status width");
#endif
int main(void) {
    dicomflux_plan_info info;
    dicomflux_execution_result result;
    dicomflux_error error;
    memset(&info, 0, sizeof(info));
    memset(&result, 0, sizeof(result));
    memset(&error, 0, sizeof(error));
    const uint32_t states[] = {DICOMFLUX_PLAN_NONE, DICOMFLUX_PLAN_READY,
        DICOMFLUX_PLAN_EXECUTING, DICOMFLUX_PLAN_SUCCEEDED,
        DICOMFLUX_PLAN_FAILED, DICOMFLUX_PLAN_CANCELLED};
    const uint32_t components[] = {DICOMFLUX_COMPONENT_NONE, DICOMFLUX_COMPONENT_ABI,
        DICOMFLUX_COMPONENT_CONTEXT, DICOMFLUX_COMPONENT_BUILDER,
        DICOMFLUX_COMPONENT_DATASET, DICOMFLUX_COMPONENT_PLAN,
        DICOMFLUX_COMPONENT_SOURCE, DICOMFLUX_COMPONENT_SINK, DICOMFLUX_COMPONENT_CANCEL};
    const uint32_t operations[] = {DICOMFLUX_OPERATION_NONE, DICOMFLUX_OPERATION_QUERY_ABI,
        DICOMFLUX_OPERATION_CONTEXT_CREATE, DICOMFLUX_OPERATION_CAPABILITIES_COPY,
        DICOMFLUX_OPERATION_ERROR_COPY, DICOMFLUX_OPERATION_BUILDER_CREATE,
        DICOMFLUX_OPERATION_BUILDER_SET, DICOMFLUX_OPERATION_BUILDER_SET_EMPTY_SEQUENCE,
        DICOMFLUX_OPERATION_BUILDER_FREEZE, DICOMFLUX_OPERATION_PLAN_CREATE,
        DICOMFLUX_OPERATION_PLAN_QUERY, DICOMFLUX_OPERATION_CANCEL_CREATE,
        DICOMFLUX_OPERATION_PLAN_EXECUTE};
    size_t i;
    for (i=0; i<sizeof(states)/sizeof(states[0]); ++i) if (states[i]!=i) return 1;
    for (i=0; i<sizeof(components)/sizeof(components[0]); ++i) if (components[i]!=i) return 2;
    for (i=0; i<sizeof(operations)/sizeof(operations[0]); ++i) if (operations[i]!=i) return 3;
    info.header.struct_size = sizeof(info);
    info.header.abi_major = DICOMFLUX_ABI_MAJOR;
    info.header.abi_minor = DICOMFLUX_ABI_MINOR;
    info.state = DICOMFLUX_PLAN_READY;
    info.execution_status = DICOMFLUX_NOT_EXECUTED;
    result.terminal_state = DICOMFLUX_PLAN_CANCELLED;
    result.status = DICOMFLUX_CANCELLED;
    error.component = DICOMFLUX_COMPONENT_SOURCE;
    error.operation = DICOMFLUX_OPERATION_PLAN_EXECUTE;
    error.tag = DICOMFLUX_NO_TAG;
    error.source_offset = DICOMFLUX_UNKNOWN_QUANTITY;
    error.expected = UINT64_MAX;
    error.observed = DICOMFLUX_UNKNOWN_QUANTITY;
    error.detail_flags = DICOMFLUX_ERROR_HAS_EXPECTED;
    if (sizeof(info.state)!=4 || sizeof(info.execution_status)!=4 ||
        info.execution_status!=-1 || info.header.abi_minor!=1 ||
        result.terminal_state!=5 || result.status!=8 ||
        error.component!=6 || error.operation!=12 || error.tag!=UINT32_MAX ||
        error.source_offset!=UINT64_MAX || error.observed!=UINT64_MAX ||
        (error.detail_flags & DICOMFLUX_ERROR_HAS_EXPECTED)==0 ||
        (error.detail_flags & DICOMFLUX_ERROR_HAS_OBSERVED)!=0 ||
        (DICOMFLUX_ERROR_HAS_TAG | DICOMFLUX_ERROR_HAS_SOURCE_OFFSET |
         DICOMFLUX_ERROR_HAS_EXPECTED | DICOMFLUX_ERROR_HAS_OBSERVED)!=DICOMFLUX_ERROR_DETAIL_MASK)
        return 4;
    printf("candidate API declaration-only: context_options=%zu source=%zu sink=%zu error=%zu result=%zu alignment=%zu\n",
           sizeof(dicomflux_context_options),sizeof(dicomflux_source),sizeof(dicomflux_sink),
           sizeof(dicomflux_error),sizeof(dicomflux_execution_result),DF_ALIGNOF(dicomflux_source));
    return 0;
}
