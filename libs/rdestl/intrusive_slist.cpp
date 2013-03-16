#include "intrusive_slist.h"

namespace rde
{
	intrusive_slist_base::intrusive_slist_base()
	{
		/**/
	}

	intrusive_slist_base::size_type intrusive_slist_base::size() const
	{
		size_type numNodes(0);
		const intrusive_slist_node* iter = &m_root;
		do
		{
			iter = iter->next;
			++numNodes;
		} while (iter != &m_root);
		return numNodes - 1;
	}

	void intrusive_slist_base::link_after(intrusive_slist_node* node, intrusive_slist_node* prevNode)
	{
		RDE_ASSERT(!node->in_list());
		node->next = prevNode->next;
		prevNode->next = node;
	}
	void intrusive_slist_base::unlink_after(intrusive_slist_node* node)
	{
		RDE_ASSERT(node->in_list());
		intrusive_slist_node* thisNode = node->next;
		node->next = thisNode->next;
		thisNode->next = thisNode;
	}
}
