#pragma once

#include <stdexcept>
#include <mutex>

#include <IntrusiveShp.hpp>

namespace IntrusiveSmart {

class FreeListBase;

class FreeListNode : public ShpBase {
	friend class FreeListBase;
private:
	mutable union {
		FreeListBase *list_;
		FreeListNode *next_;
	} u_;

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
	virtual void unmanage(const Key &) const override;
};

class FreeListBase {
private:

	FreeListNode *anchor_ {nullptr};
	std::mutex mtx_;
	std::atomic<unsigned> avail_{0};

protected:
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
	virtual void put(FreeListNode *p)
	{
		std::unique_lock l( mtx_ );
		p->setNext( anchor_ );
		anchor_ = p;
		avail_.fetch_add(1);
	}

	virtual ~FreeListBase()
	{
		while ( auto p = getRaw() ) {
			delete p;
		}
	}

	template <typename T>
	Shp<T>
	get()
	{
		auto p = getRaw();
		return Shp<T>(static_cast<T*>( p ));
	}
};

}; // namespace IntrusiveSmart
