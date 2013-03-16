#ifndef RDESTL_INTRUSIVE_SLIST_H
#define RDESTL_INTRUSIVE_SLIST_H

#include "iterator.h"
#include "type_traits.h"

namespace rde
{

//=============================================================================
struct intrusive_slist_node
{
	intrusive_slist_node()
	{
		next = this;
	}
	bool in_list() const	{ return this != next; }

	intrusive_slist_node*	next;
};

//=============================================================================
template<typename Pointer, typename Reference>
class intrusive_slist_iterator
{
public:
	typedef Pointer					pointer;
	typedef Reference				reference;
	typedef forward_iterator_tag	iterator_category;

	intrusive_slist_iterator():	m_node(0) {/**/}
	explicit intrusive_slist_iterator(Pointer iterNode)
	:	m_node(iterNode)
	{
		/**/
	}

	Reference operator*() const
	{
		RDE_ASSERT(m_node);
		return *m_node;
	}
	Pointer operator->() const
	{
		return m_node;
	}
	Pointer node() const
	{
		return m_node;
	}
	intrusive_slist_node* next() const
	{
		return m_node->next;
	}

	intrusive_slist_iterator& operator++()
	{
		m_node = static_cast<Pointer>(m_node->next);
		return *this;
	}
	intrusive_slist_iterator operator++(int)
	{
		intrusive_slist_iterator copy(*this);
		++(*this);
		return copy;
	}

	bool operator==(const intrusive_slist_iterator& rhs) const
	{
		return rhs.m_node == m_node;
	}
	bool operator!=(const intrusive_slist_iterator& rhs) const
	{
		return !(rhs == *this);
	}

private:
	Pointer	m_node;
};

//=============================================================================
class intrusive_slist_base
{
public:
	typedef size_t	size_type;

	intrusive_slist_base();
	void pop_front()
	{
		unlink_after(&m_root);
	}
	// @note: allow for constant complexity way of checking this
	// (at a cost of additional variable)?
	size_type size() const;
	bool empty() const	{ return !m_root.in_list(); }

protected:
	static void link_after(intrusive_slist_node* node, intrusive_slist_node* prevNode);
	static void unlink_after(intrusive_slist_node* node);

	intrusive_slist_node	m_root;

private:
	intrusive_slist_base(const intrusive_slist_base&);
	intrusive_slist_base& operator=(const intrusive_slist_base&);
};

//=============================================================================
// Can store pointers only!
template<class T>
class intrusive_slist : public intrusive_slist_base
{
public:
	typedef T												node_type;
	typedef T												value_type;
	typedef intrusive_slist_iterator<T*, T&>				iterator;
	typedef intrusive_slist_iterator<const T*, const T&>	const_iterator;

	void push_front(value_type* v)
	{
		link_after(v, &m_root);
	}

	iterator begin()				{ return iterator(upcast(m_root.next)); }
	const_iterator begin() const	{ return const_iterator(upcast(m_root.next)); }
	iterator end()					{ return iterator(upcast(&m_root)); }
	const_iterator end() const		{ return const_iterator(upcast(&m_root)); }

	value_type* front() 			{ return upcast(m_root.next); }
	const value_type* front() const	{ return upcast(m_root.next); }

	// @note use insert_after if possible, it's quicker.
	iterator insert(iterator pos, value_type* v)
	{
		return insert_after(previous(pos), v);
	}
	iterator insert_after(iterator pos, value_type* v)
	{
		link_after(v, pos.node());
		return iterator(v);
	}
	// @note use erase_after if possible, it's quicker.
	iterator erase(iterator pos)
	{
		pos = previous(pos);
		erase_after(pos);
		return ++pos;
	}
	void erase_after(iterator pos)
	{
		unlink_after(pos.node());
	}

	// @return iterator prev such that ++prev == nextIt
	static iterator previous(iterator nextIt)
	{
		RDE_ASSERT(nextIt.node()->in_list());
		iterator prevIt = nextIt;
		while (nextIt.node() != prevIt.next())
			++prevIt;
		return prevIt;
	}
	static const_iterator previous(const_iterator nextIt)
	{
		RDE_ASSERT(nextIt.node()->in_list());
		const_iterator prevIt = nextIt;
		while (nextIt.node() != prevIt.next())
			++prevIt;
		return prevIt;
	}
	// O(1)
	static iterator get_iterator(value_type* v)
	{
		RDE_ASSERT(v->in_list());
		return iterator(v);
	}
	// O(1)
	static const_iterator get_iterator(const value_type* v) 
	{
		RDE_ASSERT(v->in_list());
		return const_iterator(v);
	}

	iterator erase(iterator first, iterator last)
	{
		if (first != last)
		{
			while (first.next() != last.node())
				erase_after(first);
			erase_after(previous(first));
		}
		return first;
	}

	void clear()
	{
		erase(begin(), end());
	}

	// O(1)
	static void remove(value_type* v)
	{
		unlink(v);
	}

private:
	static RDE_FORCEINLINE node_type* upcast(intrusive_slist_node* n)
	{
		return static_cast<node_type*>(n);
	}
	static RDE_FORCEINLINE const node_type* upcast(const intrusive_slist_node* n)
	{
		return static_cast<const node_type*>(n);
	}
};

} // namespace rde

//-----------------------------------------------------------------------------
#endif // #ifndef RDESTL_INTRUSIVE_SLIST_H
