#ifndef RDESTL_STACK_H
#define RDESTL_STACK_H

#include "vector.h"

namespace rde
{
// Stack implemented on top of another container (vector by default).
template<typename T, class TAllocator = rde::allocator,
	class TContainer = rde::vector<T, TAllocator> >
class stack
{
public:
	typedef TContainer						container_type;
	typedef typename TContainer::value_type	value_type;
	typedef typename TContainer::size_type	size_type;
	typedef TAllocator						allocator_type;

	explicit stack(const allocator_type& allocator = allocator_type())
	:	m_container(allocator)
	{
	}
	stack(const stack& rhs, const allocator_type& allocator = allocator_type())
	:	m_container(rhs, allocator)
	{
	}
	explicit stack(e_noinitialize n)
	:	m_container(n)
	{
	}

	// @note	Doesnt copy allocator from rhs.
	stack& operator=(const stack& rhs)
	{
		if (&rhs != this)
		{
			m_container = rhs.m_container;
		}
		return *this;
	}

	void push(const value_type& v)
	{
		m_container.push_back(v);
	}
	void pop()
	{
		m_container.pop_back();
	}
	value_type& top()				{ return m_container.back(); }
	const value_type& top() const	{ return m_container.back(); }

	// @extension
	void clear()			{ m_container.clear(); }

	bool empty() const		{ return m_container.empty(); }
	size_type size() const	{ return m_container.size(); }

	const allocator_type& get_allocator() const	
	{ 
		return m_container.get_allocator(); 
	}
	void set_allocator(const allocator_type& allocator)
	{
		m_container.set_allocator(allocator);
	}

private:
	TContainer	m_container;
};

} // rde

#endif
