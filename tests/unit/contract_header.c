#include "../../docs/architecture/dicomflux-c-api-candidate.h"
#include <stdio.h>
#ifdef __cplusplus
# define DF_ALIGNOF(T) alignof(T)
static_assert(sizeof(dicomflux_status)==4);
#else
# define DF_ALIGNOF(T) _Alignof(T)
_Static_assert(sizeof(dicomflux_status)==4, "status width");
#endif
int main(void) {
    printf("candidate API declaration-only: context_options=%zu source=%zu sink=%zu error=%zu result=%zu alignment=%zu\n",
           sizeof(dicomflux_context_options),sizeof(dicomflux_source),sizeof(dicomflux_sink),
           sizeof(dicomflux_error),sizeof(dicomflux_execution_result),DF_ALIGNOF(dicomflux_source));
    return 0;
}
