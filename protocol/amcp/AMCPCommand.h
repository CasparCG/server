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

#include "../util/clientinfo.h"
#include "amcp_shared.h"
#include <core/consumer/frame_consumer.h>

#include <boost/algorithm/string.hpp>

namespace caspar { namespace protocol {
namespace amcp {
	
	class AMCPCommand
	{
		AMCPCommand& operator=(const AMCPCommand&);
	protected:
		AMCPCommand(const AMCPCommand& rhs) : client_(rhs.client_), parameters_(rhs.parameters_)
		{}

	public:
		typedef std::shared_ptr<AMCPCommand> ptr_type;

		explicit AMCPCommand(IO::ClientInfoPtr client) : client_(client) {}
		virtual ~AMCPCommand() {}

		virtual bool Execute() = 0;
		virtual int minimum_parameters() = 0;

		void SendReply();

		std::vector<std::wstring>& parameters() { return parameters_; }

		IO::ClientInfoPtr client() { return client_; }

		virtual std::wstring print() const = 0;

		void SetReplyString(const std::wstring& str){replyString_ = str;}

	private:
		std::vector<std::wstring> parameters_;
		IO::ClientInfoPtr client_;
		std::wstring replyString_;
	};

	template<int TMinParameters>
	class AMCPCommandBase : public AMCPCommand
	{
	protected:
		explicit AMCPCommandBase(IO::ClientInfoPtr client) : AMCPCommand(client) {}
		AMCPCommandBase(const AMCPCommandBase& rhs) : AMCPCommand(rhs) {}
		template<int T>
		AMCPCommandBase(const AMCPCommandBase<T>& rhs) : AMCPCommand(rhs) {}

		~AMCPCommandBase(){}
	public:
		virtual bool Execute()
		{
			return (parameters().size() < TMinParameters) ? false : DoExecute();
		}
		virtual int minimum_parameters(){return TMinParameters;}
	private:
		virtual bool DoExecute() = 0;
	};	

	class AMCPChannelCommand
	{
	protected:
		AMCPChannelCommand(const channel_context& ctx, unsigned int channel_index, int layer_index) : ctx_(ctx), channel_index_(channel_index), layer_index_(layer_index)
		{}
		AMCPChannelCommand(const AMCPChannelCommand& rhs) : ctx_(rhs.ctx_), channel_index_(rhs.channel_index_), layer_index_(rhs.layer_index_)
		{}

		spl::shared_ptr<core::video_channel>& channel() { return ctx_.channel; }
		spl::shared_ptr<IO::lock_container>& lock_container() { return ctx_.lock; }

		unsigned int channel_index(){return channel_index_;}
		int layer_index(int default = 0) const{return layer_index_ != -1 ? layer_index_ : default; }

	private:
		unsigned int channel_index_;
		int layer_index_;
		channel_context ctx_;
	};

	class AMCPChannelsAwareCommand
	{
	protected:
		AMCPChannelsAwareCommand(const std::vector<channel_context>& c) : channels_(c) {}
		AMCPChannelsAwareCommand(const AMCPChannelsAwareCommand& rhs) : channels_(rhs.channels_) {}

		const std::vector<channel_context>& channels() { return channels_; }

	private:
		const std::vector<channel_context>& channels_;
	};

	template<int TMinParameters>
	class AMCPChannelCommandBase : public AMCPChannelCommand, public AMCPCommandBase<TMinParameters>
	{
	public:
		AMCPChannelCommandBase(IO::ClientInfoPtr client, const channel_context& channel, unsigned int channel_index, int layer_index) : AMCPChannelCommand(channel, channel_index, layer_index), AMCPCommandBase(client)
		{}
	protected:
		AMCPChannelCommandBase(const AMCPChannelCommandBase& rhs) : AMCPChannelCommand(rhs), AMCPCommandBase(rhs)
		{}
		template<int T>
		AMCPChannelCommandBase(const AMCPChannelCommandBase<T>& rhs) : AMCPChannelCommand(rhs), AMCPCommandBase(rhs)
		{}
	};
}}}
