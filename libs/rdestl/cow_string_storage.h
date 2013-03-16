#ifndef RDESTL_COW_STRING_STORAGE_H
#define RDESTL_COW_STRING_STORAGE_H

namespace rde
{
//=============================================================================
struct string_rep
{
	// @todo: thread-safe version.
	void add_ref() { ++refs; }
	bool release() { --refs; return refs <= 0; }

	void init(short new_capacity = 0)
	{
		refs = 1;
		size = 0;
		capacity = new_capacity;
	}

	short	refs;
	short	size;
	short	capacity;
	// [data]
	static const size_t kMaxCapacity = (1 << (sizeof(short) << 3)) >> 1;
};
typedef char ERR_InvalidStringRepSize[sizeof(string_rep) == 6 ? 1 : -1];

//=============================================================================
// Copy On Write storage.
template<typename E, class TAllocator>
class cow_string_storage
{
	typedef char ERR_CharTypeTooBigSeeM_BufferComment[sizeof(E) <= 2 ? 1 : -1];
public:
	typedef E					value_type;
	typedef int					size_type;
	typedef TAllocator			allocator_type;
	typedef const value_type*	const_iterator;
	static const unsigned long	kGranularity = 32;	

	explicit cow_string_storage(const allocator_type& allocator)
	:	m_allocator(allocator)
	{
		construct_string(0);
	}
	cow_string_storage(const value_type* str, const allocator_type& allocator)
	:	m_allocator(allocator)
	{
		const int len = strlen(str);
		construct_string(len);
		Sys::MemCpy(m_data, str, len*sizeof(value_type));
		RDE_ASSERT(len < string_rep::kMaxCapacity);
		get_rep()->size = static_cast<short>(len);
		m_data[len] = 0;
	}
	cow_string_storage(const value_type* str, size_type len, 
		const allocator_type& allocator)
	:	m_allocator(allocator)
	{
		construct_string(len);
		Sys::MemCpy(m_data, str, len*sizeof(value_type));
		RDE_ASSERT(len < string_rep::kMaxCapacity);
		get_rep()->size = static_cast<short>(len);
		m_data[len] = 0;
	}
	cow_string_storage(const cow_string_storage& rhs, const allocator_type& allocator)
	:	m_data(rhs.m_data),
		m_allocator(allocator)
	{
		if (rhs.is_dynamic())
		{
			get_rep()->add_ref();
		}
		else
		{
			const int len = rhs.length();
			construct_string(len);
			Sys::MemCpy(m_data, rhs.c_str(), len*sizeof(value_type));
			RDE_ASSERT(len < string_rep::kMaxCapacity);
			get_rep()->size = static_cast<short>(len);
			m_data[len] = 0;
		}
	}
	~cow_string_storage()
	{
		if (!is_dynamic())
			RDE_ASSERT(get_rep()->refs == 1);
		release_string();
	}

	// @note: doesnt copy allocator
	cow_string_storage& operator=(const cow_string_storage& rhs)
	{
		if (m_data != rhs.m_data)
		{
			release_string();
			m_data = rhs.m_data;
			get_rep()->add_ref();
		}
		return *this;
	}

	void assign(const value_type* str, size_type len)
	{
		// Do not use with str = str2.c_str()!
		RDE_ASSERT(str != m_data);
		release_string();
		construct_string(len);
		Sys::MemCpy(m_data, str, len*sizeof(value_type));
		get_rep()->size = short(len);
		m_data[len] = 0;
	}

	void append(const value_type* str, size_type len)
	{
		const size_type prevLen = length();
		const size_type newCapacity = prevLen + len;
		make_unique(newCapacity);
		string_rep* rep = get_rep();
		RDE_ASSERT(rep->capacity >= short(newCapacity));
		const size_type newLen = prevLen + len;
		RDE_ASSERT(short(newLen) <= rep->capacity);
		Sys::MemCpy(m_data + prevLen, str, len * sizeof(value_type));
		m_data[newLen] = 0;
		rep->size = short(newLen);
	}

	inline const value_type* c_str() const
	{
		return m_data;
	}
	inline size_type length() const
	{
		return get_rep()->size;
	}
	const allocator_type& get_allocator() const	{ return m_allocator; }

