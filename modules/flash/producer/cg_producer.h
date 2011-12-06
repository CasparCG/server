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

#pragma once

#include <core/producer/frame_producer.h>
#include <core/producer/stage.h>
#include <core/video_format.h>
#include <core/video_channel.h>

#include <boost/thread/future.hpp>

#include <string>

namespace caspar { namespace flash {
		
class cg_producer : public core::frame_producer
{
public:
	static const unsigned int DEFAULT_LAYER = 9999;

	explicit cg_producer(const safe_ptr<core::frame_producer>& producer);
	cg_producer(cg_producer&& other);
	
	// frame_producer

	virtual safe_ptr<core::basic_frame> receive(int) override;
	virtual safe_ptr<core::basic_frame> last_frame() const override;
	virtual std::string print() const override;
	virtual boost::unique_future<std::string> call(const std::string&) override;
	virtual boost::property_tree::ptree info() const override;

	//cg_producer

	void add(int layer, const std::string& template_name,  bool play_on_load, const std::string& start_from_label = "", const std::string& data = "");
	void remove(int layer);
	void play(int layer);
	void stop(int layer, unsigned int mix_out_duration);
	void next(int layer);
	void update(int layer, const std::string& data);
	std::string invoke(int layer, const std::string& label);
	std::string description(int layer);
	std::string template_host_info();

private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};
safe_ptr<cg_producer> get_default_cg_producer(const safe_ptr<core::video_channel>& video_channel, int layer_index = cg_producer::DEFAULT_LAYER);

safe_ptr<core::frame_producer> create_ct_producer(const safe_ptr<core::frame_factory> frame_factory, const std::vector<std::string>& params);
safe_ptr<core::frame_producer> create_cg_producer(const safe_ptr<core::frame_factory> frame_factory, const std::vector<std::string>& params);

}}