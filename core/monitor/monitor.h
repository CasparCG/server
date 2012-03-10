#pragma once

#include <common/reactive.h>

#include <functional>
#include <memory>
#include <ostream>
#include <string>
#include <type_traits>
#include <vector>

#include <boost/variant.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/chrono.hpp>

#include <tbb/cache_aligned_allocator.h>

namespace boost {
namespace detail { namespace variant {

template <>
struct has_nothrow_move<std::string>
: mpl::true_
{
};
  
template <>
struct has_nothrow_move<std::vector<int8_t>>
: mpl::true_
{
};

template <>
struct has_nothrow_move<boost::chrono::duration<double, boost::ratio<1, 1>>>
: mpl::true_
{
};

}}}

namespace caspar { namespace monitor {
	
// path

class path sealed
{
public:	

	// Static Members

	// Constructors

	path();		
	path(const char* path);
	path(std::string path);

	path(const path& other);	
	path(path&& other);
		
	// Methods

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

	// Properties

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

std::ostream& operator<<(std::ostream& o, const path& p);

// param

typedef boost::chrono::duration<double, boost::ratio<1, 1>> duration;

typedef boost::variant<bool, int32_t, int64_t, float, double, std::string, std::wstring, std::vector<int8_t>, duration> param;

std::ostream& operator<<(std::ostream& o, const param& p);

// event

class event sealed
{	
public:	
	
	// Static Members

	typedef std::vector<param, tbb::cache_aligned_allocator<param>> params_t;

	// Constructors

	event(path path);	
	event(path path, params_t params);					
	event(const event& other);
	event(event&& other);

	// Methods

	event& operator=(event other);

	void swap(event& other);
		
	template<typename T>
	event& operator%(T&& value)
	{
		params_.push_back(std::forward<T>(value));
		return *this;
	}
	
	event propagate(path path) const;

	// Properties

	const path&		path() const;
	const params_t&	params() const;
private:
	monitor::path	path_;
	params_t		params_;
};

std::ostream& operator<<(std::ostream& o, const event& e);

// reactive

typedef reactive::observable<monitor::event>	observable;
typedef reactive::observer<monitor::event>		observer;
typedef reactive::subject<monitor::event>		subject;
	
class basic_subject sealed : public reactive::subject<monitor::event>
{	    
	basic_subject(const basic_subject&);
	basic_subject& operator=(const basic_subject&);
	
	class impl : public observer
	{
	public:
		impl(monitor::path path = monitor::path())
			: impl_()
			, path_(std::move(path))
		{
		}

		impl(impl&& other)
			: impl_(std::move(other.impl_))
			, path_(std::move(other.path_))
		{		
		}

		impl& operator=(impl&& other)
		{
			impl_ = std::move(other.impl_);		
			path_ = std::move(other.path_);
		}
							
		void on_next(const monitor::event& e) override
		{				
			impl_.on_next(path_.empty() ? e : e.propagate(path_));
		}

		void subscribe(const observer_ptr& o)
		{				
			impl_.subscribe(o);
		}

		void unsubscribe(const observer_ptr& o)
		{
			impl_.unsubscribe(o);
		}
				
	private:
		reactive::basic_subject_impl<monitor::event>	impl_;		
		monitor::path									path_;
	};

public:		

	// Static Members

	// Constructors

	basic_subject(monitor::path path = monitor::path())
		: impl_(std::make_shared<impl>(std::move(path)))

	{
	}
		
	basic_subject(basic_subject&& other)
		: impl_(std::move(other.impl_))
	{
	}
	
	// Methods

	basic_subject& operator=(basic_subject&& other)
	{
		impl_ = std::move(other.impl_);
	}

	operator std::weak_ptr<observer>()
	{
		return impl_;
	}

	// observable
	
	void subscribe(const observer_ptr& o) override
	{				
		impl_->subscribe(o);
	}

	void unsubscribe(const observer_ptr& o) override
	{
		impl_->unsubscribe(o);
	}

	// observer
				
	void on_next(const monitor::event& e) override
	{				
		impl_->on_next(e);
	}

	// Properties

private:
	std::shared_ptr<impl> impl_;
};

inline subject& operator<<(subject& s, const event& e)
{
	s.on_next(e);
	return s;
}

}}