/*
* Copyright (c) 2011 Sveriges Television AB <info@casparcg.com>
*
* This file is part of CasparCG (www.casparcg.com).
*
* CasparCG is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* CasparCG is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with CasparCG. If not, see <http://www.gnu.org/licenses/>.
*
* Author: Robert Nagy, ronag89@gmail.com
*/

#include "../StdAfx.h"

#include "frame.h"

#include <common/except.h>
#include <common/array.h>
#include <common/future.h>
#include <common/timer.h>
#include <common/memshfl.h>

#include <core/frame/frame_visitor.h>
#include <core/frame/pixel_format.h>
#include <core/frame/geometry.h>
#include <core/frame/audio_channel_layout.h>

#include <cstdint>
#include <vector>

#include <boost/lexical_cast.hpp>

namespace caspar { namespace core {

struct mutable_frame::impl : boost::noncopyable
{
	std::vector<array<std::uint8_t>>			buffers_;
	core::mutable_audio_buffer					audio_data_;
	const core::pixel_format_desc				desc_;
	const core::audio_channel_layout			channel_layout_;
	const void*									tag_;
	core::frame_geometry						geometry_				= frame_geometry::get_default();
	caspar::timer								since_created_timer_;

	impl(
			std::vector<array<std::uint8_t>> buffers,
			mutable_audio_buffer audio_data,
			const void* tag,
			const core::pixel_format_desc& desc,
			const core::audio_channel_layout& channel_layout)
		: buffers_(std::move(buffers))
		, audio_data_(std::move(audio_data))
		, desc_(desc)
		, channel_layout_(channel_layout)
		, tag_(tag)
	{
		for (auto& buffer : buffers_)
			if(!buffer.data())
				CASPAR_THROW_EXCEPTION(invalid_argument() << msg_info("mutable_frame: null argument"));
	}
};

mutable_frame::mutable_frame(
		std::vector<array<std::uint8_t>> image_buffers,
		mutable_audio_buffer audio_data,
		const void* tag,
		const core::pixel_format_desc& desc,
		const core::audio_channel_layout& channel_layout)
	: impl_(new impl(std::move(image_buffers), std::move(audio_data), tag, desc, channel_layout)){}
mutable_frame::~mutable_frame(){}
mutable_frame::mutable_frame(mutable_frame&& other) : impl_(std::move(other.impl_)){}
mutable_frame& mutable_frame::operator=(mutable_frame&& other)
{
	impl_ = std::move(other.impl_);
	return *this;
}
void mutable_frame::swap(mutable_frame& other){impl_.swap(other.impl_);}
const core::pixel_format_desc& mutable_frame::pixel_format_desc() const{return impl_->desc_;}
const core::audio_channel_layout& mutable_frame::audio_channel_layout() const { return impl_->channel_layout_; }
const array<std::uint8_t>& mutable_frame::image_data(std::size_t index) const{return impl_->buffers_.at(index);}
const core::mutable_audio_buffer& mutable_frame::audio_data() const{return impl_->audio_data_;}
array<std::uint8_t>& mutable_frame::image_data(std::size_t index){return impl_->buffers_.at(index);}
core::mutable_audio_buffer& mutable_frame::audio_data(){return impl_->audio_data_;}
std::size_t mutable_frame::width() const{return impl_->desc_.planes.at(0).width;}
std::size_t mutable_frame::height() const{return impl_->desc_.planes.at(0).height;}
const void* mutable_frame::stream_tag()const{return impl_->tag_;}
const frame_geometry& mutable_frame::geometry() const { return impl_->geometry_; }
void mutable_frame::set_geometry(const frame_geometry& g) { impl_->geometry_ = g; }
caspar::timer mutable_frame::since_created() const { return impl_->since_created_timer_; }

const const_frame& const_frame::empty()
{
	static int dummy;
	static const_frame empty(&dummy);
	return empty;
}

struct const_frame::impl : boost::noncopyable
{
	mutable std::vector<std::shared_future<array<const std::uint8_t>>>	future_buffers_;
	mutable core::audio_buffer											audio_data_;
	const core::pixel_format_desc										desc_;
	const core::audio_channel_layout									channel_layout_;
	const void*															tag_;
	core::frame_geometry												geometry_;
	caspar::timer														since_created_timer_;
	bool																should_record_age_;
	mutable std::atomic<int64_t>										recorded_age_;
	std::shared_future<array<const std::uint8_t>>						key_only_on_demand_;

	impl(const void* tag)
		: audio_data_(0, 0, true, 0)
		, desc_(core::pixel_format::invalid)
		, channel_layout_(audio_channel_layout::invalid())
		, tag_(tag)
		, geometry_(frame_geometry::get_default())
		, should_record_age_(true)
	{
		recorded_age_ = 0;
	}

	impl(const impl& other)
		: future_buffers_(other.future_buffers_)
		, audio_data_(other.audio_data_)
		, desc_(other.desc_)
		, channel_layout_(other.channel_layout_)
		, tag_(other.tag_)
		, geometry_(other.geometry_)
		, since_created_timer_(other.since_created_timer_)
		, should_record_age_(other.should_record_age_)
		, key_only_on_demand_(other.key_only_on_demand_)
	{
		recorded_age_ = other.recorded_age_.load();
	}

