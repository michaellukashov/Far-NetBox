#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "impl/FastMemcpy.h"

#if 0
void * __attribute__((__weak__)) memcpy(void * __restrict destination, const void * __restrict source, size_t size)
{
    return memcpy_fast(destination, source, size);
}
#endif //if 0

void * libmemcpy_memcpy(void * destination, const void * source, size_t size);

#ifdef __cplusplus
}
#endif