	value_type* reserve(size_type capacity_hint)
	{
		make_unique(capacity_hint);
		return m_data;
	}
	void clear()
	{
		resize(0);
	}
	void resize(size_type size)
	{
		string_rep* rep = get_rep();
		make_unique(size);
		rep->size = (short)size;
		m_data[size] = 0;
	}

protected:
	bool invariant() const
	{
		RDE_ASSERT(m_data);
		string_rep* rep = get_rep();
		RDE_ASSERT(rep->refs >= 1);
		RDE_ASSERT(rep->size <= rep->capacity);
		if (length() != 0)
			RDE_ASSERT(m_data[length()] == 0);
		return true;
	}
	void make_unique(size_type capacity_hint)
	{
		string_rep* rep = get_rep();
		RDE_ASSERT(rep->refs >= 1);

		if (capacity_hint != 0)
		{
			++capacity_hint;
			capacity_hint = (capacity_hint+kGranularity-1) & ~(kGranularity-1);
			if (capacity_hint < kGranularity)
				capacity_hint = kGranularity;
		}
		RDE_ASSERT(capacity_hint < string_rep::kMaxCapacity);
		// Reallocate string only if we truly need to make it unique
		// (it's shared) or if our current buffer is too small.
		if (rep->refs > 1 || short(capacity_hint) > rep->capacity)
		{
			if (capacity_hint > 0)
			{
				const size_type toAlloc = sizeof(string_rep) + sizeof(value_type)*capacity_hint;
				void* newMem = m_allocator.allocate(toAlloc);
				string_rep* newRep = reinterpret_cast<string_rep*>(newMem);
				newRep->init(short(capacity_hint));
				value_type* newData = reinterpret_cast<value_type*>(newRep + 1);
				Sys::MemCpy(newData, m_data, rep->size*sizeof(value_type));
				newRep->size = rep->size;
				newData[rep->size] = 0;
				release_string();
				m_data = newData;
			}
			else
			{
				release_string();
				string_rep* rep = reinterpret_cast<string_rep*>(m_buffer);
				rep->init();
				m_data = reinterpret_cast<value_type*>(rep + 1);
				*m_data = 0;
			}
		}
	}
	RDE_FORCEINLINE E* get_data()	{ return m_data; }

private:
	RDE_FORCEINLINE string_rep* get_rep() const
	{
		return reinterpret_cast<string_rep*>(m_data) - 1;
	}
	void construct_string(size_type capacity)
	{
		if (capacity != 0)
		{
			++capacity;
			capacity = (capacity+kGranularity-1) & ~(kGranularity-1);
			if (capacity < kGranularity)
				capacity = kGranularity;
			RDE_ASSERT(capacity < string_rep::kMaxCapacity);

			const size_type toAlloc = sizeof(string_rep) + sizeof(value_type)*capacity;
			void* mem = m_allocator.allocate(toAlloc);
			string_rep* rep = reinterpret_cast<string_rep*>(mem);
			rep->init(static_cast<short>(capacity));
			m_data = reinterpret_cast<value_type*>(rep + 1);
		}
		else	// empty string, no allocation needed. Use our internal buffer.
		{
			string_rep* rep = reinterpret_cast<string_rep*>(m_buffer);
			rep->init();
			m_data = reinterpret_cast<value_type*>(rep + 1);
		}
		*m_data = 0;
	}
	void release_string()
	{
		string_rep* rep = get_rep();
		if (rep->release() && rep->capacity != 0)
		{
			// Make sure it was a dynamically allocated data.
			RDE_ASSERT(is_dynamic());
			m_allocator.deallocate(rep, rep->capacity);
		}
	}
	bool is_dynamic() const
	{
		return get_rep() != reinterpret_cast<const string_rep*>(&m_buffer[0]);
	}

	E*			m_data;
	// @note: hack-ish. sizeof(string_rep) bytes for string_rep, than place for terminating
	// character (up to 2-bytes!)
	char		m_buffer[sizeof(string_rep)+2]; 
	TAllocator	m_allocator;
};

} // rde

#endif // RDESTL_COW_STRING_STORAGE_H
