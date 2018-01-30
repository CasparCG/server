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

#include "../../StdAfx.h"

#include "separated_producer.h"

#include <core/producer/frame_producer.h>
#include <core/frame/draw_frame.h>
#include <core/monitor/monitor.h>

#include <tbb/parallel_invoke.h>

#include <future>

namespace caspar { namespace core {

class separated_producer : public frame_producer_base
{
	spl::shared_ptr<monitor::subject>	monitor_subject_;
	spl::shared_ptr<monitor::subject>	key_monitor_subject_	= spl::make_shared<monitor::subject>("/keyer");

	spl::shared_ptr<frame_producer>	fill_producer_;
	spl::shared_ptr<frame_producer>	key_producer_;
	draw_frame						fill_						= core::draw_frame::late();
	draw_frame						key_						= core::draw_frame::late();

public:
	explicit separated_producer(const spl::shared_ptr<frame_producer>& fill, const spl::shared_ptr<frame_producer>& key)
		: fill_producer_(fill)
		, key_producer_(key)
	{
		key_monitor_subject_->attach_parent(monitor_subject_);

		key_producer_->monitor_output().attach_parent(key_monitor_subject_);
		fill_producer_->monitor_output().attach_parent(monitor_subject_);

		CASPAR_LOG(debug) << print() << L" Initialized";
	}

	// frame_producer

	draw_frame receive_impl() override
	{
		tbb::parallel_invoke(
		[&]
		{
			if(fill_ == core::draw_frame::late())
				fill_ = fill_producer_->receive();
		},
		[&]
		{
			if(key_ == core::draw_frame::late())
				key_ = key_producer_->receive();
		});

		if(fill_ == core::draw_frame::late() || key_ == core::draw_frame::late()) // One of the producers is lagging, keep them in sync.
			return core::draw_frame::late();

		auto frame = draw_frame::mask(fill_, key_);

		fill_ = draw_frame::late();
		key_  = draw_frame::late();

		return frame;
	}

	draw_frame last_frame()
	{
		return draw_frame::mask(fill_producer_->last_frame(), key_producer_->last_frame());
	}

	uint32_t nb_frames() const override
	{
		return std::min(fill_producer_->nb_frames(), key_producer_->nb_frames());
	}

	std::wstring print() const override
	{
		return L"separated[fill:" + fill_producer_->print() + L"|key[" + key_producer_->print() + L"]]";
	}

	std::future<std::wstring> call(const std::vector<std::wstring>& params) override
	{
		key_producer_->call(params);
		return fill_producer_->call(params);
	}

	std::wstring name() const override
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

	monitor::subject& monitor_output() { return *monitor_subject_; }
};

spl::shared_ptr<frame_producer> create_separated_producer(const spl::shared_ptr<frame_producer>& fill, const spl::shared_ptr<frame_producer>& key)
{
	return spl::make_shared<separated_producer>(fill, key);
}

}}

