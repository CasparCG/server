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
* Author: Helge Norberg, helge.norberg@svt.se
*/

#include "../StdAfx.h"

#include "subject_diagnostics.h"

#include "call_context.h"

#include <common/diagnostics/graph.h>

#include <boost/lexical_cast.hpp>
#include <boost/timer.hpp>

#include <atomic>
#include <mutex>
#include <unordered_map>

namespace {

int64_t create_id()
{
	static std::atomic<int64_t> counter { 0 };

	return ++counter;
}

}

namespace caspar { namespace core { namespace diagnostics {

static const double SECONDS_BETWEEN_FULL_STATE = 0.4;

class subject_graph : public caspar::diagnostics::spi::graph_sink
{
	spl::shared_ptr<monitor::subject>		subject_						= spl::make_shared<monitor::subject>("/" + boost::lexical_cast<std::string>(create_id()));
	call_context							context_						= call_context::for_thread();
	std::mutex								mutex_;
	std::wstring							text_;
	std::unordered_map<std::string, int>	colors_;
	boost::timer							time_since_full_state_send_;
public:
	subject_graph()
	{
	}

	void activate() override
	{
		subject_->attach_parent(get_or_create_subject());
	}

	void set_text(const std::wstring& value) override
	{
		std::lock_guard<std::mutex> lock(mutex_);

		text_ = value;

		*subject_ << monitor::message("/text") % text_;
	}

	void set_value(const std::string& name, double value) override
	{
		*subject_ << monitor::message("/value/" + name) % value;

		send_full_state_if_long_ago();
	}
	
	void set_color(const std::string& name, int color) override
	{
		std::lock_guard<std::mutex> lock(mutex_);

		colors_[name] = color;

		*subject_ << monitor::message("/color/" + name) % color;
	}
	
	void set_tag(const std::string& name) override
	{
		*subject_ << monitor::message("/tag/" + name);

		send_full_state_if_long_ago();
	}
	
	void auto_reset() override
	{
	}

	monitor::subject& subject()
	{
		return *subject_;
	}
private:
	void send_full_state_if_long_ago()
	{
		std::lock_guard<std::mutex> lock(mutex_);

		if (time_since_full_state_send_.elapsed() > SECONDS_BETWEEN_FULL_STATE)
		{
			send_full_state();
		}
	}

	void send_full_state()
	{
		*subject_ << monitor::message("/text") % text_;

		if (context_.video_channel != -1)
			*subject_ << monitor::message("/context/channel") % context_.video_channel;

		if (context_.layer != -1)
			*subject_ << monitor::message("/context/layer") % context_.layer;

		for (const auto& color : colors_)
			*subject_ << monitor::message("/color/" + color.first) % color.second;

		time_since_full_state_send_.restart();
	}
};

spl::shared_ptr<monitor::subject> get_or_create_subject()
{
	static auto diag_subject = []()
	{
		auto subject = spl::make_shared<monitor::subject>("/diag");

		caspar::diagnostics::spi::register_sink_factory([=]()
		{
			return spl::make_shared<subject_graph>();
		});

		return subject;
	}();

	return diag_subject;
}

}}}
