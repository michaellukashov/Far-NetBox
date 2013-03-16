#ifndef RDESTL_BUFFER_ALLOCATOR_H
#define RDESTL_BUFFER_ALLOCATOR_H

#include "rdestl_common.h"

namespace rde
{

// CONCEPT!
class buffer_allocator
{
public:
	explicit buffer_allocator(const char* name, char* mem,
		size_t bufferSize)
	:	m_name(name), 
		m_buffer(mem),
		m_bufferTop(0),
		m_bufferSize(bufferSize)
	{
		/**/
	}

	void* allocate(size_t bytes, int /*flags*/ = 0)
	{
		RDE_ASSERT(m_bufferTop + bytes <= m_bufferSize);
		char* ret = m_buffer + m_bufferTop;
		m_bufferTop += bytes;
		return ret;
	}
	void deallocate(void* ptr, size_t /*bytes*/)
	{
		RDE_ASSERT(ptr == 0 || (ptr >= m_buffer && ptr < m_buffer + m_bufferSize));
		sizeof(ptr);
	}

	const char* get_name() const	{ return m_name; }

private:
	const char*	m_name;
	char*		m_buffer;
	size_t		m_bufferTop;
	size_t		m_bufferSize;
};

} // namespace rde
//-----------------------------------------------------------------------------
#endif // #ifndef RDESTL_BUFFER_ALLOCATOR_H
