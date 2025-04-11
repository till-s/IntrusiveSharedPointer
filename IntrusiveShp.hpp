#pragma once

#include <type_traits>
#include <atomic>

#undef  SHP_DEBUG

#ifdef SHP_DEBUG
#include <cstdio>
#endif

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

	long use_count() const
	{
		return refcnt_.load();
	}

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

	template <typename U>
	Shp(Shp<U> &&rhs, struct dummy_tag)
	{
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
		auto old = p_;
		if ( (p_ = rhs.get()) ) {
			p_->incRef();
		}
		if ( old ) {
			old->decRef();
		}
#ifdef SHP_DEBUG
		printf("operator=(Shp&): %ld - %ld\n", use_count(), rhs.use_count());
#endif
		return *this;
	}

	Shp & operator=(const Shp &rhs)
	{
		return operator=<T>(rhs);
	}

	template <typename U>
	Shp & operator=(Shp<U> &&rhs)
	{
		T *p;
		// no-op if both are nullptr or both reference the same object
		if ( (p = get()) != (p_ = rhs.get()) ) {
			if ( p ) {
				p->decRef();
			}
			rhs.p_ = nullptr;
		}
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
