#ifndef RDESTL_FIXED_LIST_H
#define RDESTL_FIXED_LIST_H

#include "alignment.h"
#include "int_to_type.h"
#include "iterator.h"
#include "type_traits.h"

namespace rde
{

template<typename T, size_t TCapacity>
class fixed_list
{
	// Selects index_type that's "big" enough
	// to hold TCapacity of objects.
	template<int i>
	struct type_selector
	{
		typedef unsigned char	index_type;
	};
	template<>
	struct type_selector<1>
	{
		typedef unsigned short	index_type;
	};
	template<>
	struct type_selector<2>
	{
		typedef unsigned long	index_type;
	};

	enum { select = TCapacity+1 > 255 ? (TCapacity+1 > 65535 ? 2 : 1) : 0 };
	typedef type_selector<select>	index_type_selector;

public:
	typedef typename index_type_selector::index_type	index_type;

private:
	struct node
	{
		node() {}
		explicit node(const T& v, int i): value(v), index((index_type)i) {}
		void reset()
		{
			index = index_next = index_prev = 0;
		}
		bool in_list() const { return index != index_next; }

		T			value;
		index_type	index;
		index_type	index_next;
		index_type	index_prev;
	};

	template<typename TNodePtr, typename TPtr, typename TRef>
	class node_iterator
	{
	public:
		typedef bidirectional_iterator_tag	iterator_category;

		explicit node_iterator(TNodePtr node, const fixed_list* list)
		:	m_node(node), m_list(list) {/**/}
		template<typename UNodePtr, typename UPtr, typename URef>
		node_iterator(const node_iterator<UNodePtr, UPtr, URef>& rhs)
		:	m_node(rhs.node()),
			m_list(rhs.list())
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
		const fixed_list* list() const
		{
			return m_list;
		}

		node_iterator& operator++()
		{
			m_node = m_list->get_next(m_node);
			return *this;
		}
		node_iterator& operator--()
		{
			m_node = m_list->get_prev(m_node);
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
		TNodePtr			m_node;
		const fixed_list*	m_list;
	};

public:
	typedef T												value_type;
	typedef int												size_type;
	typedef node_iterator<node*, T*, T&>					iterator;
	typedef node_iterator<const node*, const T*, const T&>	const_iterator;

	fixed_list()
	:	m_num_nodes(0)
	{
		init_free_indices();
		get_root()->reset();
	}
	template<class InputIterator>
	fixed_list(InputIterator first, InputIterator last)
	{
		get_root()->reset();
		assign(first, last);
	}
	fixed_list(const fixed_list& rhs)
	{
		get_root()->reset();
		copy(rhs, 
			int_to_type<has_trivial_copy<T>::value>());
	}
	explicit fixed_list(e_noinitialize)
	{
		/**/
	}
	~fixed_list()
	{
		clear();
	}

	fixed_list& operator=(const fixed_list& rhs)
	{
		if (this != &rhs)
		{
			copy(rhs, 
				int_to_type<has_trivial_copy<T>::value>());
		}
		return *this;
	}

	iterator begin()				{ return iterator(get_next(get_root()), this); }
	const_iterator begin() const	{ return const_iterator(get_next(get_root()), this); }
	iterator end()					{ return iterator(get_root(), this); }
	const_iterator end() const		{ return const_iterator(get_root(), this); }

	const T& front() const	{ RDE_ASSERT(!empty()); return get_next(get_root())->value; }
	T& front()				{ RDE_ASSERT(!empty()); return get_next(get_root())->value; }
	const T& back() const	{ RDE_ASSERT(!empty()); return get_prev(get_root())->value; }
	T& back()				{ RDE_ASSERT(!empty()); return get_prev(get_root())->value; }

	void push_front(const T& value)
	{
		RDE_ASSERT(m_free_index_top > 0);	// 0 is root
		const int index = m_free_indices[m_free_index_top--];
		node* new_node = construct_node(value, index);
		++m_num_nodes;
		RDE_ASSERT(m_num_nodes <= TCapacity);
		link_before(new_node, get_next(get_root()));
	}
	// @pre: !empty()
	inline void pop_front()
	{
		RDE_ASSERT(!empty());
		node* front_node = get_next(get_root());
		m_free_indices[++m_free_index_top] = front_node->index;
		unlink(front_node);
		destruct_node(front_node);
		--m_num_nodes;
	}

