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
* Author: Nicklas P Andersson
*/

#pragma once

#include "../util/ProtocolStrategy.h"

#include <core/video_channel.h>
#include <core/thumbnail_generator.h>
#include <core/producer/media_info/media_info_repository.h>
#include <core/producer/cg_proxy.h>
#include <core/system_info_provider.h>

#include <common/memory.h>

#include <boost/noncopyable.hpp>

#include <string>
#include <future>

namespace caspar { namespace protocol { namespace amcp {

class AMCPProtocolStrategy : public IO::IProtocolStrategy, boost::noncopyable
{
public:
	AMCPProtocolStrategy(
			const std::vector<spl::shared_ptr<core::video_channel>>& channels,
			const std::shared_ptr<core::thumbnail_generator>& thumb_gen,
			const spl::shared_ptr<core::media_info_repository>& media_info_repo,
			const spl::shared_ptr<core::system_info_provider_repository>& system_info_provider_repo,
			const spl::shared_ptr<core::cg_producer_registry>& cg_registry,
			std::promise<bool>& shutdown_server_now);

	virtual ~AMCPProtocolStrategy();

	virtual void Parse(const std::wstring& msg, IO::ClientInfoPtr pClientInfo);
	virtual std::string GetCodepage() const { return "UTF-8"; }

private:
	struct impl;
	spl::unique_ptr<impl> impl_;
};

}}}
