#pragma once

#include <stdexcept>
#include <mutex>

#include <IntrusiveShp.hpp>

// Intrusive shared pointer for objects managed by
// a free list.

namespace IntrusiveSmart {

class FreeListBase;

// Intrusive base for objects to be managed.
// This class adds a 'next' pointer member so the
// object can be enqueued on a list.
class FreeListNode : public ShpBase {
	friend class FreeListBase;
private:
	// pointer member union: 
	//   - while the object is managed by a Shp the
	//     u_.list_ member is used to hold a pointer
	//     to the FreeList to which this object is to be
	//     returned once unmanaged.
	//   - while the object is not 'owned' by any Shp
	//     the 'u_.next_' member is used for implementing
	//     a linked list.
	mutable union {
		FreeListBase *list_;
		FreeListNode *next_;
	} u_;

	// all the setters and getters for the pointer members
	// must only be used while the object is not referenced
	// by a Shp.

	FreeListNode *next() const
	{
		if ( use_count() ) {
			throw std::bad_alloc();
		}
		return u_.next_;
	}

	void setNext(FreeListNode *p) const
	{
		if ( use_count() ) {
			throw std::bad_alloc();
		}
		u_.next_ = p;
	}

	FreeListBase *list() const
	{
		if ( use_count() ) {
			throw std::bad_alloc();
		}
		return u_.list_;
	}

	void setList(FreeListBase *p) const
	{
		if ( use_count() ) {
			throw std::bad_alloc();
		}
		u_.list_ = p;
	}

protected:
	// unmanage() returns the object to the associated free list
	virtual void unmanage(const Key &) override;
};

// Base class for a free list holding FreeListNodes.
class FreeListBase {
private:

	FreeListNode *anchor_ {nullptr};
	std::mutex mtx_;
	std::atomic<unsigned> avail_{0};

protected:

	// Get head from the free-list as a plain/raw (non-shared)
	// pointer.
	virtual FreeListNode *getRaw()
	{
		std::unique_lock l( mtx_ );
		auto rv = anchor_;
		if ( rv ) {
			anchor_ = rv->next();
			rv->setList( this );
			avail_.fetch_sub(1);
		}
		return rv;
	}

public:
	// Enqueue object on the free list.
	// Note that new objects (as created with 'new'
	// have a reference count of zero and may simply
	// be added to the free list).
	virtual void put(FreeListNode *p)
	{
		std::unique_lock l( mtx_ );
		p->setNext( anchor_ );
		anchor_ = p;
		avail_.fetch_add(1);
	}

	virtual ~FreeListBase()
	{
		// by default we delete all the objects on
		// the list. A subclass can define their
		// own destructor; since that one is executed
		// first this classes' destructor may find
		// the list already emptied.
		while ( auto p = getRaw() ) {
			delete p;
		}
	}

	// obtain a new shared pointer-managed object
	// from the list.
	template <typename T>
	Shp<T>
	get()
	{
		auto p = getRaw();
		return Shp<T>(static_cast<T*>( p ));
	}
};

}; // namespace IntrusiveSmart
