#pragma once

#include <type_traits>
#include <atomic>

#undef  SHP_DEBUG

#ifdef SHP_DEBUG
#include <cstdio>
#endif

// Simple intrusive shared pointer; the control block 'ShpBase'
// should be derived from by a managed object.

namespace IntrusiveSmart {

class ShpBase {
	template <typename T> friend class Shp;

protected:

	class Key {
		Key() = default;
		Key(const Key &) = delete;
		Key& operator=(const Key &) = delete;
		friend class ShpBase;
	};

private:

	// declare mutable; the reference-count is not conceptually
	// 'part' of the 'real' object. We must manipulate it even
	// in the case of a 'const' object.
	mutable std::atomic<int> refcnt_{0};

	int incRef() const {
		return refcnt_.fetch_add(1);
	}

	int decRef() const {
		int rv = refcnt_.fetch_sub(1);
		if ( 1 == rv ) {
			unmanage(Key());
		}
		return rv;
	}

public:
	// unmanage may be used to
	//  - reset an object for reuse
	//  - hand it over to an object manager, e.g., a free list.
	virtual void unmanage(const Key &) const {
		delete this;
	}

public:
#ifdef SHP_DEBUG
	virtual void pr()
	{
		printf("refcnt %d\n", refcnt_.load());
	}
#endif

	// for testing/debugging
	long use_count() const
	{
		return refcnt_.load();
	}

	// must have virtual destructor!
	virtual ~ShpBase()
	{
	}
};

template <typename T> class Shp {
	static_assert( std::is_base_of<ShpBase, T>::value );
	template <typename U> friend class Shp;
	T *p_;
public:
	typedef T element_type;

	Shp() : p_(nullptr) {}

	// construct a new shared pointer
	Shp(T *p) : p_(p) {
		if ( p_ ) {
			p_->incRef();
		}
#ifdef SHP_DEBUG
		printf("Shp(T*): %ld\n", use_count());
#endif
	}

	long use_count() const
	{
		return p_ ? p_->refcnt_.load() : 0;
	}

	T *
	get() const
	{
		return p_;
	}

private:

	struct dummy_tag {};

	// copy constructor; use a dummy argument so
	// that we may reuse a single template for the
	// templated- and non-templated versions below.
	// If we dont explicitly provide Shp(const Shp &rhs)
	// (i.e., a non-templated variant) then the compiler
	// assumes the default constructor has been deleted
	// and does *not* use the templated version.

	template <typename U>
	Shp(const Shp<U> &rhs, struct dummy_tag)
	: p_(rhs.get())
	{
		if ( p_ ) {
			p_->incRef();
		}
#ifdef SHP_DEBUG
		printf("Shp(const Shp<U> &)\n");
#endif
	}

	// Same for the move constructor.
	template <typename U>
	Shp(Shp<U> &&rhs, struct dummy_tag)
	{
		// just take over from rhs; no need to adjust reference count
		p_ = rhs.get();
		rhs.p_ = nullptr;
#ifdef SHP_DEBUG
		printf("Shp(Shp&&): %ld - %ld\n", use_count(), rhs.use_count());
#endif
	}

public:

	template <typename U>
	Shp(const Shp<U> &rhs)
	: Shp( rhs, dummy_tag() )
	{
	}

	Shp(const Shp &rhs)
	: Shp( rhs, dummy_tag() )
	{
	}

	template <typename U>
	Shp(Shp<U> &&rhs)
	: Shp( rhs, dummy_tag() )
	{
	}

	Shp(Shp &&rhs)
	: Shp( rhs, dummy_tag() )
	{
	}

	template <typename U>
	Shp & operator=(const Shp<U> &rhs)
	{
		// we can decrement 'this' right away - if both
		// 'this' and 'rhs' reference the same object then
		// 'this' cannot drop to zero even if we decrement first.
		if ( p_ ) {
			p_->decRef();
		}
		if ( (p_ = rhs.get()) ) {
			p_->incRef();
		}
#ifdef SHP_DEBUG
		printf("operator=(Shp&): %ld - %ld\n", use_count(), rhs.use_count());
#endif
		return *this;
	}

	// must provide a non-templated version; the compiler
	// won't use a the template if U==T.
	Shp & operator=(const Shp &rhs)
	{
		return operator=<T>(rhs);
	}

	template <typename U>
	Shp & operator=(Shp<U> &&rhs)
	{
		T *p;
		// if 'this' and 'rhs' reference the same object
		// then the count is at least 2 and can not drop
		// to zero if we decrement first.
		if ( p_ ) {
			p_->decRef();
		}
		// just move over; no need to adjust the count
		p_    = rhs.get();
		rhs.p = nullptr;
		
#ifdef SHP_DEBUG
		printf("operator=(Shp&&): %ld - %ld\n", use_count(), rhs.use_count());
#endif
		return *this;
	}

	Shp & operator=(Shp &&rhs)
	{
		return operator=<T>(rhs);
	}

	~Shp()
	{
		if ( p_ ) {
			p_->decRef();
		}
#ifdef SHP_DEBUG
		printf("~Shp(): %ld\n", use_count());
#endif
	}

	void reset()
	{
		if ( p_ ) {
			p_->decRef();
			p_ = nullptr;
		}
	}

	void swap(Shp &rhs)
	{
		// reference counts don't change
		auto tmp = rhs.p_;
		rhs.p_   = p_;
		p_       = tmp;
	}

	operator bool() const
	{
		return !!p_;
	}

	T *operator->() {
		return p_;
	}

	T &operator*() {
		return *p_;
	}

#ifdef SHP_DEBUG
	void pr()
	{
		printf("%p: %ld\n", p_, use_count());
	}
#endif

};

template <typename L, typename R>
static bool operator==(Shp<L> lhs, Shp<R> rhs)
{
	return lhs.get() == rhs.get();
}

template <typename L, typename R>
static bool operator!=(Shp<L> lhs, Shp<R> rhs)
{
	return lhs.get() != rhs.get();
}

}; // namespace IntrusiveSmart
