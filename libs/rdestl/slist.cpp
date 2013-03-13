#include "slist.h"

namespace rde
{
	namespace internal
	{
		void slist_base_node::link_after(slist_base_node* prevNode)
		{
			RDE_ASSERT(!in_list());
			next = prevNode->next;
			prevNode->next = this;
		}
		void slist_base_node::unlink(slist_base_node* prevNode)
		{
			RDE_ASSERT(in_list());
			RDE_ASSERT(prevNode->next == this);
			prevNode->next = next;
			next = this;
		}
	}
}
