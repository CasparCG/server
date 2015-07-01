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

#pragma once

#include <common/memory.h>

#include "system_info_provider.h"
#include "producer/cg_proxy.h"
#include "producer/media_info/media_info_repository.h"
#include "producer/frame_producer.h"
#include "consumer/frame_consumer.h"

namespace caspar { namespace core {

struct module_dependencies
{
	const spl::shared_ptr<system_info_provider_repository>	system_info_provider_repo;
	const spl::shared_ptr<cg_producer_registry>				cg_registry;
	const spl::shared_ptr<media_info_repository>			media_info_repo;
	const spl::shared_ptr<frame_producer_registry>			producer_registry;
	const spl::shared_ptr<frame_consumer_registry>			consumer_registry;

	module_dependencies(
			spl::shared_ptr<system_info_provider_repository> system_info_provider_repo,
			spl::shared_ptr<cg_producer_registry> cg_registry,
			spl::shared_ptr<media_info_repository> media_info_repo,
			spl::shared_ptr<frame_producer_registry> producer_registry,
			spl::shared_ptr<frame_consumer_registry> consumer_registry)
		: system_info_provider_repo(std::move(system_info_provider_repo))
		, cg_registry(std::move(cg_registry))
		, media_info_repo(std::move(media_info_repo))
		, producer_registry(std::move(producer_registry))
		, consumer_registry(std::move(consumer_registry))
	{
	}
};

}}
