#ifndef RDESTL_SIMPLE_STRING_STORAGE_H
#define RDESTL_SIMPLE_STRING_STORAGE_H

namespace rde
{
template<typename E, class TAllocator>
class simple_string_storage
{
public:
	typedef E					value_type;
	typedef int					size_type;
	typedef TAllocator			allocator_type;
	typedef const value_type*	const_iterator;
	static const unsigned long	kGranularity = 32;	

	explicit simple_string_storage(const allocator_type& allocator)
	:	m_length(0),
		m_allocator(allocator)
	{
		m_data = construct_string(0, m_capacity);
	}
	simple_string_storage(const value_type* str, const allocator_type& allocator)
	:	m_allocator(allocator)
	{
		const int len = strlen(str);
		m_data = construct_string(len, m_capacity);
		Sys::MemCpy(m_data, str, len*sizeof(value_type));
		m_length = len;
		m_data[len] = 0;
	}
	simple_string_storage(const value_type* str, size_type len, 
		const allocator_type& allocator)
	:	m_allocator(allocator)
	{
		m_data = construct_string(len, m_capacity);
		Sys::MemCpy(m_data, str, len*sizeof(value_type));
		m_length = len;
		m_data[len] = 0;
	}
	simple_string_storage(const simple_string_storage& rhs, const allocator_type& allocator)
	:	m_data(0),
		m_capacity(0), 
		m_length(0),
		m_allocator(allocator)
	{
		assign(rhs.c_str(), rhs.length());
	}
	~simple_string_storage()	
	{
		release_string();
	}

	// @note: doesnt copy allocator
    simple_string_storage& operator=(const simple_string_storage& rhs)
	{
		if (m_data != rhs.c_str()) 
		{
			assign(rhs.c_str(), rhs.length());
		}
		return *this;
	}
        
	void assign(const value_type* str, size_type len)
	{
		// Do not use with str = str.c_str()!
		RDE_ASSERT(str != m_data);
		if (m_capacity <= len + 1)
		{
			release_string();
			m_data = construct_string(len, m_capacity);
		}
		Sys::MemCpy(m_data, str, len*sizeof(value_type));
		m_length = len;
		m_data[len] = 0;
	}

	void append(const value_type* str, size_type len)
	{
		const size_type prevLen = length();
		const size_type newLen = prevLen + len;
		if (m_capacity <= newLen + 1)
		{
			size_type newCapacity;
			value_type* newData = construct_string(newLen, newCapacity);
			Sys::MemCpy(newData, m_data, prevLen * sizeof(value_type));
			release_string();
            m_data = newData;
			m_capacity = newCapacity;
		}
		Sys::MemCpy(m_data + prevLen, str, len * sizeof(value_type));
		m_data[newLen] = 0;
		m_length = newLen;
		RDE_ASSERT(invariant());
	}

	inline const value_type* c_str() const
	{
		return m_data;
	}
	
    inline size_type length() const
	{
		return m_length;
	}
    
    inline size_type capacity() const { return m_capacity; }
    
    void clear() 
    {
        release_string();
        m_data = construct_string(0, m_capacity);
        m_length = 0;
    }
    
	const allocator_type& get_allocator() const	{ return m_allocator; }

	void make_unique(size_type) {}
	value_type* get_data() { return m_data; }

protected:
	bool invariant() const
	{
		RDE_ASSERT(m_data);
		RDE_ASSERT(m_length <= m_capacity);
		if (length() != 0)
			RDE_ASSERT(m_data[length()] == 0);
		return true;
	}
private:
	value_type* construct_string(size_type capacity, size_type& out_capacity)
	{  
        value_type* data(0);
        if (capacity != 0)
        {
            capacity = (capacity+kGranularity-1) & ~(kGranularity-1);
            if (capacity < kGranularity)
                capacity = kGranularity;

            const size_type toAlloc = sizeof(value_type)*(capacity + 1);
            void* mem = m_allocator.allocate(toAlloc);
            data = static_cast<value_type*>(mem);
        }
        else	// empty string, no allocation needed. Use our internal buffer.
        {
            data = &m_end_of_data;
        }
    
		out_capacity = capacity;
		*data = 0;
		return data;
	}
	void release_string()
	{
		if (m_capacity != 0)
		{
			RDE_ASSERT(m_data != &m_end_of_data);
			m_allocator.deallocate(m_data, m_capacity);
		}
	}
    
	E*			m_data;
	E			m_end_of_data;
	size_type	m_capacity;
	size_type	m_length;
	TAllocator	m_allocator;
};

} // rde

#endif // #ifndef RDESTL_SIMPLE_STRING_STORAGE_H
