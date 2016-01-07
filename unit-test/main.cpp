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

#include "stdafx.h"

#include <gtest/gtest.h>

#include <common/log.h>
#include <common/memory.h>

#include <core/system_info_provider.h>
#include <core/producer/cg_proxy.h>
#include <core/producer/media_info/in_memory_media_info_repository.h>

#include <modules/ffmpeg/ffmpeg.h>

#include <boost/locale.hpp>

int main(int argc, char** argv)
{
	using namespace caspar;

	boost::locale::generator gen;
	gen.categories(boost::locale::codepage_facet);
	std::locale::global(gen(""));

	spl::shared_ptr<core::system_info_provider_repository> system_info_provider_repo;
	spl::shared_ptr<core::cg_producer_registry> cg_registry;
	auto media_info_repo = core::create_in_memory_media_info_repository();
	spl::shared_ptr<core::help_repository> help_repo;
	auto producer_registry = spl::make_shared<core::frame_producer_registry>(help_repo);
	auto consumer_registry = spl::make_shared<core::frame_consumer_registry>(help_repo);

	core::module_dependencies dependencies(system_info_provider_repo, cg_registry, media_info_repo, producer_registry, consumer_registry);
	caspar::ffmpeg::init(dependencies);
	testing::InitGoogleTest(&argc, argv);

	caspar::log::set_log_level(L"trace");

	return RUN_ALL_TESTS();
}
