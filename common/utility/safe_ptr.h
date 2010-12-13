#pragma once

#include "../exception/exceptions.h"

#include <memory>
#include <vector>
#include <type_traits>

namespace caspar {
	
template<typename T>
class safe_ptr
{	
	template <typename> friend class safe_ptr;
public:
	typedef T element_type;
	
	safe_ptr() : impl_(std::make_shared<T>()){static_assert(!std::is_abstract<T>::value, "Cannot construct abstract class.");}	
	
	safe_ptr(const safe_ptr<T>& other) : impl_(other.impl_){}
	
	template<typename Y>
	safe_ptr(const safe_ptr<Y>& other, typename std::enable_if<std::is_convertible<Y*, T*>::value, void*>::type = 0) : impl_(other.impl_){}
		
	template<typename Y>	
	safe_ptr(const Y& impl, typename std::enable_if<std::is_convertible<typename std::add_pointer<Y>::type, typename std::add_pointer<T>::type>::value, void>::type* = 0)
		: impl_(std::make_shared<Y>(impl)) {}
	
	template<typename Y>	
	safe_ptr(Y&& impl, typename std::enable_if<std::is_convertible<typename std::add_pointer<Y>::type, typename std::add_pointer<T>::type>::value, void>::type* = 0)
		: impl_(std::make_shared<Y>(std::forward<Y>(impl))) {}

	template<typename Y>
	typename std::enable_if<std::is_convertible<Y*, T*>::value, safe_ptr<T>&>::type
	operator=(const safe_ptr<Y>& other)
	{
		safe_ptr<T> temp(other);
		temp.swap(*this);
		return *this;
	}

	template <typename Y>
	typename std::enable_if<std::is_convertible<typename std::add_pointer<Y>::type, typename std::add_pointer<T>::type>::value, safe_ptr<T>&>::type
	operator=(Y&& impl)
	{
		safe_ptr<T> temp(std::forward<T>(impl));
		temp.swap(*this);
		return *this;
	}

	T& operator*() const { return *impl_.get();}

	T* operator->() const { return impl_.get();}

	T* get() const { return impl_.get();}

	bool unique() const { return impl_.unique();}

	long use_count() const { return impl_.use_count();}
				
	void swap(safe_ptr& other) { impl_.swap(other.impl_); }	
	
	std::shared_ptr<T> get_shared() const	{ return impl_;	}

	static safe_ptr<T> from_shared(const std::shared_ptr<T>& impl) { return safe_ptr<T>(impl); }

private:		
	
	template<typename Y>	
	safe_ptr(const std::shared_ptr<Y>& impl, typename std::enable_if<std::is_convertible<Y*, T*>::value, void*>::type = 0) : impl_(impl)
	{
		if(!impl)
			BOOST_THROW_EXCEPTION(null_argument() << msg_info("impl"));
	}

	std::shared_ptr<T> impl_;
};

template<class T, class U>
bool operator==(safe_ptr<T> const & a, safe_ptr<U> const & b)
{
	return a.get() == b.get();
}

template<class T, class U>
bool operator!=(safe_ptr<T> const & a, safe_ptr<U> const & b)
{
	return a.get() != b.get();
}

template<class T, class U>
bool operator<(safe_ptr<T> const & a, safe_ptr<U> const & b)
{
	return a.get() < b.get();
}

template<class T> void swap(safe_ptr<T> & a, safe_ptr<T> & b)
{
	a.swap(b);
}

template<class T> T* get_pointer(safe_ptr<T> const & p)
{
	return p.get();
}

template<typename T>
safe_ptr<T> make_safe()
{
	static_assert(!std::is_abstract<T>::value, "Cannot construct abstract class.");
	return safe_ptr<T>();
}

template<typename T, typename P0>
safe_ptr<T> make_safe(P0&& p0)
{
	static_assert(!std::is_abstract<T>::value, "Cannot construct abstract class.");
	return safe_ptr<T>(T(std::forward<P0>(p0)));
}

template<typename T, typename P0, typename P1>
safe_ptr<T> make_safe(P0&& p0, P1&& p1)
{
	static_assert(!std::is_abstract<T>::value, "Cannot construct abstract class.");
	return safe_ptr<T>(T(std::forward<P0>(p0), std::forward<P1>(p1)));
}

template<typename T, typename P0, typename P1, typename P2>
safe_ptr<T> make_safe(P0&& p0, P1&& p1, P2&& p2)
{
	static_assert(!std::is_abstract<T>::value, "Cannot construct abstract class.");
	return safe_ptr<T>(T(std::forward<P0>(p0), std::forward<P1>(p1), std::forward<P2>(p2)));
}

}