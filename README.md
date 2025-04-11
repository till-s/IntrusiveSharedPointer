# IntrusiveSharedPointer

C++ template implementing an intrusive shared pointer.

Unfortunately, `std::shared_ptr` is ill-suited to implement
object management via a free-list. Even using custom allocator
and deleters provides no way of managing and unmanaging objects
(without constructing and deleting them).

Consider a use case where complex objects (i.e., their
construction involves multiple heap allocations) should
not be completely destroyed and recreated when the last reference
is relinquished and a new object is needed.

In such a case, we would like the object to be added to a free-list
after it is no longer being used. When a new object is required it
can be obtained from the free list and should be managed with a 
shared pointer.

The intrusive shared pointer introduced here provides a simple
control block holding the reference counter. The control block
is intended to be used as a base of the managed objects.
The control block provides a virtual `unmanage()` method which
is executed when the last shared reference is relinquished.
The default implementation simply `delete`s the object but
a suitable subclass (such a subclass is introduced by
`IntrusiveShpFreeList`) could take over management in any
desired way.

In the case of a free-list manager the `unmanage` method could

 - put the object into a state so that it may be reused
 - enqueue the object on a free list.
