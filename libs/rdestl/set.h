#ifndef RDESTL_SET_H
#define RDESTL_SET_H

#include "rb_tree.h"
#include "iterator.h"

namespace rde
{
template<typename T,
	class TAllocator = rde::allocator>
class set : private rb_tree<T, TAllocator>
{
	typedef rb_tree<T, TAllocator>  Base;
    typedef typename Base::node     BaseNode;

	template<typename TNodePtr, typename TPtr, typename TRef>
	class node_iterator
	{
	public:
		typedef forward_iterator_tag	iterator_category;

		explicit node_iterator(TNodePtr node, set* set_)
		:	m_node(node),
			m_set(set_)
		{/**/}
		template<typename UNodePtr, typename UPtr, typename URef>
		node_iterator(const node_iterator<UNodePtr, UPtr, URef>& rhs)
		:	m_node(rhs.node()),
			m_set(rhs.get_set())
		{
			/**/
		}

		TRef operator*() const
		{
			RDE_ASSERT(m_node != 0);
			return m_node->key;
		}
		TPtr operator->() const
		{
			return &m_node->key;
		}
		TNodePtr node() const
		{
			return m_node;
		}

		node_iterator& operator++() 
		{
			RDE_ASSERT(m_node != 0);
			TNodePtr next = m_node->next;
			if (next == 0)
				next = find_next_node(m_node);
			m_node = next;
			return *this;
		} 
		node_iterator operator++(int)
		{
			node_iterator copy(*this);
			++(*this);
			return copy;
		}

		bool operator==(const node_iterator& rhs) const
		{
			return rhs.m_node == m_node && m_set == rhs.m_set;
		}
		bool operator!=(const node_iterator& rhs) const
		{
			return !(rhs == *this);
		}

		set* get_set() const { return m_set; }
	private:
		TNodePtr find_next_node(TNodePtr node) const
		{
			return 0;
		}

		TNodePtr	m_node;
		set*		m_set;
	};

public:
	typedef T															value_type;
	typedef node_iterator<BaseNode*, value_type*, value_type&>				iterator;
	typedef node_iterator<BaseNode*, const value_type*, const value_type&>	const_iterator;
	typedef TAllocator													allocator_type;

	explicit set(const allocator_type& allocator = allocator_type())
	:	Base(allocator) {}

	iterator begin()
	{
		return iterator(Base::get_begin_node(), this);
	}
	const_iterator begin() const
	{
		return const_iterator(Base::get_begin_node(), this);
	}
	iterator end()
	{
		return iterator(0, this);
	}
	const_iterator end() const
	{
		return iterator(0, this);
	}

	iterator find(const value_type& v)	
	{ 
		return iterator(Base::find_node(v), this); 
	}
	const_iterator find(const value_type& v) const
	{
		return const_iterator(Base::find_node(v), this);
	}
	void erase(const value_type& v)
	{
		erase(find(v));
	}
	void erase(iterator it)
	{
		RDE_ASSERT(it != end());
		Base::erase(it.node());
	}

	using Base::insert;
	using Base::empty;
	using Base::size;
};

} // rde

#endif // 
