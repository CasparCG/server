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

#include "../../stdafx.h"

#include "separated_producer.h"

#include <core/producer/frame/basic_frame.h>

#include <tbb/parallel_invoke.h>

namespace caspar { namespace core {	

struct separated_producer : public frame_producer
{		
	safe_ptr<frame_producer>	fill_producer_;
	safe_ptr<frame_producer>	key_producer_;
	safe_ptr<basic_frame>		fill_;
	safe_ptr<basic_frame>		key_;
	safe_ptr<basic_frame>		last_frame_;
		
	explicit separated_producer(const safe_ptr<frame_producer>& fill, const safe_ptr<frame_producer>& key) 
		: fill_producer_(fill)
		, key_producer_(key)
		, fill_(core::basic_frame::late())
		, key_(core::basic_frame::late())
		, last_frame_(core::basic_frame::empty())
	{
	}

	// frame_producer
	
	virtual safe_ptr<basic_frame> receive(int hints) override
	{
		tbb::parallel_invoke(
		[&]
		{
			if(fill_ == core::basic_frame::late())
				fill_ = receive_and_follow(fill_producer_, hints);
		},
		[&]
		{
			if(key_ == core::basic_frame::late())
				key_ = receive_and_follow(key_producer_, hints | ALPHA_HINT);
		});

		if(fill_ == basic_frame::eof() || key_ == basic_frame::eof())
			return basic_frame::eof();

		if(fill_ == core::basic_frame::late() || key_ == core::basic_frame::late()) // One of the producers is lagging, keep them in sync.
			return core::basic_frame::late();
		
		auto frame = basic_frame::fill_and_key(fill_, key_);

		fill_ = basic_frame::late();
		key_  = basic_frame::late();

		return last_frame_ = frame;
	}

	virtual safe_ptr<core::basic_frame> last_frame() const override
	{
		return disable_audio(last_frame_);
	}

	virtual safe_ptr<basic_frame> create_thumbnail_frame() override
	{
		auto fill_frame = fill_producer_->create_thumbnail_frame();
		auto key_frame = key_producer_->create_thumbnail_frame();

		if (fill_frame == basic_frame::empty() || key_frame == basic_frame::empty())
			return basic_frame::empty();

		if (fill_frame == basic_frame::eof() || key_frame == basic_frame::eof())
			return basic_frame::eof();

		if (fill_frame == basic_frame::late() || key_frame == basic_frame::late())
			return basic_frame::late();

		return basic_frame::fill_and_key(fill_frame, key_frame);
	}

	virtual uint32_t nb_frames() const override
	{
		return std::min(fill_producer_->nb_frames(), key_producer_->nb_frames());
	}

	virtual std::wstring print() const override
	{
		return L"separated[fill:" + fill_producer_->print() + L"|key[" + key_producer_->print() + L"]]";
	}	

	boost::property_tree::wptree info() const override
	{
		boost::property_tree::wptree info;
		info.add(L"type", L"separated-producer");
		info.add_child(L"fill.producer",	fill_producer_->info());
		info.add_child(L"key.producer",	key_producer_->info());
		return info;
	}
};

safe_ptr<frame_producer> create_separated_producer(const safe_ptr<frame_producer>& fill, const safe_ptr<frame_producer>& key)
{
	return create_producer_print_proxy(
			make_safe<separated_producer>(fill, key));
}

safe_ptr<frame_producer> create_separated_thumbnail_producer(const safe_ptr<frame_producer>& fill, const safe_ptr<frame_producer>& key)
{
	return make_safe<separated_producer>(fill, key);
}

}}

