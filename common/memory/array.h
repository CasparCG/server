#pragma once

#include "../spl/memory.h"
#include "../forward.h"

#include <boost/any.hpp>

#include <cstddef>
#include <cstdint>

FORWARD1(boost, template<typename> class shared_future);

namespace caspar { namespace core {
	
class mutable_array
{
	mutable_array(const mutable_array&);
	mutable_array& operator=(const mutable_array&);

	friend class const_array;
public:

	// Static Members

	// Constructors
	
	template<typename T>
	explicit mutable_array(std::uint8_t* ptr, std::size_t size, bool cacheable, T&& storage)
		: ptr_(ptr)
		, size_(size)
		, cacheable_(cacheable)
		, storage_(new boost::any(std::forward<T>(storage)))
	{
	}

	mutable_array(mutable_array&& other);

	// Methods

	mutable_array& operator=(mutable_array&& other);

	// Properties
	
	std::uint8_t* begin();
	std::uint8_t* data();
	std::uint8_t* end();
	const std::uint8_t* begin() const;
	const std::uint8_t* data() const;
	const std::uint8_t* end() const;
	std::size_t size() const;
	bool empty() const;
	bool cacheable() const;
	
	template<typename T>
	T storage() const
	{
		return boost::any_cast<T>(*storage_);
	}
private:
	std::uint8_t*	ptr_;
	std::size_t		size_;
	bool			cacheable_;
	std::unique_ptr<boost::any>	storage_;
};

class const_array
{
public:

	// Static Members

	// Constructors

	template<typename T>
	explicit const_array(const std::uint8_t* ptr, std::size_t size, bool cacheable, T&& storage)
		: ptr_(ptr)
		, size_(size)
		, cacheable_(cacheable)
		, storage_(new boost::any(std::forward<T>(storage)))
	{
	}
	
	const_array(const const_array& other);	
	const_array(mutable_array&& other);

	// Methods

	const_array& operator=(const const_array& other);
	void swap(const_array& other);

	// Properties
		
	const std::uint8_t* begin() const;
	const std::uint8_t* data() const;
	const std::uint8_t* end() const;
	std::size_t size() const;
	bool empty() const;
	bool cacheable() const;

	template<typename T>
	T storage() const
	{
		return boost::any_cast<T>(*storage_);
	}

private:
	const std::uint8_t*	ptr_;
	std::size_t			size_;
	bool				cacheable_;
	std::shared_ptr<boost::any>	storage_;
};

}}