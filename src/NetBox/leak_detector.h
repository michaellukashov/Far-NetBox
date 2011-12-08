#pragma once

#define GC_DEBUG
// #include "gc.h"
// #undef _GLOBAL_USING
#define GC_API extern
#include "gc_cpp.h"

// #define malloc(n) GC_MALLOC(n)
// #define calloc(m,n) GC_MALLOC((m)*(n))
// #define free(p) GC_FREE(p)
// #define realloc(p,n) GC_REALLOC((p),(n))
#define CHECK_LEAKS() GC_gcollect()
