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
* Author: Niklas P Andersson, niklas.p.andersson@svt.se
*/

#pragma once

#include "../frame_producer.h"

#include <common/memory.h>

#include <boost/property_tree/ptree_fwd.hpp>
#include <string>
#include <vector>

#include "utils/color.h"
#include "utils/string_metrics.h"
#include "utils/text_info.h"

namespace caspar { namespace core {
	namespace text 
	{
		void init();
	}

class text_producer : public frame_producer_base
{
public:
	text_producer(const spl::shared_ptr<frame_factory>& frame_factory, int x, int y, const std::wstring& str, text::text_info& text_info, long parent_width, long parent_height, bool standalone);
	static spl::shared_ptr<text_producer> create(const spl::shared_ptr<class frame_factory>& frame_factory, int x, int y, const std::wstring& str, text::text_info& text_info, long parent_width, long parent_height, bool standalone = false);
	
	draw_frame receive_impl() override;
	boost::unique_future<std::wstring> call(const std::vector<std::wstring>& param) override;
	variable& get_variable(const std::wstring& name) override;
	const std::vector<std::wstring>& get_variables() const override;

	text::string_metrics measure_string(const std::wstring& str);

	constraints& pixel_constraints() override;
	std::wstring print() const override;
	std::wstring name() const override;
	boost::property_tree::wptree info() const override;
	void subscribe(const monitor::observable::observer_ptr& o) override;
	void unsubscribe(const monitor::observable::observer_ptr& o) override;

	binding<std::wstring>& text();
	binding<double>& tracking();
	const binding<double>& current_bearing_y() const;
	const binding<double>& current_protrude_under_y() const;
private:
	struct impl;
	spl::unique_ptr<impl> impl_;
};

spl::shared_ptr<frame_producer> create_text_producer(const spl::shared_ptr<class frame_factory>& frame_factory, const video_format_desc& format_desc, const std::vector<std::wstring>& params);


}}
