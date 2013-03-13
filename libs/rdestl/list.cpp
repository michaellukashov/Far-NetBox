#include "list.h"

namespace rde
{
	namespace internal
	{
		void list_base_node::link_before(list_base_node* nextNode)
		{
			RDE_ASSERT(!in_list());
			prev = nextNode->prev;
			prev->next = this;
			nextNode->prev = this;
			next = nextNode;
		}
		void list_base_node::unlink()
		{
			RDE_ASSERT(in_list());
			prev->next = next;
			next->prev = prev;
			next = prev = this;
		}
	} // internal
}
