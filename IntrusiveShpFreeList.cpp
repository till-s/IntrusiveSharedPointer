
#include <IntrusiveShpFreeList.hpp>

void
IntrusiveSmart::FreeListNode::unmanage(const Key &) const
{
	u_.list_->put( const_cast<FreeListNode*>( this ) );
}

