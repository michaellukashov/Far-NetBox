#ifndef RDESTL_LIST_H
#define RDESTL_LIST_H

#include "allocator.h"
#include "iterator.h"

namespace rde
{
namespace internal
{
	struct list_base_node
	{
		list_base_node()
		{
#if RDE_DEBUG
			reset();
#endif
		}
		void reset()
		{
			next = prev = this;
		}
		bool in_list() const { return this != next; }

		void link_before(list_base_node* nextNode);
		void unlink();

		list_base_node* prev;
		list_base_node*	next;
	};
}

//=============================================================================
template<typename T, class TAllocator = rde::allocator>
class list
{
private:
	struct node : public internal::list_base_node
	{
		node():	list_base_node() {}
		explicit node(const T& v): list_base_node(), value(v) {}

		T		value;
	};
	static RDE_FORCEINLINE node* upcast(internal::list_base_node* n)
	{
		return (node*)n;
	}

	template<typename TNodePtr, typename TPtr, typename TRef>
	class node_iterator
	{
	public:
		typedef bidirectional_iterator_tag	iterator_category;

        explicit node_iterator(): m_node(NULL) {/**/}
		
        explicit node_iterator(TNodePtr node):	m_node(node) {/**/}
        
		template<typename UNodePtr, typename UPtr, typename URef>
		node_iterator(const node_iterator<UNodePtr, UPtr, URef>& rhs)
		:	m_node(rhs.node())
		{
			/**/
		}

		TRef operator*() const
		{
			RDE_ASSERT(m_node != 0);
			return m_node->value;
		}
		TPtr operator->() const
		{
			return &m_node->value;
		}
		TNodePtr node() const
		{
			return m_node;
		}

		node_iterator& operator++()
		{
			m_node = upcast(m_node->next);
			return *this;
		}
		node_iterator& operator--()
		{
			m_node = upcast(m_node->prev);
			return *this;
		}
		node_iterator operator++(int)
		{
			node_iterator copy(*this);
			++(*this);
			return copy;
		}
		node_iterator operator--(int)
		{
			node_iterator copy(*this);
			--(*this);
			return copy;
		}

		bool operator==(const node_iterator& rhs) const
		{
			return rhs.m_node == m_node;
		}
		bool operator!=(const node_iterator& rhs) const
		{
			return !(rhs == *this);
		}

	private:
		TNodePtr	m_node;
	};

public:
	typedef T													value_type;
	typedef TAllocator											allocator_type;
	typedef int 												size_type;
	typedef node_iterator<node*, T*, T&>						iterator;
	typedef node_iterator<const node*, const T*, const T&>	const_iterator;
	static const size_t										kNodeSize = sizeof(node);

	explicit list(const allocator_type& allocator = allocator_type())
	:	m_allocator(allocator)
	{
		m_root.reset();
	}
	template<class InputIterator>
	list(InputIterator first, InputIterator last, 
		const allocator_type& allocator = allocator_type())
	:	m_allocator(allocator)
	{
		m_root.reset();
		assign(first, last);
	}
	list(const list& rhs, const allocator_type& allocator = allocator_type())
	:	m_allocator(allocator)
	{
		m_root.reset();
		assign(rhs.begin(), rhs.end());
	}
	~list()
	{
		clear();
	}

	list& operator=(const list& rhs)
	{
		if (this != &rhs)
		{
			assign(rhs.begin(), rhs.end());
		}
		return *this;
	}

	iterator begin()				{ return iterator(upcast(m_root.next)); }
	const_iterator begin() const	{ return const_iterator(upcast(m_root.next)); }
	iterator end()					{ return iterator(&m_root); }
	const_iterator end() const		{ return const_iterator(&m_root); }

	const T& front() const	{ RDE_ASSERT(!empty()); return upcast(m_root.next)->value; }
	T& front()				{ RDE_ASSERT(!empty()); return upcast(m_root.next)->value; }
	const T& back() const	{ RDE_ASSERT(!empty()); return upcast(m_root.prev)->value; }
	T& back()				{ RDE_ASSERT(!empty()); return upcast(m_root.prev)->value; }

	void push_front(const T& value)
	{
		node* newNode = construct_node(value);
		newNode->link_before(m_root.next);
	}
	// @pre: !empty()
	inline void pop_front()
	{
		RDE_ASSERT(!empty());
		node* frontNode = upcast(m_root.next);
		frontNode->unlink();
		destruct_node(frontNode);
	}

	void push_back(const T& value)
	{
		node* newNode = construct_node(value);
		newNode->link_before(&m_root);
	}
	// @pre: !empty()
	inline void pop_back()
	{
		RDE_ASSERT(!empty());
		node* backNode = upcast(m_root.prev);
		backNode->unlink();
		destruct_node(backNode);
	}

	iterator insert(iterator pos, const T& value)
	{
		node* newNode = construct_node(value);
		newNode->link_before(pos.node());
		return iterator(newNode);
	}
	iterator erase(iterator it)
	{
		RDE_ASSERT(it.node()->in_list());
		iterator itErase(it);
		++it;
		itErase.node()->unlink();
		destruct_node(itErase.node());
		return it;
	}
	iterator erase(iterator first, iterator last)
	{
		while (first != last)
			first = erase(first);
		return first;
	}

	template<class InputIterator>
	void assign(InputIterator first, InputIterator last) 
	{
		clear();
		while (first != last)
		{
			push_back(*first);
			++first;
		}
	}

	bool empty() const	{ return !m_root.in_list(); }

	void clear()
	{
		// quicker then erase(begin(), end())
		node* it = upcast(m_root.next);
		while (it != &m_root)
		{
			node* nextIt = upcast(it->next);
			destruct_node(it);
			it = nextIt;
		}
		m_root.reset();
	}
	// @todo: consider keeping size member, would make this O(1)
	// as a policy? via preprocessor macro? TBD
	size_type size() const
	{
		const node* it = upcast(m_root.next);
		size_type size(0);
		while (it != &m_root)
		{
			++size;
			it = upcast(it->next);
		}
		return size;
	}

	const allocator_type& get_allocator() const	{ return m_allocator; }
	void set_allocator(const allocator_type& allocator)
	{
		m_allocator = allocator;
	}

private:
	node* construct_node(const T& value)
	{
		void* mem = m_allocator.allocate(sizeof(node));
		return new (mem) node(value);
	}
	void destruct_node(node* n)
	{
		n->~node();
		m_allocator.deallocate(n, sizeof(node));
	}

	allocator_type	m_allocator;
	node			m_root;
};

} // namespace rde

//-----------------------------------------------------------------------------
#endif // RDESTL_LIST_H
