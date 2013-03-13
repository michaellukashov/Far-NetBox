#include "intrusive_list.h"

namespace rde
{
	intrusive_list_base::intrusive_list_base()
	:	m_root()
	{
		/**/
	}

	intrusive_list_base::size_type intrusive_list_base::size() const
	{
		size_type numNodes(0);
		const intrusive_list_node* iter = &m_root;
		do
		{
			iter = iter->next;
			++numNodes;
		} while (iter != &m_root);
		return numNodes - 1;
	}

	void intrusive_list_base::link(intrusive_list_node* node, intrusive_list_node* nextNode)
	{
		RDE_ASSERT(!node->in_list());
		node->prev = nextNode->prev;
		node->prev->next = node;
		nextNode->prev = node;
		node->next = nextNode;
	}
	void intrusive_list_base::unlink(intrusive_list_node* node)
	{
		RDE_ASSERT(node->in_list());
		node->prev->next = node->next;
		node->next->prev = node->prev;
		node->next = node->prev = node;
	}
}
