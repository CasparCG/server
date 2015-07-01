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

#include "image.h"

#include "producer/image_producer.h"
#include "producer/image_scroll_producer.h"
#include "consumer/image_consumer.h"

#include <core/producer/frame_producer.h>
#include <core/consumer/frame_consumer.h>
#include <core/producer/media_info/media_info.h>
#include <core/producer/media_info/media_info_repository.h>
#include <core/system_info_provider.h>

#include <common/utf.h>

#include <boost/property_tree/ptree.hpp>

#include <FreeImage.h>

namespace caspar { namespace image {

std::wstring version()
{
	return u16(FreeImage_GetVersion());
}

void init(core::module_dependencies dependencies)
{
	FreeImage_Initialise();
	dependencies.producer_registry->register_producer_factory(create_scroll_producer);
	dependencies.producer_registry->register_producer_factory(create_producer);
	dependencies.producer_registry->register_thumbnail_producer_factory(create_thumbnail_producer);
	core::register_consumer_factory(create_consumer);
	dependencies.media_info_repo->register_extractor([](const std::wstring& file, const std::wstring& extension, core::media_info& info)
	{
		if (extension == L".TGA"
			|| extension == L".COL"
			|| extension == L".PNG"
			|| extension == L".JPEG"
			|| extension == L".JPG"
			|| extension == L".GIF"
			|| extension == L".BMP")
		{
			info.clip_type = L"STILL";

			return true;
		}

		return false;
	});
	dependencies.system_info_provider_repo->register_system_info_provider([](boost::property_tree::wptree& info)
	{
		info.add(L"system.freeimage", version());
	});
}

void uninit()
{
	FreeImage_DeInitialise();
}

}}
