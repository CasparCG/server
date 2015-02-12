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

#include "../util/protocolstrategy.h"

#include <core/video_channel.h>
#include <core/thumbnail_generator.h>

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
		std::promise<bool>& shutdown_server_now);

	virtual ~AMCPProtocolStrategy();

	virtual void Parse(const std::wstring& msg, IO::ClientInfoPtr pClientInfo);
	virtual std::string GetCodepage() {	return "UTF-8"; }

private:
	struct impl;
	spl::unique_ptr<impl> impl_;
};

}}}
