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

#include "../frame_producer.h"
#include "../../frame/draw_frame.h"

#include <tbb/parallel_invoke.h>

namespace caspar { namespace core {	

struct separated_producer : public frame_producer
{		
	spl::shared_ptr<frame_producer>	fill_producer_;
	spl::shared_ptr<frame_producer>	key_producer_;
	spl::shared_ptr<draw_frame>		fill_;
	spl::shared_ptr<draw_frame>		key_;
	spl::shared_ptr<draw_frame>		last_frame_;
		
	explicit separated_producer(const spl::shared_ptr<frame_producer>& fill, const spl::shared_ptr<frame_producer>& key) 
		: fill_producer_(fill)
		, key_producer_(key)
		, fill_(core::draw_frame::late())
		, key_(core::draw_frame::late())
		, last_frame_(core::draw_frame::empty())
	{
	}

	// frame_producer
	
	virtual spl::shared_ptr<draw_frame> receive(int flags) override
	{
		tbb::parallel_invoke(
		[&]
		{
			if(fill_ == core::draw_frame::late())
				fill_ = fill_producer_->receive(flags);
		},
		[&]
		{
			if(key_ == core::draw_frame::late())
				key_ = key_producer_->receive(flags | frame_producer::flags::alpha_only);
		});

		if(fill_ == draw_frame::eof() || key_ == draw_frame::eof())
			return draw_frame::eof();

		if(fill_ == core::draw_frame::late() || key_ == core::draw_frame::late()) // One of the producers is lagging, keep them in sync.
			return core::draw_frame::late();
		
		auto frame = draw_frame::mask(fill_, key_);

		fill_ = draw_frame::late();
		key_  = draw_frame::late();

		return last_frame_ = frame;
	}

	virtual spl::shared_ptr<core::draw_frame> last_frame() const override
	{
		return last_frame_;
	}

	virtual uint32_t nb_frames() const override
	{
		return std::min(fill_producer_->nb_frames(), key_producer_->nb_frames());
	}

	virtual std::wstring print() const override
	{
		return L"separated[fill:" + fill_producer_->print() + L"|key[" + key_producer_->print() + L"]]";
	}	

	virtual std::wstring name() const override
	{
		return L"separated";
	}

	boost::property_tree::wptree info() const override
	{
		boost::property_tree::wptree info;
		info.add(L"type", L"separated-producer");
		info.add_child(L"fill.producer",	fill_producer_->info());
		info.add_child(L"key.producer",	key_producer_->info());
		return info;
	}

	virtual void subscribe(const monitor::observable::observer_ptr& o) override															
	{
		return fill_producer_->subscribe(o);
	}

	virtual void unsubscribe(const monitor::observable::observer_ptr& o) override		
	{
		return fill_producer_->unsubscribe(o);
	}
};

spl::shared_ptr<frame_producer> create_separated_producer(const spl::shared_ptr<frame_producer>& fill, const spl::shared_ptr<frame_producer>& key)
{
	return core::wrap_producer(spl::make_shared<separated_producer>(fill, key));
}

}}

