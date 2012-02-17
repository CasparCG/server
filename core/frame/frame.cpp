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

#include "../stdafx.h"

#include "frame.h"

#include <common/except.h>
#include <core/frame/frame_visitor.h>
#include <core/frame/pixel_format.h>

#include <boost/lexical_cast.hpp>
#include <boost/thread/future.hpp>

namespace caspar { namespace core {
		
const const_frame& const_frame::empty()
{
	static int dummy;
	static const_frame empty(&dummy);
	return empty;
}

struct const_frame::impl : boost::noncopyable
{			
	mutable boost::shared_future<const_array>	future_buffer_;
	int											id_;
	core::audio_buffer							audio_data_;
	const core::pixel_format_desc				desc_;
	const void*									tag_;
	double										frame_rate_;
	core::field_mode							field_mode_;

	impl(const void* tag)
		: desc_(core::pixel_format::invalid)
		, tag_(tag)	
		, id_(0)
		, field_mode_(core::field_mode::empty)
	{
	}
	
	impl(boost::shared_future<const_array> image, audio_buffer audio_buffer, const void* tag, const core::pixel_format_desc& desc, double frame_rate, core::field_mode field_mode) 
		: future_buffer_(std::move(image))
		, audio_data_(std::move(audio_buffer))
		, desc_(desc)
		, tag_(tag)
		, id_(reinterpret_cast<int>(this))
		, frame_rate_(frame_rate)
		, field_mode_(field_mode)
	{
		if(desc.format != core::pixel_format::bgra)
			BOOST_THROW_EXCEPTION(not_implemented());
	}

	const_array image_data() const
	{
		return tag_ != empty().tag() ? future_buffer_.get() : const_array(nullptr, 0, 0);
	}

	std::size_t width() const
	{
		return tag_ != empty().tag() ? desc_.planes.at(0).width : 0;
	}

	std::size_t height() const
	{
		return tag_ != empty().tag() ? desc_.planes.at(0).height : 0;
	}

	std::size_t size() const
	{
		return tag_ != empty().tag() ? desc_.planes.at(0).size : 0;
	}

	bool operator==(const impl& other)
	{
		return tag_ == other.tag_ && id_ == other.id_;
	}
};
	
const_frame::const_frame(const void* tag) : impl_(new impl(tag)){}
const_frame::const_frame(boost::shared_future<const_array> image, audio_buffer audio_buffer, const void* tag, const core::pixel_format_desc& desc, double frame_rate, core::field_mode field_mode) 
	: impl_(new impl(std::move(image), std::move(audio_buffer), tag, desc, frame_rate, field_mode)){}
const_frame::~const_frame(){}
const_frame::const_frame(const_frame&& other) : impl_(std::move(other.impl_)){}
const_frame& const_frame::operator=(const_frame&& other)
{
	impl_ = std::move(other.impl_);
	return *this;
}
const_frame::const_frame(const const_frame& other) : impl_(other.impl_){}
const_frame& const_frame::operator=(const const_frame& other)
{
	impl_ = other.impl_;
	return *this;
}
bool const_frame::operator==(const const_frame& other){return *impl_ == *other.impl_;}
bool const_frame::operator!=(const const_frame& other){return !(*this == other);}
const core::pixel_format_desc& const_frame::pixel_format_desc()const{return impl_->desc_;}
const_array const_frame::image_data()const{return impl_->image_data();}
const core::audio_buffer& const_frame::audio_data()const{return impl_->audio_data_;}
double const_frame::frame_rate()const{return impl_->frame_rate_;}
core::field_mode const_frame::field_mode()const{return impl_->field_mode_;}
std::size_t const_frame::width()const{return impl_->width();}
std::size_t const_frame::height()const{return impl_->height();}	
std::size_t const_frame::size()const{return impl_->size();}						
const void* const_frame::tag()const{return impl_->tag_;}	

struct mutable_frame::impl : boost::noncopyable
{			
	std::vector<mutable_array>					buffers_;
	core::audio_buffer							audio_data_;
	const core::pixel_format_desc				desc_;
	const void*									tag_;
	double										frame_rate_;
	core::field_mode							field_mode_;
	
	impl(std::vector<mutable_array> buffers, audio_buffer audio_buffer, const void* tag, const core::pixel_format_desc& desc, double frame_rate, core::field_mode field_mode) 
		: buffers_(std::move(buffers))
		, audio_data_(std::move(audio_buffer))
		, desc_(desc)
		, tag_(tag)
		, frame_rate_(frame_rate)
		, field_mode_(field_mode)
	{
		BOOST_FOREACH(auto& buffer, buffers_)
			if(!buffer.data())
				BOOST_THROW_EXCEPTION(invalid_argument() << msg_info("mutable_frame: null argument"));
	}
};
	
mutable_frame::mutable_frame(std::vector<mutable_array> image_buffers, audio_buffer audio_buffer, const void* tag, const core::pixel_format_desc& desc, double frame_rate, core::field_mode field_mode) 
	: impl_(new impl(std::move(image_buffers), std::move(audio_buffer), tag, desc, frame_rate, field_mode)){}
mutable_frame::~mutable_frame(){}
mutable_frame::mutable_frame(mutable_frame&& other) : impl_(std::move(other.impl_)){}
mutable_frame& mutable_frame::operator=(mutable_frame&& other)
{
	impl_ = std::move(other.impl_);
	return *this;
}
void mutable_frame::swap(mutable_frame& other){impl_.swap(other.impl_);}
const core::pixel_format_desc& mutable_frame::pixel_format_desc() const{return impl_->desc_;}
const mutable_array& mutable_frame::image_data(std::size_t index) const{return impl_->buffers_.at(index);}
const core::audio_buffer& mutable_frame::audio_data() const{return impl_->audio_data_;}
mutable_array& mutable_frame::image_data(std::size_t index){return impl_->buffers_.at(index);}
core::audio_buffer& mutable_frame::audio_data(){return impl_->audio_data_;}
double mutable_frame::frame_rate() const{return impl_->frame_rate_;}
core::field_mode mutable_frame::field_mode() const{return impl_->field_mode_;}
std::size_t mutable_frame::width() const{return impl_->desc_.planes.at(0).width;}
std::size_t mutable_frame::height() const{return impl_->desc_.planes.at(0).height;}						
const void* mutable_frame::tag() const{return impl_->tag_;}	

}}