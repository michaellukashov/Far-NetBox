#pragma once

#ifdef USE_DLMALLOC
#include <dlmalloc/malloc-2.8.6.h>
#endif

#ifdef USE_DLMALLOC
#define nb_malloc(size) dlmalloc(size)
#define nb_calloc(count,size) dlcalloc(count,size)
#define nb_realloc(ptr,size) dlrealloc(ptr,size)
#define nb_free(ptr) dlfree(ptr)
#else
#define nb_malloc(size) malloc(size)
#define nb_calloc(count,size) calloc(count,size)
#define nb_realloc(ptr,size) realloc(ptr,size)
#define nb_free(ptr) free(ptr)
#endif

