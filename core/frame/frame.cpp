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
#include <common/array.h>

#include <core/frame/frame_visitor.h>
#include <core/frame/pixel_format.h>

#include <boost/lexical_cast.hpp>
#include <boost/thread/future.hpp>

namespace caspar { namespace core {
		
struct mutable_frame::impl : boost::noncopyable
{			
	std::vector<array<std::uint8_t>>			buffers_;
	core::audio_buffer							audio_data_;
	const core::pixel_format_desc				desc_;
	const void*									tag_;
	
	impl(std::vector<array<std::uint8_t>> buffers, audio_buffer audio_buffer, const void* tag, const core::pixel_format_desc& desc) 
		: buffers_(std::move(buffers))
		, audio_data_(std::move(audio_buffer))
		, desc_(desc)
		, tag_(tag)
	{
		BOOST_FOREACH(auto& buffer, buffers_)
			if(!buffer.data())
				CASPAR_THROW_EXCEPTION(invalid_argument() << msg_info("mutable_frame: null argument"));
	}
};
	
mutable_frame::mutable_frame(std::vector<array<std::uint8_t>> image_buffers, audio_buffer audio_buffer, const void* tag, const core::pixel_format_desc& desc) 
	: impl_(new impl(std::move(image_buffers), std::move(audio_buffer), tag, desc)){}
mutable_frame::~mutable_frame(){}
mutable_frame::mutable_frame(mutable_frame&& other) : impl_(std::move(other.impl_)){}
mutable_frame& mutable_frame::operator=(mutable_frame&& other)
{
	impl_ = std::move(other.impl_);
	return *this;
}
void mutable_frame::swap(mutable_frame& other){impl_.swap(other.impl_);}
const core::pixel_format_desc& mutable_frame::pixel_format_desc() const{return impl_->desc_;}
const array<std::uint8_t>& mutable_frame::image_data(std::size_t index) const{return impl_->buffers_.at(index);}
const core::audio_buffer& mutable_frame::audio_data() const{return impl_->audio_data_;}
array<std::uint8_t>& mutable_frame::image_data(std::size_t index){return impl_->buffers_.at(index);}
core::audio_buffer& mutable_frame::audio_data(){return impl_->audio_data_;}
std::size_t mutable_frame::width() const{return impl_->desc_.planes.at(0).width;}
std::size_t mutable_frame::height() const{return impl_->desc_.planes.at(0).height;}						
const void* mutable_frame::stream_tag()const{return impl_->tag_;}				
const void* mutable_frame::data_tag()const{return impl_.get();}	

const const_frame& const_frame::empty()
{
	static int dummy;
	static const_frame empty(&dummy);
	return empty;
}

struct const_frame::impl : boost::noncopyable
{			
	mutable std::vector<boost::shared_future<array<const std::uint8_t>>>	future_buffers_;
	int											id_;
	core::audio_buffer							audio_data_;
	const core::pixel_format_desc				desc_;
	const void*									tag_;

	impl(const void* tag)
		: desc_(core::pixel_format::invalid)
		, tag_(tag)	
		, id_(0)
	{
	}
	
	impl(boost::shared_future<array<const std::uint8_t>> image, audio_buffer audio_buffer, const void* tag, const core::pixel_format_desc& desc) 
		: audio_data_(std::move(audio_buffer))
		, desc_(desc)
		, tag_(tag)
		, id_(reinterpret_cast<int>(this))
	{
		if(desc.format != core::pixel_format::bgra)
			CASPAR_THROW_EXCEPTION(not_implemented());
		
		future_buffers_.push_back(std::move(image));
	}

	impl(mutable_frame&& other)
		: audio_data_(other.audio_data())
		, desc_(other.pixel_format_desc())
		, tag_(other.stream_tag())
		, id_(reinterpret_cast<int>(this))
	{
		for(std::size_t n = 0; n < desc_.planes.size(); ++n)
		{
			boost::promise<array<const std::uint8_t>> p;
			p.set_value(std::move(other.image_data(n)));
			future_buffers_.push_back(p.get_future());
		}
	}

	array<const std::uint8_t> image_data(int index) const
	{
		return tag_ != empty().stream_tag() ? future_buffers_.at(index).get() : array<const std::uint8_t>(nullptr, 0, true, 0);
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
};
	
const_frame::const_frame(const void* tag) : impl_(new impl(tag)){}
const_frame::const_frame(boost::shared_future<array<const std::uint8_t>> image, audio_buffer audio_buffer, const void* tag, const core::pixel_format_desc& desc) 
	: impl_(new impl(std::move(image), std::move(audio_buffer), tag, desc)){}
const_frame::const_frame(mutable_frame&& other) : impl_(new impl(std::move(other))){}
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
bool const_frame::operator==(const const_frame& other){return impl_ == other.impl_;}
bool const_frame::operator!=(const const_frame& other){return !(*this == other);}
bool const_frame::operator<(const const_frame& other){return impl_< other.impl_;}
bool const_frame::operator>(const const_frame& other){return impl_> other.impl_;}
const core::pixel_format_desc& const_frame::pixel_format_desc()const{return impl_->desc_;}
array<const std::uint8_t> const_frame::image_data(int index)const{return impl_->image_data(index);}
const core::audio_buffer& const_frame::audio_data()const{return impl_->audio_data_;}
std::size_t const_frame::width()const{return impl_->width();}
std::size_t const_frame::height()const{return impl_->height();}	
std::size_t const_frame::size()const{return impl_->size();}						
const void* const_frame::stream_tag()const{return impl_->tag_;}				
const void* const_frame::data_tag()const{return impl_.get();}	

}}