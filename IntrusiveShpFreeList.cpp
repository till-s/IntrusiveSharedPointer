
#include <IntrusiveShpFreeList.hpp>

void
IntrusiveSmart::FreeListNode::unmanage(const Key &)
{
	u_.list_->put( this );
}

