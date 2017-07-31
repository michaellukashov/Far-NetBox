#ifndef RDESTL_MAP_H
#define RDESTL_MAP_H

#include "algorithm.h"
#include "iterator.h"
#include "pair.h"
#include "rb_tree.h"

namespace rde
{
template<typename Tk, typename Tv, 
	class TAllocator = rde::allocator>
class RDESTL_LIB map
{
	template<typename TNodePtr, typename TPtr, typename TRef>
	class node_iterator
	{
	public:
		typedef forward_iterator_tag	iterator_category;

		explicit node_iterator(TNodePtr node, const map* map_)
		:	m_node(node),
			m_map(map_)
		{/**/}
		template<typename UNodePtr, typename UPtr, typename URef>
		node_iterator(const node_iterator<UNodePtr, UPtr, URef>& rhs)
		:	m_node(rhs.node()),
			m_map(rhs.get_map())
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
			RDE_ASSERT(m_node != 0);
			TNodePtr next = find_next_node(m_node);
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
			return rhs.m_node == m_node && m_map == rhs.m_map;
		}
		bool operator!=(const node_iterator& rhs) const
		{
			return !(rhs == *this);
		}

		const map* get_map() const { return m_map; }
	private:
		TNodePtr find_next_node(TNodePtr node) const
		{
			return m_map->m_tree.find_next_node(node);
		}

		TNodePtr	m_node;
		const map*	m_map;
	};
	template<typename Tkm, typename Tvm>
	struct RDESTL_LIB map_pair : public rde::pair<Tkm, Tvm>
	{
		map_pair() {}
		map_pair(const Tkm& k, const Tvm& v): pair<Tkm, Tvm>(k, v) {}
		bool operator<(const map_pair& rhs) const
		{
			return this->first < rhs.first;
		}
		RDE_FORCEINLINE const Tkm& get_key() const	{ return this->first; }
	};
	template<typename Tkm, typename Tvm>
	struct map_traits
	{
		typedef	Tkm					key_type;
		typedef map_pair<Tkm, Tvm>	value_type;
	};

public:
	typedef Tk															key_type;
	typedef Tv															data_type;
	typedef map_pair<Tk, Tv>											value_type;
	typedef rb_tree_base<map_traits<Tk, Tv> >							tree_type;
	typedef typename tree_type::size_type								size_type;
	typedef node_iterator<typename tree_type::node*, value_type*, value_type&>	iterator;
	typedef node_iterator<typename tree_type::node*, const value_type*, const value_type&>	const_iterator;
	typedef TAllocator													allocator_type;

	explicit map(const allocator_type& allocator = allocator_type())
	:	m_tree(allocator) {}
	template<typename TInputIterator>
	map(TInputIterator dataBegin, TInputIterator dataEnd, const allocator_type& allocator = allocator_type())
	:	m_tree(allocator)
	{
		TInputIterator it = dataBegin;
		while (it != dataEnd)
		{
			insert(*it);
			++it;
		}
	}

	iterator begin()				{ return iterator(m_tree.get_begin_node(), this); }
	const_iterator begin() const	{ return const_iterator(m_tree.get_begin_node(), this); }
	iterator end()					{ return iterator(0, this); }
	const_iterator end() const		{ return const_iterator(0, this); }

	// @note:	Added for compatibility sake.
	//			Personally, I consider it "risky". Use find/insert for more
	//			explicit operations.
	data_type& operator[](const key_type& key)
	{
		typename tree_type::node* n = m_tree.find_node(key);
		if (n == 0)
			n = m_tree.insert(value_type(key, Tv()));
		return n->value.second;
	}

	// (should result pair<iterator, bool>)
	RDE_FORCEINLINE iterator insert(const value_type& v)
	{
		typename tree_type::node* n = m_tree.insert(v);
		return iterator(n, this);
	}
	RDE_FORCEINLINE iterator find(const key_type& key)
	{
		return iterator(m_tree.find_node(key), this);
	}
	RDE_FORCEINLINE size_type erase(const key_type& key)
	{
		return m_tree.erase(key);
	}
	RDE_FORCEINLINE void clear() { m_tree.clear(); }
	RDE_FORCEINLINE void swap(map& other)
	{
		m_tree.swap(other.m_tree);
	}

	RDE_FORCEINLINE bool empty() const		{ return m_tree.empty(); }
	RDE_FORCEINLINE size_type size() const	{ return m_tree.size(); }

private:
	tree_type	m_tree;
};

}

#endif // RDESTL_MAP_H

