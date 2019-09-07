#include "memcpy.h"

#ifdef __cplusplus
extern "C"
{
#endif

void * libmemcpy_memcpy(void * destination, const void * source, size_t size)
{
    return memcpy_fast(destination, source, size);
}

#ifdef __cplusplus
}
#endif
