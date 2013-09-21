#pragma once

#ifdef USE_DLMALLOC
#include "../../libs/dlmalloc/malloc-2.8.6.h"

#define nb_malloc(size) dlmalloc(size)
#define nb_calloc(count, size) dlcalloc(count, size)
#define nb_realloc(ptr, size) dlrealloc(ptr, size)

#if defined(__cplusplus)
#define nb_free(ptr) dlfree(reinterpret_cast<void *>(ptr))
#else
#define nb_free(ptr) dlfree((void *)(ptr))
#endif // if defined(__cplusplus)

#else

#define nb_malloc(size) malloc(size)
#define nb_calloc(count, size) calloc(count, size)
#define nb_realloc(ptr, size) realloc(ptr, size)

#if defined(__cplusplus)
#define nb_free(ptr) free(reinterpret_cast<void *>(ptr))
#else
#define nb_free(ptr) free((void *)(ptr))
#endif // if defined(__cplusplus)

#endif // ifdef USE_DLMALLOC
