#pragma once

#include "../video_format.h"

#include <common/spl/memory.h>
#include <common/forward.h>

#include <boost/range.hpp>
#include <boost/any.hpp>

#include <tbb/cache_aligned_allocator.h>

#include <cstddef>
#include <stdint.h>

FORWARD1(boost, template<typename> class shared_future);

namespace caspar { namespace core {

class const_array
{
public:

	// Static Members

	// Constructors

	template<typename T>
	explicit const_array(const uint8_t* ptr, std::size_t size, T&& storage)
		: ptr_(ptr)
		, size_(size)
		, storage_(new boost::any(std::forward<T>(storage)))
	{
	}
	
	const_array(const const_array& other)
		: ptr_(other.ptr_)
		, size_(other.size_)
		, storage_(other.storage_)
	{
	}

	const_array(const_array&& other)
		: ptr_(other.ptr_)
		, size_(other.size_)
		, storage_(std::move(other.storage_))
	{
	}

	// Methods

	const_array& operator=(const_array other)
	{
		other.swap(*this);
		return *this;
	}

	void swap(const_array& other)
	{
		std::swap(ptr_, other.ptr_);
		std::swap(size_, other.size_);
		std::swap(storage_, other.storage_);
	}

	// Properties
		
	const uint8_t* begin() const	{return ptr_;}		
	const uint8_t* data() const		{return ptr_;}
	const uint8_t* end() const		{return ptr_ + size_;}
	std::size_t size() const		{return size_;}
	bool empty() const				{return size() == 0;}
		
private:
	const uint8_t*	ptr_;
	std::size_t		size_;
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
	explicit mutable_array(uint8_t* ptr, std::size_t size, T&& storage)
		: ptr_(ptr)
		, size_(size)
		, storage_(new boost::any(std::forward<T>(storage)))
	{
	}

	mutable_array(mutable_array&& other)
		: ptr_(other.ptr_)
		, size_(other.size_)
		, storage_(std::move(other.storage_))
	{
	}

	// Methods

	mutable_array& operator=(mutable_array&& other)
	{
		ptr_		= other.ptr_;
		size_		= other.size_;
		storage_	= std::move(other.storage_);
		return *this;
	}

	// Properties
	
	uint8_t* begin()					{return ptr_;}		
	uint8_t* data()						{return ptr_;}
	uint8_t* end()						{return ptr_ + size_;}	
	const uint8_t* begin() const		{return ptr_;}		
	const uint8_t* data() const			{return ptr_;}
	const uint8_t* end() const			{return ptr_ + size_;}
	std::size_t size() const			{return size_;}
	bool empty() const					{return size() == 0;}
	const boost::any& storage() const	{return *storage_;}
private:
	uint64_t	id_;
	uint8_t*	ptr_;
	std::size_t	size_;
	spl::unique_ptr<boost::any>	storage_;
};

typedef std::vector<int32_t, tbb::cache_aligned_allocator<int32_t>> audio_buffer;

class const_frame sealed
{
public:	

	// Static Members

	static const const_frame& empty();

	// Constructors

	explicit const_frame(const void* tag = nullptr);
	explicit const_frame(boost::shared_future<const_array> image, 
						audio_buffer audio_buffer, 
						const void* tag, 
						const struct pixel_format_desc& desc, 
						double frame_rate, 
						core::field_mode field_mode);
	~const_frame();

	// Methods

	const_frame(const_frame&& other);
	const_frame& operator=(const_frame&& other);
	const_frame(const const_frame&);
	const_frame& operator=(const const_frame& other);
				
	// Properties
			
	const struct pixel_format_desc& pixel_format_desc() const;

	const_array image_data() const;
	const core::audio_buffer& audio_data() const;
		
	double frame_rate() const;
	core::field_mode field_mode() const;

	std::size_t width() const;
	std::size_t height() const;
	std::size_t size() const;
								
	const void* tag() const;

	bool operator==(const const_frame& other);
	bool operator!=(const const_frame& other);
			
private:
	struct impl;
	spl::shared_ptr<impl> impl_;
};

class mutable_frame sealed
{
	mutable_frame(const mutable_frame&);
	mutable_frame& operator=(const mutable_frame&);
public:	

	// Static Members

	// Constructors

	explicit mutable_frame(std::vector<mutable_array> image_buffers, 
						audio_buffer audio_buffer, 
						const void* tag, 
						const struct pixel_format_desc& desc, 
						double frame_rate, 
						core::field_mode field_mode);
	~mutable_frame();

	// Methods

	mutable_frame(mutable_frame&& other);
	mutable_frame& operator=(mutable_frame&& other);

	void swap(mutable_frame& other);
			
	// Properties
			
	const struct pixel_format_desc& pixel_format_desc() const;

	const mutable_array& image_data(std::size_t index = 0) const;
	const core::audio_buffer& audio_data() const;

	mutable_array& image_data(std::size_t index = 0);
	core::audio_buffer& audio_data();
	
	double frame_rate() const;
	core::field_mode field_mode() const;

	std::size_t width() const;
	std::size_t height() const;
								
	const void* tag() const;
			
private:
	struct impl;
	spl::unique_ptr<impl> impl_;
};
}}