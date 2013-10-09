#pragma once

#include <common/memory.h>
#include <common/assert.h>

#include <boost/variant.hpp>
#include <boost/chrono/duration.hpp>

#include <cstdint>
#include <string>
#include <vector>

#include <agents.h>

namespace caspar { namespace monitor {

typedef boost::variant<bool, 
					   std::int32_t, 
					   std::int64_t, 
					   float, 
					   double, 
					   std::string,
					   std::wstring,
					   std::vector<std::int8_t>> data_t;

class message
{
public:

	message(std::string path, std::vector<data_t> data = std::vector<data_t>())
		: path_(std::move(path))
		, data_ptr_(std::make_shared<std::vector<data_t>>(std::move(data)))
	{
		CASPAR_ASSERT(path.empty() || path[0] == '/');
	}

	message(std::string path, spl::shared_ptr<std::vector<data_t>> data_ptr)
		: path_(std::move(path))
		, data_ptr_(std::move(data_ptr))
	{
		CASPAR_ASSERT(path.empty() || path[0] == '/');
	}

	const std::string& path() const
	{
		return path_;
	}

	const std::vector<data_t>& data() const
	{
		return *data_ptr_;
	}

	message propagate(const std::string& path) const
	{
		return message(path + path_, data_ptr_);
	}

	template<typename T>
	message& operator%(T&& data)
	{
		data_ptr_->push_back(std::forward<T>(data));
		return *this;
	}

private:
	std::string								path_;
	spl::shared_ptr<std::vector<data_t>>	data_ptr_;
};

class subject : public Concurrency::transformer<monitor::message, monitor::message>
{
public:
	subject(std::string path = "")
		: Concurrency::transformer<monitor::message, monitor::message>([=](const message& msg)
		{
			return msg.propagate(path);
		})
	{
		CASPAR_ASSERT(path.empty() || path[0] == '/');
	}

	template<typename T>
	subject& operator<<(T&& msg)
	{
		Concurrency::send(*this, std::forward<T>(msg));
		return *this;
	}
};

typedef Concurrency::ISource<monitor::message> source;


}}