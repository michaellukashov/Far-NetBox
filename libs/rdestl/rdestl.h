#ifndef RDESTL_H
#define RDESTL_H

#include "rdestl_common.h"
#include "vector.h"
#include "hash_map.h"
#include "rde_string.h"
#include "stack.h"
#include "algorithm.h"
#include "functional.h"
#include "list.h"
#include "pair.h"
#include "sort.h"
#include "utility.h"
#include "sstream.h"

namespace rde
{
    template<typename T, class TAllocator = rde::allocator,
                class TStorage = standard_vector_storage<T, TAllocator> >
    struct deque : public vector<T,TAllocator,TStorage>
    {
        // TODO
    };
};
#endif
