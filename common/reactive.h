#pragma once

#include "spl/memory.h"

#include <tbb/spin_rw_mutex.h>
#include <tbb/cache_aligned_allocator.h>

#include <algorithm>
#include <functional>
#include <memory>
#include <vector>

namespace caspar { namespace reactive {
	
namespace detail {

// function_traits which works with MSVC2010 lambdas.
	
template<typename FPtr>
struct function_traits_impl;

template<typename R, typename A1>
struct function_traits_impl<R (*)(A1)>
{
	typedef A1 arg1_type;
};

template<typename R, typename C, typename A1>
struct function_traits_impl<R (C::*)(A1)>
{
	typedef A1 arg1_type;
};

template<typename R, typename C, typename A1>
struct function_traits_impl<R (C::*)(A1) const>
{
	typedef A1 arg1_type;
};

template<typename T>
typename function_traits_impl<T>::arg1_type arg1_type_helper(T);

template<typename F>
struct function_traits
{
	typedef decltype(detail::arg1_type_helper(&F::operator())) arg1_type;
};

}

template<typename T>
class observer
{	
	observer(const observer&);
	observer& operator=(const observer&);
public:
	typedef T value_type;
	
	observer()
	{
	}

	virtual ~observer()
	{
	}

	virtual void on_next(const T&) = 0;
};

template<typename T>
class observable
{
	observable(const observable&);
	observable& operator=(const observable&);
public:
	typedef T						value_type;
	typedef observer<T>				observer;
	typedef std::weak_ptr<observer>	observer_ptr;

	observable()
	{
	}

	virtual ~observable()
	{
	}

	virtual void subscribe(const observer_ptr&) = 0;
	virtual void unsubscribe(const observer_ptr&) = 0;
};

template<typename I, typename O = I>
struct subject : public observer<I>, public observable<O>
{
	typedef typename observable<O>::observer		observer;
	typedef typename observable<O>::observer_ptr	observer_ptr;

	virtual ~subject()
	{
	}
};

namespace detail {

template<typename T>
struct true_func
{
	bool operator()(T)
	{
		return true;
	}
};

template<typename T>
struct void_func
{
	void operator()(T)
	{
	}
};

template<typename I, typename O>
struct forward_func
{
	forward_func(std::function<O(const I&)> func)
		: func_(std::move(func))
	{
	}

	O operator()(const I& value)
	{
		return func_(value);
	}

	std::function<O(const I&)> func_;
};

template<typename I>
struct forward_func<I, I>
{
	const I& operator()(const I& value)
	{
		return value;
	}
};

}

template<typename T, typename C, typename F = detail::true_func<T>>
class observer_function : public observer<T>
{
public:
	observer_function()
	{
	}

	observer_function(C func)
		: func_(std::move(func))
	{
	}

	observer_function(C func, F filter)
		: func_(std::move(func))
		, filter_(std::move(filter))
	{
	}

	observer_function(const observer_function& other)
		: func_(other.func_)
		, filter_(other.filter_)
	{
	}

	observer_function(observer_function&& other)
		: func_(std::move(other.func_))
		, filter_(std::move(other.filter_))
	{
	}

	observer_function& operator=(observer_function other)
	{
		other.swap(*this);
	}

	void swap(observer_function& other)
	{
		std::swap(func_, other.func_);
		std::swap(filter_, other.filter_);
	}
		
	virtual void on_next(const T& e) override
	{
		if(!filter_(e))
			return;

		func_(e);
	}
private:
	C func_;
	F filter_;
};

template<typename T, typename F>
class observer_function<T, detail::void_func<T>, F> : public observer<T>
{
public:		
	virtual void on_next(const T& e) override
	{
	}
};

template<typename I, typename O = I, typename F = detail::true_func<const I&>, typename T = detail::forward_func<I, O>>
class basic_subject : public subject<I, O>
{	
    template <typename, typename, typename, typename> friend class basic_subject;

	basic_subject(const basic_subject&);
	basic_subject& operator=(const basic_subject&);
public:	
	typedef typename subject<I, O>::observer		observer;
	typedef typename subject<I, O>::observer_ptr	observer_ptr;
	typedef F										filter;
	typedef T										transform;

	basic_subject()
	{
	}
	
	basic_subject(F filter)
		: filter_(std::move(filter))
	{
	}
	
	basic_subject(T transform)
		: transform_(std::move(transform))
	{
	}
	
	basic_subject(std::function<O(const I&)> transform)
		: transform_(std::move(transform))
	{
	}

	basic_subject(F filter, T transform)
		: filter_(std::move(filter))
		, transform_(std::move(transform))
	{
	}
	
	virtual ~basic_subject()
	{
	}
		