	void push_back(const T& value)
	{
		RDE_ASSERT(m_free_index_top > 0);	// 0 is root
		const int index = m_free_indices[m_free_index_top--];
		node* new_node = construct_node(value, index);
		++m_num_nodes;
		RDE_ASSERT(m_num_nodes <= TCapacity + 1);
		link_before(new_node, get_root());
	}
	// @pre: !empty()
	inline void pop_back()
	{
		RDE_ASSERT(!empty());
		node* back_node = get_prev(get_root());
		m_free_indices[++m_free_index_top] = back_node->index;
		unlink(back_node);
		destruct_node(back_node);
		--m_num_nodes;
	}

	iterator insert(iterator pos, const T& value)
	{
		RDE_ASSERT(m_free_index_top > 0);
		const int index = m_free_indices[m_free_index_top--];
		node* new_node = construct_node(value, index);
		link_before(new_node, pos.node());
		++m_num_nodes;
		return iterator(new_node, this);
	}
	// @pre: !empty()
	iterator erase(iterator it)
	{
		RDE_ASSERT(!empty());
		RDE_ASSERT(it.node()->in_list());
		RDE_ASSERT(it.list() == this);
		iterator it_erase(it);
		++it;
		node* erase_node = it_erase.node();
		m_free_indices[++m_free_index_top] = erase_node->index;
		unlink(erase_node);
		destruct_node(erase_node);
		--m_num_nodes;
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

	void clear()
	{
		// Quicker than erase(begin(), end()), doesnt have
		// to unlink all the nodes.
		node* root = get_root();
		node* iter = get_next(root);
		while (iter != root)
		{
			node* next = get_next(iter);
			destruct_node(iter);
			iter = next;
		}
		root->reset();
		m_num_nodes = 0;
		init_free_indices();
	}

	bool empty() const	{ return m_num_nodes == 0; }
	size_type size() const
	{
		return m_num_nodes;
	}

private:
	void link_before(node* n, node* next)
	{
		n->index_prev = next->index_prev;
		get_prev(n)->index_next = n->index;
		next->index_prev = n->index;
		n->index_next = next->index;
	}
	void unlink(node* n)
	{
		get_prev(n)->index_next = n->index_next;
		get_next(n)->index_prev = n->index_prev;
		n->index_next = n->index_prev = n->index;
	}
	RDE_FORCEINLINE node* get_prev(const node* n) const
	{
		return get_nodes() + n->index_prev;
	}
	RDE_FORCEINLINE node* get_next(const node* n) const
	{
		return get_nodes() + n->index_next;
	}

	node* construct_node(const T& value, int index)
	{
		node* mem = get_nodes() + index;
		return new (mem) node(value, index);
	}
	void destruct_node(node* n)
	{
		(void)n;
		n->~node();
	}
	void init_free_indices()
	{
		for (int i = 0; i < TCapacity + 1; ++i)
			m_free_indices[i] = (index_type)i;
		m_free_index_top = TCapacity;
	}
	RDE_FORCEINLINE node* get_nodes() const
	{
		return (node*)(&m_node_mem[0]);
	}
	void copy(const fixed_list& rhs, int_to_type<true>)
	{
		Sys::MemCpy(this, &rhs, sizeof(rhs));
	}
	void copy(const fixed_list& rhs, int_to_type<false>)
	{
		assign(rhs.begin(), rhs.end());
	}
	RDE_FORCEINLINE node* get_root() const
	{
		// I'll be nice and inline it for the compiler (=get_nodes()).
		return (node*)(&m_node_mem[0]);
	}

	typedef typename aligned_as<node>::res	etype_t;

	etype_t			m_node_mem[sizeof(node) * (TCapacity + 1) / sizeof(etype_t)];
	index_type		m_free_indices[TCapacity + 1];
	int				m_free_index_top;
	size_type		m_num_nodes;	// Excluding root!
};

}

#endif
