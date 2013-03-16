#ifndef RDESTL_SORT_H
#define RDESTL_SORT_H

#include "functional.h"

namespace rde
{
namespace internal
{
template<typename T, class TPredicate>
void quick_sort(T* data, long low, long high, TPredicate pred)
{
	while (true)
	{
		long i = low;
		long j = high;
		const T pivot = data[(low + high) >> 1];
		do
		{
			// Jump over elements that are OK (smaller than pivot)
			while (pred(data[i], pivot))
				++i;
			// Jump over elements that are OK (greater than pivot)
			while (pred(pivot, data[j]))
				--j;
			// Anything to swap?
			if (j >= i)
			{
				if (i != j)
				{
					// Swap
					T tmp(data[i]);
					data[i] = data[j];
					data[j] = tmp;
				}
				++i;
				--j;
			}
		}
		while (i <= j);

		if (low < j)
			quick_sort(data, low, j, pred);

		if (i < high)
			low = i;	// that's basically quick_sort(data, i, high, pred), but we avoid recursive call.
		else
			break;
	} // while (true)
}

template<typename T, class TPredicate>
void down_heap(T* data, size_t k, size_t n, TPredicate pred)
{
	const T temp = data[k - 1];
	while (k <= n / 2)
	{
		size_t child = 2 * k;
		if (child < n && pred(data[child - 1], data[child]))
			++child;
		if (pred(temp, data[child - 1]))
		{
			data[k - 1] = data[child - 1];
			k = child;
		}
		else
			break;
	}
	data[k - 1] = temp;
}

} // internal

template<typename T, class TPredicate>
void insertion_sort(T* begin, T* end, TPredicate pred)
{
	const size_t num = end - begin;	
	for (size_t i = 0; i < num; ++i)
	{
		const T t = begin[i];
		size_t j = i;
		while (j > 0 && pred(t, begin[j - 1]))
		{
			begin[j] = begin[j - 1];
			--j;
		}
		begin[j] = t;
	}
}
template<typename T>
void insertion_sort(T* begin, T* end)
{
	insertion_sort(begin, end, less<T>());
}

template<typename T, class TPredicate>
void quick_sort(T* begin, T* end, TPredicate pred)
{
	if (end - begin > 1)
		internal::quick_sort(begin, 0, (long)(end - begin - 1), pred);
}
template<typename T>
void quick_sort(T* begin, T* end)
{
	quick_sort(begin, end, less<T>());
}

template<typename T, class TPredicate>
void heap_sort(T* begin, T* end, TPredicate pred)
{
	size_t n = end - begin;
	for (size_t k = n / 2; k != 0; --k)
		internal::down_heap(begin, k, n, pred);

	while (n >= 1)
	{
		const T temp = begin[0];
		begin[0] = begin[n - 1];
		begin[n - 1] = temp;
		
		--n;
		internal::down_heap(begin, 1, n, pred);
	}
}
template<typename T>
void heap_sort(T* begin, T* end)
{
	heap_sort(begin, end, rde::less<T>());
}

// True if given set of data is sorted according to given predicate.
// Ie, for every pair of objects x = data[i], y = data[i + 1], pred(y, x) doesn't return true;
template<typename TIter, typename TPredicate>
bool is_sorted(TIter begin, TIter end, TPredicate pred)
{
	TIter it = begin;
	TIter it_prev = it;
	bool is_sorted = true;
	while (it != end)
	{
		if (it_prev != it)
		{
			if (pred(*it, *it_prev))
			{
				is_sorted = false;
				break;
			}
		}
		it_prev = it;
		++it;
	}
	return is_sorted;
}

} // rde

#endif 
