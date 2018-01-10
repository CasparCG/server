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
#include <accelerator/ogl/util/device.h>

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
		std::shared_ptr<accelerator::ogl::device>				ogl_device;
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
				std::shared_ptr<accelerator::ogl::device> ogl_device,
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
			, ogl_device(std::move(ogl_device))
			, shutdown_server_now(shutdown_server_now)
		{
		}
	};

	typedef std::function<std::wstring(command_context& args)> amcp_simple_command_func;
	typedef std::function<void(const std::wstring& res)> amcp_send_response_func;
	typedef std::shared_ptr<amcp_send_response_func> amcp_send_response_func_ptr;
	typedef std::function<std::wstring(command_context& args, amcp_send_response_func_ptr respond)> amcp_command_func;

	class AMCPCommand
	{
	private:
		command_context		ctx_;
		amcp_command_func	command_;
		const int			min_num_params_;
		const std::wstring	name_;
		const std::wstring	request_id_;
	public:
		AMCPCommand(const command_context& ctx, const amcp_command_func& command, const int min_num_params, const std::wstring& name, const std::wstring& request_id)
			: ctx_(ctx)
			, command_(command)
			, min_num_params_(min_num_params)
			, name_(name)
			, request_id_(request_id)
		{
		}

		typedef std::shared_ptr<AMCPCommand> ptr_type;

		bool Execute(const amcp_send_response_func_ptr send)
		{
			(*send)(command_(ctx_, send));
			return true;
		}

		int minimum_parameters() const
		{
			return min_num_params_;
		}

		void SendReply(const std::wstring& str) const
		{
			if (str.empty())
				return;

			std::wstring reply = str;
			if (!request_id_.empty())
				reply = L"RES " + request_id_ + L" " + str;

			ctx_.client->send(std::move(reply));
		}

		std::vector<std::wstring>& parameters() { return ctx_.parameters; }

		IO::ClientInfoPtr client() const { return ctx_.client; }

		std::wstring print() const
		{
			return name_;
		}
	};
}}}
