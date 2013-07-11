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

#include "psd.h"
#include "layer.h"
#include "doc.h"

#include <core/frame/pixel_format.h>
#include <core/frame/frame_factory.h>
#include <core/producer/frame_producer.h>
#include <core/producer/scene/scene_producer.h>
#include <core/producer/scene/const_producer.h>
#include <core/frame/draw_frame.h>

#include <common/env.h>

#include <boost/filesystem.hpp>

namespace caspar { namespace psd {

void init()
{
	core::register_producer_factory(create_producer);
}

spl::shared_ptr<core::frame_producer> create_producer(const spl::shared_ptr<core::frame_factory>& frame_factory, const core::video_format_desc& format_desc, const std::vector<std::wstring>& params)
{
	std::wstring filename = env::media_folder() + L"\\" + params[0] + L".psd";
	if(!boost::filesystem::is_regular_file(boost::filesystem::path(filename)))
		return core::frame_producer::empty();

	psd_document doc;
	if(!doc.parse(filename))
		return core::frame_producer::empty();

	spl::shared_ptr<core::scene::scene_producer> root(spl::make_shared<core::scene::scene_producer>(doc.width(), doc.height()));

	auto layers_end = doc.layers().end();
	for(auto it = doc.layers().begin(); it != layers_end; ++it)
	{
		if((*it)->image())
		{
			core::pixel_format_desc pfd(core::pixel_format::bgra);
			pfd.planes.push_back(core::pixel_format_desc::plane((*it)->rect().width(), (*it)->rect().height(), 4));

			auto frame = frame_factory->create_frame(it->get(), pfd);
			memcpy(frame.image_data().data(), (*it)->image()->data(), frame.image_data().size());

			auto layer_producer = core::create_const_producer(core::draw_frame(std::move(frame)), (*it)->rect().width(), (*it)->rect().height());
			auto& new_layer = root->create_layer(layer_producer, (*it)->rect().left, (*it)->rect().top);
			new_layer.adjustments.opacity.set((*it)->opacity() / 255.0);
			new_layer.hidden.set(!(*it)->visible());
		}
	}
	
	return root;
}

}}