#include "../StdAfx.h"

#include "monitor.h"

#include <utility>

namespace caspar { namespace monitor {
	
// param
	
namespace detail {

struct param_visitor : public boost::static_visitor<void>
{
	std::ostream& o;

	param_visitor(std::ostream& o)
		: o(o)
	{
	}

	void operator()(const std::vector<int8_t>& bytes)
	{		
		o << std::string(std::begin(bytes), std::end(bytes));
	}

	void operator()(const duration& duration)
	{		
		o << duration.count();
	}

	template<typename T>
	void operator()(const T& value)
	{
		o << value;
	}
};

}

std::ostream& operator<<(std::ostream& o, const param& p)
{
	detail::param_visitor v(o);
	boost::apply_visitor(v, p);
	return o;
}

// path

path::path()
{
}

path::path(const char* path)
	: str_(path)
{
}

path::path(std::string path)
	: str_(path)
{
}

path::path(const path& other)
	: str_(other.str_)
{		
}
	
path::path(path&& other)
	: str_(std::move(other.str_))
{		
}

path& path::operator=(path other)
{
	std::swap(*this, other);
	return *this;
}

path& path::operator%=(path other)
{
	return *this %= other.str_;
}

void path::swap(path& other)
{
	std::swap(str_, other.str_);
}
	
const std::string& path::str() const
{
	return str_;
}

bool path::empty() const
{
	return str_.empty();
}

std::ostream& operator<<(std::ostream& o, const path& p)
{
	o << p.str();
	return o;
}

// event

event::event(monitor::path path)
	: path_(std::move(path))
{
}
	
event::event(monitor::path path, params_t params)
	: path_(std::move(path))
	, params_(std::move(params))
{
}

event::event(const event& other)
	: path_(other.path_)
	, params_(other.params_)
{
}

event::event(event&& other)
	: path_(std::move(other.path_))
	, params_(std::move(other.params_))
{
}

event& event::operator=(event other)
{
	other.swap(*this);
	return *this;
}

void event::swap(event& other)
{
	std::swap(path_, other.path_);
	std::swap(params_, other.params_);
}
		
const path&	event::path() const		
{
	return path_;
}

const event::params_t& event::params() const	
{
	return params_;
}

event event::propagate(monitor::path path) const
{
	return event(std::move(path) % path_, params_);
}

std::ostream& operator<<(std::ostream& o, const event& e)
{
	o << e.path();
	for(auto it = e.params().begin(); it != e.params().end(); ++it)
		o << " " << *it;
	return o;
}

}}