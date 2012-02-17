#pragma once

#include "../spl/memory.h"
#include "../forward.h"

#include <boost/any.hpp>

#include <cstddef>
#include <cstdint>

FORWARD1(boost, template<typename> class shared_future);

namespace caspar { namespace core {

class const_array
{
public:

	// Static Members

	// Constructors

	template<typename T>
	explicit const_array(const std::uint8_t* ptr, std::size_t size, T&& storage)
		: ptr_(ptr)
		, size_(size)
		, storage_(new boost::any(std::forward<T>(storage)))
	{
	}
	
	const_array(const const_array& other);
	const_array(const_array&& other);

	// Methods

	const_array& operator=(const_array other);
	void swap(const_array& other);

	// Properties
		
	const std::uint8_t* begin() const;
	const std::uint8_t* data() const;
	const std::uint8_t* end() const;
	std::size_t size() const;
	bool empty() const;
		
private:
	const std::uint8_t*	ptr_;
	std::size_t			size_;
	spl::shared_ptr<boost::any>	storage_;
};

class mutable_array
{
	mutable_array(const mutable_array&);
	mutable_array& operator=(const mutable_array&);
public:

	// Static Members

	// Constructors
	
	template<typename T>
	explicit mutable_array(std::uint8_t* ptr, std::size_t size, T&& storage)
		: ptr_(ptr)
		, size_(size)
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
	const boost::any& storage() const;
private:
	std::uint8_t*	ptr_;
	std::size_t		size_;
	spl::unique_ptr<boost::any>	storage_;
};

}}