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

#include "reroute_producer.h"
#include "layer_producer.h"
#include "channel_producer.h"

#include <common/param.h>

#include <core/producer/frame_producer.h>
#include <core/video_channel.h>
#include <core/help/help_sink.h>
#include <core/help/help_repository.h>

#include <boost/algorithm/string.hpp>
#include <boost/range/algorithm/find_if.hpp>

namespace caspar { namespace reroute {

void describe_producer(core::help_sink& sink, const core::help_repository& repository)
{
	sink.short_description(L"Reroutes a complete channel or a layer to another layer.");
	sink.syntax(L"route://[source_channel:int]{-[source_layer:int]} {FRAMES_DELAY [frames_delay:int]} {[no_auto_deinterlace:NO_AUTO_DEINTERLACE]}");
	sink.para()->text(L"Reroutes the composited video of a channel or the untransformed video of a layer.");
	sink.para()
		->text(L"If ")->code(L"source_layer")->text(L" is specified, only the video of the source layer is rerouted. ")
		->text(L"If on the other hand only ")->code(L"source_channel")->text(L" is specified, the video of the complete channel is rerouted.");
	sink.para()
		->text(L"An optional additional delay can be specified with the ")->code(L"frames_delay")
		->text(L" parameter.");
	sink.para()
		->text(L"For channel routing an optional ")->code(L"no_auto_deinterlace")
		->text(L" parameter can be specified, when performance is more important than good quality output.");
	sink.para()->text(L"Examples:");
	sink.example(L">> PLAY 1-10 route://1-11", L"Play the contents of layer 1-11 on layer 1-10 as well.");
	sink.example(L">> PLAY 1-10 route://2", L"Play the composited contents of channel 2 on layer 1-10 as well.");
	sink.example(L">> PLAY 1-10 route://2 NO_AUTO_DEINTERLACE");
	sink.example(
		L">> MIXER 1-10 FILL 0.02 0.01 0.9 0.9\n"
		L">> PLAY 1-10 route://1\n"
		L">> PLAY 1-9 AMB LOOP", L"Play the composited contents of channel 1 on layer 1-10. Since the source and destination channel is the same, an \"infinity\" effect is created.");
	sink.example(L">> PLAY 1-10 route://1-11 FRAMES_DELAY 10", L"Play the contents of layer 1-11 on layer 1-10 as well with an added 10 frames delay.");
	sink.para()
		->text(L"Always expect a few frames delay on the routed-to layer in addition to the optionally specified ")
		->code(L"frames_delay")->text(L" parameter.");
}

spl::shared_ptr<core::frame_producer> create_producer(
		const core::frame_producer_dependencies& dependencies,
		const std::vector<std::wstring>& params)
{
	static const std::wstring PREFIX = L"route://";

	if (params.empty() ||
		!boost::starts_with(params.at(0), PREFIX) ||
		boost::ends_with(params.at(0), L"_A") ||
		boost::ends_with(params.at(0), L"_ALPHA"))
	{
		return core::frame_producer::empty();
	}

	auto& url = params.at(0);
	auto channel_layer_spec = url.substr(PREFIX.length());
	auto dash = channel_layer_spec.find(L"-");
	bool has_layer_spec = dash != std::wstring::npos;
	int channel_id;

	if (has_layer_spec)
		channel_id = boost::lexical_cast<int>(channel_layer_spec.substr(0, dash));
	else
		channel_id = boost::lexical_cast<int>(channel_layer_spec);

	auto found_channel = boost::find_if(dependencies.channels, [=](spl::shared_ptr<core::video_channel> ch) { return ch->index() == channel_id; });

	if (found_channel == dependencies.channels.end())
		CASPAR_THROW_EXCEPTION(user_error() << msg_info(L"No channel with id " + boost::lexical_cast<std::wstring>(channel_id)));

	auto params2 = params;
	params2.erase(params2.begin());

	auto frames_delay			= get_param(L"FRAMES_DELAY", params2, 0);
	bool no_auto_deinterlace	= contains_param(L"NO_AUTO_DEINTERLACE", params2);

	if (has_layer_spec)
	{
		auto layer = boost::lexical_cast<int>(channel_layer_spec.substr(dash + 1));

		return create_layer_producer(*found_channel, layer, frames_delay, dependencies.format_desc);
	}
	else
	{
		return create_channel_producer(dependencies, *found_channel, frames_delay, no_auto_deinterlace);
	}
}

}}