	template<typename F1, typename T1>
	basic_subject(basic_subject<typename observer::value_type, typename observable::value_type, F1, T1>&& other)
		: observers_(std::move(other.observers_))
		, filter_(std::move(other.filter_))
		, transform_(std::move(other.transform_))
	{
	}
	
	template<typename F1, typename T1>
	basic_subject& operator=(basic_subject<typename observer::value_type, typename observable::value_type, F1, T1>&& other)
	{
		other.swap(*this);
		return *this;
	}
	
	template<typename F1, typename T1>
	void swap(basic_subject<typename observer::value_type, typename observable::value_type, F1, T1>& other)
	{		
		tbb::spin_rw_mutex::scoped_lock lock(mutex_, true);
		tbb::spin_rw_mutex::scoped_lock other_lock(other.mutex_, true);

		std::swap(observers_, other.observers_);
		std::swap(filter_, other.filter_);		
	}

	virtual void clear()
	{
		tbb::spin_rw_mutex::scoped_lock lock(mutex_, true);

		observers_.clear();
	}
	
	virtual void subscribe(const observer_ptr& o) override
	{				
		tbb::spin_rw_mutex::scoped_lock lock(mutex_, false);

		auto it = std::lower_bound(std::begin(observers_), std::end(observers_), o, comp_);
		if (it == std::end(observers_) || comp_(o, *it))
		{
			lock.upgrade_to_writer();
			observers_.insert(it, o);
		}		
	}

	virtual void unsubscribe(const observer_ptr& o) override
	{
		tbb::spin_rw_mutex::scoped_lock lock(mutex_, false);
		
		auto it = std::lower_bound(std::begin(observers_), std::end(observers_), o, comp_);
		if(it != std::end(observers_) && !comp_(o, *it))
		{
			lock.upgrade_to_writer();
			observers_.erase(it);
		}		
	}
	
	virtual void on_next(const I& e) override
	{				
		if(!filter_(e))
			return;

		std::vector<spl::shared_ptr<observer>> observers;

		{
			tbb::spin_rw_mutex::scoped_lock lock(mutex_, false);
		
			auto expired = std::end(observers_);

			for(auto it = std::begin(observers_); it != std::end(observers_); ++it)
			{
				auto o = it->lock();
				if(o)
					observers.push_back(spl::make_shared_ptr(std::move(o)));
				else
					expired = it;
			}

			if(expired != std::end(observers_))
			{		
				lock.upgrade_to_writer();
				observers_.erase(expired);
			}	
		}
		
		const auto& e2 = transform_(e);
		for(auto it = std::begin(observers); it != std::end(observers); ++it)
			(*it)->on_next(e2);
	}
private:
	typedef tbb::cache_aligned_allocator<std::weak_ptr<observer>>	allocator;

	std::owner_less<std::weak_ptr<observer>>		comp_;
	std::vector<std::weak_ptr<observer>, allocator>	observers_;
	mutable tbb::spin_rw_mutex						mutex_;
	F												filter_;
	T												transform_;
};

template<typename F>
spl::shared_ptr<basic_subject<typename std::decay<typename detail::function_traits<F>::arg1_type>::type, typename std::decay<typename detail::function_traits<F>::arg1_type>::type, F>> 
make_filter(F filter)
{
	return std::make_shared<basic_subject<std::decay<typename detail::function_traits<F>::arg1_type>::type, typename std::decay<typename detail::function_traits<F>::arg1_type>::type, F>>(std::move(filter));
}

template<typename F>
spl::shared_ptr<observer_function<typename std::decay<typename detail::function_traits<F>::arg1_type>::type, F>> 
make_observer(F func)
{
	return std::make_shared<observer_function<std::decay<typename detail::function_traits<F>::arg1_type>::type, F>>(std::move(func));
}

template<typename F1, typename F2>
spl::shared_ptr<observer_function<typename std::decay<typename detail::function_traits<F1>::arg1_type>::type, F1, F2>> 
make_observer(F1 func, F2 filter)
{
	return std::make_shared<observer_function<std::decay<typename detail::function_traits<F1>::arg1_type>::type, F1, F2>>(std::move(func), std::move(filter));
}

}}

namespace std {
	
template <typename T, typename F1, typename F2>
void swap(caspar::reactive::observer_function<T, F1>& lhs, caspar::reactive::observer_function<T, F2>& rhs) 
{
    lhs.swap(rhs);
}

template <typename I, typename O, typename F1, typename F2, typename T1, typename T2>
void swap(caspar::reactive::basic_subject<I, O, F1, T1>& lhs, caspar::reactive::basic_subject<I, O, F2, T2>& rhs) 
{
    lhs.swap(rhs);
}

} // std
