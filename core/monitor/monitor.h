#pragma once

#include <common/reactive.h>
#include <common/spl/memory.h>

#include <functional>
#include <string>
#include <vector>
#include <ostream>
#include <type_traits>

#include <boost/variant.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/chrono.hpp>

#include <tbb/cache_aligned_allocator.h>
#include <tbb/spin_mutex.h>

#ifndef _SCL_SECURE_NO_WARNINGS
#define _SCL_SECURE_NO_WARNINGS
#endif

namespace caspar { namespace monitor {
	
// path

class path sealed
{
public:	
	path();		
	path(const char* path);
	path(std::string path);

	path(const path& other);	
	path(path&& other);
		
	path& operator=(path other);
	path& operator%=(path other);

	template<typename T>
	typename std::enable_if<!std::is_same<typename std::decay<T>::type, path>::value, path&>::type operator%=(T&& value)
	{
		auto str = boost::lexical_cast<std::string>(std::forward<T>(value));

		if(!str.empty())
			str_ += (str.front() != '/' ? "/" : "") + std::move(str);

		return *this;
	}
	
	path& operator%=(const char* value)
	{
		return *this %= std::string(value);
	}

	void swap(path& other);

	const std::string& str() const;	
	bool empty() const;
private:
	std::string str_;
};

template<typename T>
path operator%(path path, T&& value)
{	
	return std::move(path %= std::forward<T>(value));
}

// param

typedef boost::chrono::duration<double, boost::ratio<1, 1>> duration;

typedef boost::variant<bool, int32_t, int64_t, float, double, std::string, std::vector<int8_t>,  duration> param;

std::ostream& operator<<(std::ostream& o, const param& p);

// event

class event sealed
{	
public:	
	typedef std::vector<param, tbb::cache_aligned_allocator<param>> params_t;
	
	event(path path);	
	event(path path, std::shared_ptr<params_t> params);	
				
	event(const event& other);
	event(event&& other);
	event& operator=(event other);

	void swap(event& other);
		
	template<typename T>
	event& operator%(T&& value)
	{
		if(read_only_)
		{
			read_only_ = false;
			params_ = spl::make_shared<params_t>(*params_);
		}
		params_->push_back(std::forward<T>(value));
		return *this;
	}
	
	event			propagate(path path) const;
	const path&		path() const;
	const params_t&	params() const;
private:
	bool						read_only_;		
	monitor::path				path_;
	std::shared_ptr<params_t>	params_;
};

typedef reactive::observable<monitor::event> observable;
	
class subject : public reactive::subject<monitor::event>
{	    
	subject(const subject&);
	subject& operator=(const subject&);
	
	typedef reactive::basic_subject<monitor::event> impl;
public:		
	subject(monitor::path path = monitor::path())
		: path_(std::move(path))
		, impl_(std::make_shared<impl>())

	{
	}
		
	subject(subject&& other)
		: path_(std::move(other.path_))
		, impl_(std::move(other.impl_))
	{
	}

	virtual ~subject()
	{
	}
	
	virtual void subscribe(const observer_ptr& o) override
	{				
		impl_->subscribe(o);
	}

	virtual void unsubscribe(const observer_ptr& o) override
	{
		impl_->unsubscribe(o);
	}
				
	virtual void on_next(const monitor::event& e) override
	{				
		impl_->on_next(path_.empty() ? e : e.propagate(path_));
	}
private:
	monitor::path			path_;
	std::shared_ptr<impl>	impl_;
};

inline subject& operator<<(subject& s, const event& e)
{
	s.on_next(e);
	return s;
}

}}

namespace std {

inline void swap(caspar::monitor::path& lhs, caspar::monitor::path& rhs) 
{
    lhs.swap(rhs);
}
   
inline void swap(caspar::monitor::event& lhs, caspar::monitor::event& rhs) 
{
    lhs.swap(rhs);
}

}
