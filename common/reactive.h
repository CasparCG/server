#pragma once

#include "spl/memory.h"
#include "concurrency/lock.h"

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

template<typename T, typename C>
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

	observer_function(const observer_function& other)
		: func_(other.func_)
	{
	}

	observer_function(observer_function&& other)
		: func_(std::move(other.func_))
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
		func_(e);
	}
private:
	C func_;
};

template<typename I, typename O = I>
class basic_subject_impl sealed : public subject<I, O>
{	
    template <typename, typename> friend class basic_subject_impl;

	basic_subject_impl(const basic_subject_impl&);
	basic_subject_impl& operator=(const basic_subject_impl&);
public:	
	typedef typename subject<I, O>::observer		observer;
	typedef typename subject<I, O>::observer_ptr	observer_ptr;

	basic_subject_impl()
	{
	}
					
	basic_subject_impl(basic_subject_impl<I, O>&& other)
		: observers_(std::move(other.observers_))
	{
	}
	
	basic_subject_impl& operator=(basic_subject_impl<I, O>&& other)
	{
		tbb::spin_rw_mutex::scoped_lock lock(mutex_, true);
		observers_ = std::move(observers_);
		return *this;
	}

	void clear()
	{
		tbb::spin_rw_mutex::scoped_lock lock(mutex_, true);

		observers_.clear();
	}
	
	void subscribe(const observer_ptr& o) override
	{				
		tbb::spin_rw_mutex::scoped_lock lock(mutex_, false);

		auto it = std::lower_bound(std::begin(observers_), std::end(observers_), o, comp_);
		if (it == std::end(observers_) || comp_(o, *it))
		{
			lock.upgrade_to_writer();
			observers_.insert(it, o);
		}		
	}

	void unsubscribe(const observer_ptr& o) override
	{
		tbb::spin_rw_mutex::scoped_lock lock(mutex_, false);
		
		auto it = std::lower_bound(std::begin(observers_), std::end(observers_), o, comp_);
		if(it != std::end(observers_) && !comp_(o, *it))
		{
			lock.upgrade_to_writer();
			observers_.erase(it);
		}		
	}
	
	void on_next(const I& e) override
	{				
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
		
		for(auto it = std::begin(observers); it != std::end(observers); ++it)
			(*it)->on_next(e);
	}
private:
	typedef tbb::cache_aligned_allocator<std::weak_ptr<observer>>	allocator;

	std::owner_less<std::weak_ptr<observer>>		comp_;
	std::vector<std::weak_ptr<observer>, allocator>	observers_;
	mutable tbb::spin_rw_mutex						mutex_;
};

template<typename I, typename O = I>
class basic_subject : public subject<I, O>
{	
    template <typename, typename> friend class basic_subject;

	basic_subject(const basic_subject&);
	basic_subject& operator=(const basic_subject&);

	typedef basic_subject_impl<I, O> impl;
public:	
	typedef typename subject<I, O>::observer		observer;
	typedef typename subject<I, O>::observer_ptr	observer_ptr;

	basic_subject()
		: impl_(std::make_shared<impl>())

	{
	}
		
	basic_subject(subject&& other)
		: impl_(std::move(other.impl_))
	{
	}

	virtual ~basic_subject()
	{
	}

	basic_subject& operator=(basic_subject&& other)
	{
		other.swap(*this);
	}

	void swap(basic_subject& other)
	{
		impl_.swap(other.impl_);
	}
	
	virtual void subscribe(const observer_ptr& o) override
	{				
		impl_->subscribe(o);
	}

	virtual void unsubscribe(const observer_ptr& o) override
	{
		impl_->unsubscribe(o);
	}
				
	virtual void on_next(const I& e) override
	{				
		impl_->on_next(e);
	}

	operator std::weak_ptr<observer>()
	{
		return impl_;
	}
private:
	std::shared_ptr<impl> impl_;
};

template<typename F>
spl::shared_ptr<observer_function<typename std::decay<typename detail::function_traits<F>::arg1_type>::type, F>> 
make_observer(F func)
{
	return spl::make_shared<observer_function<std::decay<typename detail::function_traits<F>::arg1_type>::type, F>>(std::move(func));
}

template<typename T>
basic_subject<T>& operator<<(basic_subject<T>& s, const T& val)
{
	s.on_next(val);
	return s;
}

}}