	impl(
			std::shared_future<array<const std::uint8_t>> image,
			audio_buffer audio_data,
			const void* tag,
			const core::pixel_format_desc& desc,
			const core::audio_channel_layout& channel_layout,
			caspar::timer since_created_timer = caspar::timer())
		: audio_data_(std::move(audio_data))
		, desc_(desc)
		, channel_layout_(channel_layout)
		, tag_(tag)
		, geometry_(frame_geometry::get_default())
		, since_created_timer_(std::move(since_created_timer))
		, should_record_age_(false)
	{
		if (desc.format != core::pixel_format::bgra)
			CASPAR_THROW_EXCEPTION(not_implemented());

		future_buffers_.push_back(image);

		key_only_on_demand_ = std::async(std::launch::deferred, [image]
		{
			auto fill	= image.get();
			auto key	= cache_aligned_vector<std::uint8_t>(fill.size());

			aligned_memshfl(key.data(), fill.data(), fill.size(), 0x0F0F0F0F, 0x0B0B0B0B, 0x07070707, 0x03030303);

			return array<const std::uint8_t>(key.data(), key.size(), false, std::move(key));
		}).share();
	}

	impl(mutable_frame&& other)
		: audio_data_(0, 0, true, 0) // Complex init done in body instead.
		, desc_(other.pixel_format_desc())
		, channel_layout_(other.audio_channel_layout())
		, tag_(other.stream_tag())
		, geometry_(other.geometry())
		, since_created_timer_(other.since_created())
		, should_record_age_(true)
	{
		spl::shared_ptr<mutable_audio_buffer> shared_audio_data(new mutable_audio_buffer(std::move(other.audio_data())));
		// pointer returned by vector::data() should be the same after move, but just to be safe.
		audio_data_ = audio_buffer(shared_audio_data->data(), shared_audio_data->size(), true, std::move(shared_audio_data));

		for (std::size_t n = 0; n < desc_.planes.size(); ++n)
		{
			future_buffers_.push_back(make_ready_future<array<const std::uint8_t>>(std::move(other.image_data(n))).share());
		}

		recorded_age_ = -1;
	}

	array<const std::uint8_t> image_data(int index) const
	{
		return tag_ != empty().stream_tag() ? future_buffers_.at(index).get() : array<const std::uint8_t>(nullptr, 0, true, 0);
	}

	spl::shared_ptr<impl> key_only() const
	{
		return spl::make_shared<impl>(key_only_on_demand_, audio_data_, tag_, desc_, channel_layout_, since_created_timer_);
	}

	std::size_t width() const
	{
		return tag_ != empty().stream_tag() ? desc_.planes.at(0).width : 0;
	}

	std::size_t height() const
	{
		return tag_ != empty().stream_tag() ? desc_.planes.at(0).height : 0;
	}

	std::size_t size() const
	{
		return tag_ != empty().stream_tag() ? desc_.planes.at(0).size : 0;
	}

	int64_t get_age_millis() const
	{
		if (should_record_age_)
		{
			if (recorded_age_ == -1)
				recorded_age_ = static_cast<int64_t>(since_created_timer_.elapsed() * 1000.0);

			return recorded_age_;
		}
		else
			return static_cast<int64_t>(since_created_timer_.elapsed() * 1000.0);
	}
};

const_frame::const_frame(const void* tag) : impl_(new impl(tag)){}
const_frame::const_frame(
		std::shared_future<array<const std::uint8_t>> image,
		audio_buffer audio_data,
		const void* tag,
		const core::pixel_format_desc& desc,
		const core::audio_channel_layout& channel_layout)
	: impl_(new impl(std::move(image), std::move(audio_data), tag, desc, channel_layout)){}
const_frame::const_frame(mutable_frame&& other) : impl_(new impl(std::move(other))){}
const_frame::~const_frame(){}
const_frame::const_frame(const_frame&& other) : impl_(std::move(other.impl_)){}
const_frame& const_frame::operator=(const_frame&& other)
{
	impl_ = std::move(other.impl_);
	return *this;
}
const_frame::const_frame(const const_frame& other) : impl_(other.impl_){}
const_frame::const_frame(const const_frame::impl& other) : impl_(new impl(other)) {}
const_frame& const_frame::operator=(const const_frame& other)
{
	impl_ = other.impl_;
	return *this;
}
bool const_frame::operator==(const const_frame& other){return impl_ == other.impl_;}
bool const_frame::operator!=(const const_frame& other){return !(*this == other);}
bool const_frame::operator<(const const_frame& other){return impl_ < other.impl_;}
bool const_frame::operator>(const const_frame& other){return impl_ > other.impl_;}
const core::pixel_format_desc& const_frame::pixel_format_desc()const{return impl_->desc_;}
const core::audio_channel_layout& const_frame::audio_channel_layout()const { return impl_->channel_layout_; }
array<const std::uint8_t> const_frame::image_data(int index)const{return impl_->image_data(index);}
const core::audio_buffer& const_frame::audio_data()const{return impl_->audio_data_;}
std::size_t const_frame::width()const{return impl_->width();}
std::size_t const_frame::height()const{return impl_->height();}
std::size_t const_frame::size()const{return impl_->size();}
const void* const_frame::stream_tag()const{return impl_->tag_;}
const frame_geometry& const_frame::geometry() const { return impl_->geometry_; }
const_frame const_frame::with_geometry(const frame_geometry& g) const
{
	const_frame copy(*impl_);

	copy.impl_->geometry_ = g;

	return copy;
}
int64_t const_frame::get_age_millis() const { return impl_->get_age_millis(); }
const_frame const_frame::key_only() const
{
	auto result		= const_frame();
	result.impl_	= impl_->key_only();

	return result;
}

}}
