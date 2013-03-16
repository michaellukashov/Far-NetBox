#ifndef RDESTL_FIXED_VECTOR_H
#define RDESTL_FIXED_VECTOR_H

#include "alignment.h"
#include "vector.h"

#define RDESTL_RECORD_WATERMARKS	0

// @TODO Wont work on 64-bit!
// 4267 -- conversion from size_t to int.
#pragma warning(push)
#pragma warning(disable: 4267)

namespace rde
{
//=============================================================================
template<typename T, class TAllocator, int TCapacity, bool TGrowOnOverflow>
struct fixed_vector_storage 
{
	explicit fixed_vector_storage(const TAllocator& allocator)
	:	m_begin((T*)&m_data[0]),
		m_end(m_begin),
		m_capacityEnd(m_begin + TCapacity),
		m_allocator(allocator)
#if RDESTL_RECORD_WATERMARKS
		, m_max_size(0)
#endif
	{
		/**/
	}	
	explicit fixed_vector_storage(e_noinitialize)
	{
	}

	// @note	Cant shrink
	void reallocate(base_vector::size_type newCapacity, base_vector::size_type oldSize)
	{
		if (!TGrowOnOverflow)
		{
			RDE_ASSERT(!"fixed_vector cannot grow");
			// @TODO: do something more spectacular here... do NOT throw exception, tho :)
		}
		T* newBegin = static_cast<T*>(m_allocator.allocate(newCapacity * sizeof(T)));
		const base_vector::size_type newSize = oldSize < newCapacity ? oldSize : newCapacity;
		// Copy old data if needed.
		if (m_begin)
		{
			rde::copy_construct_n(m_begin, newSize, newBegin);
			destroy(m_begin, oldSize);
		}
		m_begin = newBegin;
		m_end = m_begin + newSize;
		m_capacityEnd = m_begin + newCapacity;
		record_high_watermark();
		RDE_ASSERT(invariant());
	}

	// Reallocates memory, doesnt copy contents of old buffer.
	void reallocate_discard_old(base_vector::size_type newCapacity)
	{
		if (newCapacity > base_vector::size_type(m_capacityEnd - m_begin))
		{
			if (!TGrowOnOverflow)
			{
				RDE_ASSERT(!"fixed_vector cannot grow");
			}
			T* newBegin = static_cast<T*>(m_allocator.allocate(newCapacity * sizeof(T)));
			const base_vector::size_type currSize((size_t)(m_end - m_begin));
			if (m_begin)
				destroy(m_begin, currSize);
			m_begin = newBegin;
			m_end = m_begin + currSize;
			record_high_watermark();
			m_capacityEnd = m_begin + newCapacity;
		}
		RDE_ASSERT(invariant());
	}
	RDE_FORCEINLINE void destroy(T* ptr, base_vector::size_type n)
	{
		rde::destruct_n(ptr, n);
		if ((etype_t*)ptr != &m_data[0])
			m_allocator.deallocate(ptr, n * sizeof(T));
	}
	bool invariant() const
	{
		return m_end >= m_begin;
	}
	RDE_FORCEINLINE void record_high_watermark()
	{
#if RDESTL_RECORD_WATERMARKS
		const base_vector::size_type curr_size((size_t)(m_end - m_begin));
		if (curr_size > m_max_size)
			m_max_size = curr_size;
#endif
	}
	base_vector::size_type get_high_watermark() const
	{
#if RDESTL_RECORD_WATERMARKS
		return m_max_size;
#else
		return TCapacity;	// ???
#endif
	}

	typedef typename aligned_as<T>::res	etype_t;	
	
	T*						m_begin;
	T*						m_end;
	// Not T[], because we need uninitialized memory.
	etype_t					m_data[(TCapacity * sizeof(T)) / sizeof(etype_t)];
	T*						m_capacityEnd;
	TAllocator				m_allocator;
#if RDESTL_RECORD_WATERMARKS
	base_vector::size_type	m_max_size;
#endif
};

//=============================================================================
template<typename T, int TCapacity, bool TGrowOnOverflow,
	class TAllocator = rde::allocator>
class fixed_vector : public vector<T, TAllocator, 
	fixed_vector_storage<T, TAllocator, TCapacity, TGrowOnOverflow> >
{
	typedef vector<T, TAllocator, 
		fixed_vector_storage<T, TAllocator, TCapacity, TGrowOnOverflow> > base_vector;
    typedef TAllocator allocator_type;
    typedef typename base_vector::size_type size_type;
    typedef T value_type;
public:
	explicit fixed_vector(const allocator_type& allocator = allocator_type())
	:	base_vector(allocator)
	{
		/**/
	}
	explicit fixed_vector(size_type initialSize, const allocator_type& allocator = allocator_type())
	:	base_vector(initialSize, allocator)
	{
		/**/
	}
	fixed_vector(const T* first, const T* last, const allocator_type& allocator = allocator_type())
	:	base_vector(first, last, allocator)
	{
		/**/
	}
	// @note: allocator is not copied from rhs.
	// @note: will not perform default constructor for newly created objects.
	fixed_vector(const fixed_vector& rhs, const allocator_type& allocator = allocator_type())
	:	base_vector(rhs, allocator)
	{
		/**/
	}
	explicit fixed_vector(e_noinitialize n)
	:	base_vector(n)
	{
		/**/
	}

	fixed_vector& operator=(const fixed_vector& rhs)
	{
		if (&rhs != this)
		{
            base_vector::copy(rhs);
		}
		return *this;
	}
};

#pragma warning(pop)

} // rde

#endif // #ifndef RDESTL_FIXED_VECTOR_H
