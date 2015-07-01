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

#include "../util/ClientInfo.h"
#include "amcp_shared.h"
#include <core/consumer/frame_consumer.h>
#include <core/producer/frame_producer.h>

#include <boost/algorithm/string.hpp>

namespace caspar { namespace protocol {
namespace amcp {

	struct command_context
	{
		IO::ClientInfoPtr										client;
		channel_context											channel;
		int														channel_index;
		int														layer_id;
		std::vector<channel_context>							channels;
		spl::shared_ptr<core::help_repository>					help_repo;
		spl::shared_ptr<core::media_info_repository>			media_info_repo;
		spl::shared_ptr<core::cg_producer_registry>				cg_registry;
		spl::shared_ptr<core::system_info_provider_repository>	system_info_repo;
		std::shared_ptr<core::thumbnail_generator>				thumb_gen;
		spl::shared_ptr<const core::frame_producer_registry>	producer_registry;
		spl::shared_ptr<const core::frame_consumer_registry>	consumer_registry;
		std::promise<bool>&										shutdown_server_now;
		std::vector<std::wstring>								parameters;

		int layer_index(int default_ = 0) const { return layer_id == -1 ? default_: layer_id; }

		command_context(
				IO::ClientInfoPtr client,
				channel_context channel,
				int channel_index,
				int layer_id,
				std::vector<channel_context> channels,
				spl::shared_ptr<core::help_repository> help_repo,
				spl::shared_ptr<core::media_info_repository> media_info_repo,
				spl::shared_ptr<core::cg_producer_registry> cg_registry,
				spl::shared_ptr<core::system_info_provider_repository> system_info_repo,
				std::shared_ptr<core::thumbnail_generator> thumb_gen,
				spl::shared_ptr<const core::frame_producer_registry> producer_registry,
				spl::shared_ptr<const core::frame_consumer_registry> consumer_registry,
				std::promise<bool>& shutdown_server_now)
			: client(std::move(client))
			, channel(channel)
			, channel_index(channel_index)
			, layer_id(layer_id)
			, channels(std::move(channels))
			, help_repo(std::move(help_repo))
			, media_info_repo(std::move(media_info_repo))
			, cg_registry(std::move(cg_registry))
			, system_info_repo(std::move(system_info_repo))
			, thumb_gen(std::move(thumb_gen))
			, producer_registry(std::move(producer_registry))
			, consumer_registry(std::move(consumer_registry))
			, shutdown_server_now(shutdown_server_now)
		{
		}
	};

	typedef std::function<std::wstring(command_context& args)> amcp_command_func;

	class AMCPCommand
	{
	private:
		command_context		ctx_;
		amcp_command_func	command_;
		int					min_num_params_;
		std::wstring		name_;
		std::wstring		replyString_;
	public:
		AMCPCommand(const command_context& ctx, const amcp_command_func& command, int min_num_params, const std::wstring& name)
			: ctx_(ctx)
			, command_(command)
			, min_num_params_(min_num_params)
			, name_(name)
		{
		}

		typedef std::shared_ptr<AMCPCommand> ptr_type;

		bool Execute()
		{
			SetReplyString(command_(ctx_));
			return true;
		}

		int minimum_parameters() const
		{
			return min_num_params_;
		}

		void SendReply()
		{
			if (replyString_.empty())
				return;

			ctx_.client->send(std::move(replyString_));
		}

		std::vector<std::wstring>& parameters() { return ctx_.parameters; }

		IO::ClientInfoPtr client() { return ctx_.client; }

		std::wstring print() const
		{
			return name_;
		}

		void SetReplyString(const std::wstring& str)
		{
			replyString_ = str;
		}
	};
}}}
