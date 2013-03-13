#ifndef RDESTL_FIXED_SORTED_VECTOR_H
#define RDESTL_FIXED_SORTED_VECTOR_H

#include "fixed_vector.h"
#include "sorted_vector.h"

namespace rde
{
template<typename TKey, typename TValue, 
	int TCapacity, bool TGrowOnOverflow, class TCompare = rde::less<TKey>,
	class TAllocator = rde::allocator>
class fixed_sorted_vector : public sorted_vector<TKey, TValue, TCompare,
	TAllocator,
	fixed_vector_storage<pair<TKey, TValue>, TAllocator, TCapacity, 
		TGrowOnOverflow> >
{
public:
	explicit fixed_sorted_vector(const allocator_type& allocator = allocator_type())
	:	sorted_vector(allocator)
	{
		/**/
	}
};

} // rde

#endif // #ifndef RDESTL_FIXED_SORTED_VECTOR_H

