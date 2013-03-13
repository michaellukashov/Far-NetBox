#ifndef RDESTL_STRING_H
#define RDESTL_STRING_H

#include "basic_string.h"
#include "hash.h"

namespace rde
{
typedef basic_string<char>	string;

template<typename E, class TAllocator, typename TStorage>
struct hash<basic_string<E, TAllocator, TStorage> >
{
    hash_value_t operator()(const basic_string<E, TAllocator, TStorage>& x) const 
    {
        // Derived from: http://blade.nagaokaut.ac.jp/cgi-bin/scat.rb/ruby/ruby-talk/142054
        hash_value_t h = 0;
		for (basic_string<E, TAllocator, TStorage>::size_type p = 0; p < x.length(); ++p) 
		{
            h = x[p] + (h<<6) + (h<<16) - h;
        }
        return h & 0x7FFFFFFF;
    }
};

}

#endif // RDESTL_STRING_H
