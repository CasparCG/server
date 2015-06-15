#pragma once

#undef BOOST_PARAMETER_MAX_ARITY
#define BOOST_PARAMETER_MAX_ARITY 7

#include "../fwd.h"

#include <common/memory.h>
#include <common/forward.h>
#include <common/array.h>
#include <common/future_fwd.h>
#include <common/cache_aligned_vector.h>

#include <cstddef>
#include <cstdint>

FORWARD1(boost, template<typename> class shared_future);

namespace caspar { namespace core {
	
typedef cache_aligned_vector<int32_t> audio_buffer;
class frame_geometry;

class mutable_frame final
{
	mutable_frame(const mutable_frame&);
	mutable_frame& operator=(const mutable_frame&);
public:	

	// Static Members

	// Constructors

	explicit mutable_frame(std::vector<array<std::uint8_t>> image_buffers, 
						audio_buffer audio_buffer, 
						const void* tag, 
						const pixel_format_desc& desc);
	~mutable_frame();

	// Methods

	mutable_frame(mutable_frame&& other);
	mutable_frame& operator=(mutable_frame&& other);

	void swap(mutable_frame& other);
			
	// Properties
			
	const pixel_format_desc& pixel_format_desc() const;

	const array<std::uint8_t>& image_data(std::size_t index = 0) const;
	const core::audio_buffer& audio_data() const;

	array<std::uint8_t>& image_data(std::size_t index = 0);
	core::audio_buffer& audio_data();
	
	std::size_t width() const;
	std::size_t height() const;
								
	const void* stream_tag() const;
	const void* data_tag() const;

	const core::frame_geometry& geometry() const;
	void set_geometry(const frame_geometry& g);
			
private:
	struct impl;
	spl::unique_ptr<impl> impl_;
};

class const_frame final
{
public:	

	// Static Members

	static const const_frame& empty();

	// Constructors

	explicit const_frame(const void* tag = nullptr);
	explicit const_frame(std::shared_future<array<const std::uint8_t>> image, 
						audio_buffer audio_buffer, 
						const void* tag, 
						const pixel_format_desc& desc);
	const_frame(mutable_frame&& other);
	~const_frame();

	// Methods

	const_frame(const_frame&& other);
	const_frame& operator=(const_frame&& other);
	const_frame(const const_frame&);
	const_frame& operator=(const const_frame& other);
				
	// Properties
				
	const pixel_format_desc& pixel_format_desc() const;

	array<const std::uint8_t> image_data(int index = 0) const;
	const core::audio_buffer& audio_data() const;
		
	std::size_t width() const;
	std::size_t height() const;
	std::size_t size() const;
								
	const void* stream_tag() const;
	const void* data_tag() const;

	const core::frame_geometry& geometry() const;
	void set_geometry(const frame_geometry& g);

	bool operator==(const const_frame& other);
	bool operator!=(const const_frame& other);
	bool operator<(const const_frame& other);
	bool operator>(const const_frame& other);
			
private:
	struct impl;
	spl::shared_ptr<impl> impl_;
};

}}