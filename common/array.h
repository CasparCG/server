#pragma once

#include "memory.h"
#include "forward.h"

#include <boost/any.hpp>

#include <cstddef>
#include <cstdint>

#include "assert.h"

namespace caspar {
	
template<typename T>
class array final
{
	array(const array<T>&);
	array& operator=(const array<T>&);

	template<typename> friend class array;
public:

	// Boost Range support

	typedef T*			iterator;
	typedef const T*	const_iterator;

	// Static Members

	// Constructors
	
	template<typename T2>
	explicit array(T* ptr, std::size_t size, bool cacheable, T2&& storage)
		: ptr_(ptr)
		, size_(size)
		, cacheable_(cacheable)
		, storage_(new boost::any(std::forward<T2>(storage)))
	{
	}

	array(array&& other)
		: ptr_(other.ptr_)
		, size_(other.size_)
		, cacheable_(other.cacheable_)
		, storage_(std::move(other.storage_))
	{
		CASPAR_ASSERT(storage_);
	}

	// Methods
	
	array& operator=(array&& other)
	{
		std::swap(ptr_,	other.ptr_);
		std::swap(size_, other.size_);
		std::swap(storage_,	other.storage_);
		std::swap(cacheable_, other.cacheable_);

		CASPAR_ASSERT(storage_);

		return *this;
	}

	// Properties	
			
	T* begin() const			{return ptr_;}		
	T* data() const				{return ptr_;}
	T* end() const				{return ptr_ + size_;}
	std::size_t size() const	{return size_;}
	bool empty() const			{return size() == 0;}
	bool cacheable() const		{return cacheable_;}
	
	template<typename T2>
	T2* storage() const
	{
		return boost::any_cast<T2>(storage_.get());
	}
private:
	T*							ptr_;
	std::size_t					size_;
	bool						cacheable_;
	std::unique_ptr<boost::any>	storage_;
};

template<typename T>
class array<const T> final
{
public:

	// Boost Range support

	typedef const T*	iterator;
	typedef const T*	const_iterator;

	// Static Members

	// Constructors
	array() = default; // Needed by std::future

	template<typename T2>
	explicit array(const T* ptr, std::size_t size, bool cacheable, T2&& storage)
		: ptr_(ptr)
		, size_(size)
		, cacheable_(cacheable)
		, storage_(new boost::any(std::forward<T2>(storage)))
	{
	}

	explicit array(const T* ptr, std::size_t size, bool cacheable)
		: ptr_(ptr)
		, size_(size)
		, cacheable_(cacheable)
		, storage_(new boost::any)
	{
	}

	array(const array& other)
		: ptr_(other.ptr_)
		, size_(other.size_)
		, cacheable_(other.cacheable_)
		, storage_(other.storage_)
	{
		CASPAR_ASSERT(storage_);
	}

	array(array<T>&& other)
		: ptr_(other.ptr_)
		, size_(other.size_)
		, cacheable_(other.cacheable_)
		, storage_(std::move(other.storage_))
	{
		CASPAR_ASSERT(storage_);
	}

	// Methods

	array& operator=(array other)
	{
		other.swap(*this);
		return *this;
	}

	void swap(array& other)
	{
		std::swap(ptr_,	other.ptr_);
		std::swap(size_, other.size_);
		std::swap(storage_,	other.storage_);
		std::swap(cacheable_, other.cacheable_);
	}

	// Properties
			
	const T* begin() const		{return ptr_;}		
	const T* data() const		{return ptr_;}
	const T* end() const		{return ptr_ + size_;}
	std::size_t size() const	{return size_;}
	bool empty() const			{return size() == 0;}
	bool cacheable() const		{return cacheable_;}
	
	template<typename T2>
	T2* storage() const
	{
		return boost::any_cast<T2>(storage_.get());
	}

private:
	const T*					ptr_		= nullptr;
	std::size_t					size_		= 0;
	bool						cacheable_	= false;
	std::shared_ptr<boost::any>	storage_;
};

}

namespace std {
	
template<typename T>
void swap(caspar::array<const T>& lhs, caspar::array<const T>& rhs)
{
	lhs.swap(rhs);
}

}

