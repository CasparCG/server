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

 
#ifndef __AMCPCOMMANDSIMPL_H__
#define __AMCPCOMMANDSIMPL_H__

#include "AMCPCommand.h"

#include <boost/thread/future.hpp>
#include <core/thumbnail_generator.h>

namespace caspar { namespace protocol {
	
std::wstring ListMedia();
std::wstring ListTemplates();

namespace amcp {
	
class ChannelGridCommand : public AMCPCommandBase<0>, AMCPChannelsAwareCommand
{
public:
	ChannelGridCommand(IO::ClientInfoPtr client, const std::vector<channel_context>& channels) : AMCPCommandBase(client), AMCPChannelsAwareCommand(channels) {}
	std::wstring print() const { return L"ChannelGridCommand";}
	bool DoExecute();
};

class DiagnosticsCommand : public AMCPCommandBase<0>
{
public:
	explicit DiagnosticsCommand(IO::ClientInfoPtr client) : AMCPCommandBase(client) {}
	std::wstring print() const { return L"DiagnosticsCommand";}
	bool DoExecute();
};

class CallCommand : public AMCPChannelCommandBase<1>
{
public:
	CallCommand(IO::ClientInfoPtr client, const channel_context& channel, unsigned int channel_index, int layer_index) : AMCPChannelCommandBase(client, channel, channel_index, layer_index)
	{}

private:
	std::wstring print() const { return L"CallCommand";}
	bool DoExecute();
};

class MixerCommand : public AMCPChannelCommandBase<1>
{
public:
	MixerCommand(IO::ClientInfoPtr client, const channel_context& channel, unsigned int channel_index, int layer_index) : AMCPChannelCommandBase(client, channel, channel_index, layer_index)
	{}

private:
	std::wstring print() const { return L"MixerCommand";}
	bool DoExecute();
};
	
class AddCommand : public AMCPChannelCommandBase<1>
{
public:
	AddCommand(IO::ClientInfoPtr client, const channel_context& channel, unsigned int channel_index, int layer_index) : AMCPChannelCommandBase(client, channel, channel_index, layer_index)
	{}

private:
	std::wstring print() const { return L"AddCommand";}
	bool DoExecute();
};

class RemoveCommand : public AMCPChannelCommandBase<0>
{
public:
	RemoveCommand(IO::ClientInfoPtr client, const channel_context& channel, unsigned int channel_index, int layer_index) : AMCPChannelCommandBase(client, channel, channel_index, layer_index)
	{}

private:
	std::wstring print() const { return L"RemoveCommand";}
	bool DoExecute();
};

class SwapCommand : public AMCPChannelCommandBase<1>, AMCPChannelsAwareCommand
{
public:
	SwapCommand(IO::ClientInfoPtr client, const channel_context& channel, unsigned int channel_index, int layer_index, const std::vector<channel_context>& channels) : AMCPChannelCommandBase(client, channel, channel_index, layer_index), AMCPChannelsAwareCommand(channels)
	{}

private:
	std::wstring print() const { return L"SwapCommand";}
	bool DoExecute();
};

class LoadCommand : public AMCPChannelCommandBase<1>
{
public:
	LoadCommand(IO::ClientInfoPtr client, const channel_context& channel, unsigned int channel_index, int layer_index) : AMCPChannelCommandBase(client, channel, channel_index, layer_index)
	{}

private:
	std::wstring print() const { return L"LoadCommand";}
	bool DoExecute();
};


class PlayCommand: public AMCPChannelCommandBase<0>, public AMCPChannelsAwareCommand
{
public:
	PlayCommand(IO::ClientInfoPtr client, const channel_context& channel, unsigned int channel_index, int layer_index, const std::vector<channel_context>& channels) : AMCPChannelCommandBase(client, channel, channel_index, layer_index), AMCPChannelsAwareCommand(channels)
	{}

private:
	std::wstring print() const { return L"PlayCommand";}
	bool DoExecute();
};

class LoadbgCommand : public AMCPChannelCommandBase<1>, public AMCPChannelsAwareCommand
{
public:
	LoadbgCommand(IO::ClientInfoPtr client, const channel_context& channel, unsigned int channel_index, int layer_index, const std::vector<channel_context>& channels) : AMCPChannelCommandBase(client, channel, channel_index, layer_index), AMCPChannelsAwareCommand(channels)
	{}
	LoadbgCommand(const PlayCommand& rhs) : AMCPChannelCommandBase<1>(rhs), AMCPChannelsAwareCommand(rhs) {}

private:
	std::wstring print() const { return L"LoadbgCommand";}
	bool DoExecute();
};

class PauseCommand: public AMCPChannelCommandBase<0>
{
public:
	PauseCommand(IO::ClientInfoPtr client, const channel_context& channel, unsigned int channel_index, int layer_index) : AMCPChannelCommandBase(client, channel, channel_index, layer_index)
	{}

private:
	std::wstring print() const { return L"PauseCommand";}
	bool DoExecute();
};

class StopCommand : public AMCPChannelCommandBase<0>
{
public:
	StopCommand(IO::ClientInfoPtr client, const channel_context& channel, unsigned int channel_index, int layer_index) : AMCPChannelCommandBase(client, channel, channel_index, layer_index)
	{}

private:
	std::wstring print() const { return L"StopCommand";}
	bool DoExecute();
};

class ClearCommand : public AMCPChannelCommandBase<0>
{
public:
	ClearCommand(IO::ClientInfoPtr client, const channel_context& channel, unsigned int channel_index, int layer_index) : AMCPChannelCommandBase(client, channel, channel_index, layer_index)
	{}

private:
	std::wstring print() const { return L"ClearCommand";}
	bool DoExecute();
};

class PrintCommand : public AMCPChannelCommandBase<0>
{
public:
	PrintCommand(IO::ClientInfoPtr client, const channel_context& channel, unsigned int channel_index, int layer_index) : AMCPChannelCommandBase(client, channel, channel_index, layer_index)
	{}

private:
	std::wstring print() const { return L"PrintCommand";}
	bool DoExecute();
};

class LogCommand : public AMCPCommandBase<2>
{
public:
	explicit LogCommand(IO::ClientInfoPtr client) : AMCPCommandBase(client) {}
	std::wstring print() const { return L"LogCommand";}
	bool DoExecute();
};

class CGCommand : public AMCPChannelCommandBase<1>
{
public:
	CGCommand(IO::ClientInfoPtr client, const channel_context& channel, unsigned int channel_index, int layer_index) : AMCPChannelCommandBase(client, channel, channel_index, layer_index)
	{}

private:
	std::wstring print() const { return L"CGCommand";}
	bool DoExecute();
	bool ValidateLayer(const std::wstring& layerstring);

	bool DoExecuteAdd();
	bool DoExecutePlay();
	bool DoExecuteStop();
	bool DoExecuteNext();
	bool DoExecuteRemove();
	bool DoExecuteClear();
	bool DoExecuteUpdate();
	bool DoExecuteInvoke();
	bool DoExecuteInfo();
};

class DataCommand : public AMCPCommandBase<1>
{
public:
	explicit DataCommand(IO::ClientInfoPtr client) : AMCPCommandBase(client) {}

	std::wstring print() const { return L"DataCommand";}
	bool DoExecute();
	bool DoExecuteStore();
	bool DoExecuteRetrieve();
	bool DoExecuteRemove();
	bool DoExecuteList();
};

class ClsCommand : public AMCPCommandBase<0>
{
public:
	explicit ClsCommand(IO::ClientInfoPtr client) : AMCPCommandBase(client) {}
	std::wstring print() const { return L"ClsCommand";}
	bool DoExecute();
};

class TlsCommand : public AMCPCommandBase<0>
{
public:
	explicit TlsCommand(IO::ClientInfoPtr client) : AMCPCommandBase(client) {}
	std::wstring print() const { return L"TlsCommand";}
	bool DoExecute();
};

class CinfCommand : public AMCPCommandBase<1>
{
public:
	explicit CinfCommand(IO::ClientInfoPtr client) : AMCPCommandBase(client) {}
	std::wstring print() const { return L"CinfCommand";}
	bool DoExecute();
};

class InfoCommand : public AMCPCommandBase<0>, AMCPChannelsAwareCommand
{
public:
	InfoCommand(IO::ClientInfoPtr client, const std::vector<channel_context>& channels) : AMCPChannelsAwareCommand(channels), AMCPCommandBase(client) {}
	std::wstring print() const { return L"InfoCommand";}
	bool DoExecute();
};

class VersionCommand : public AMCPCommandBase<0>
{
public:
	explicit VersionCommand(IO::ClientInfoPtr client) : AMCPCommandBase(client) {}
	std::wstring print() const { return L"VersionCommand";}
	bool DoExecute();
};

class ByeCommand : public AMCPCommandBase<0>
{
public:
	explicit ByeCommand(IO::ClientInfoPtr client) : AMCPCommandBase(client) {}
	std::wstring print() const { return L"ByeCommand";}
	bool DoExecute();
};

class SetCommand : public AMCPChannelCommandBase<2>
{
public:
	SetCommand(IO::ClientInfoPtr client, const channel_context& channel, unsigned int channel_index, int layer_index) : AMCPChannelCommandBase(client, channel, channel_index, layer_index)
	{}

	std::wstring print() const { return L"SetCommand";}
	bool DoExecute();
};

class LockCommand : public AMCPCommandBase<1>, AMCPChannelsAwareCommand
{
public:
	LockCommand(IO::ClientInfoPtr client, const std::vector<channel_context>& channels) : AMCPChannelsAwareCommand(channels), AMCPCommandBase(client) {}
	std::wstring print() const { return L"LockCommand";}
	bool DoExecute();
};

class ThumbnailCommand : public AMCPCommandBase<1>
{
public:
	ThumbnailCommand(IO::ClientInfoPtr client, std::shared_ptr<core::thumbnail_generator> thumb_gen) : AMCPCommandBase(client), thumb_gen_(thumb_gen) 
	{}
	std::wstring print() const { return L"ThumbnailCommand";}
	bool DoExecute();

private:
	bool DoExecuteRetrieve();
	bool DoExecuteList();
	bool DoExecuteGenerate();
	bool DoExecuteGenerateAll();

	std::shared_ptr<core::thumbnail_generator> thumb_gen_;
};

class KillCommand : public AMCPCommandBase<0> 
{
public:
	KillCommand(IO::ClientInfoPtr client, boost::promise<bool>& shutdown_server_now) : AMCPCommandBase(client), shutdown_server_now_(&shutdown_server_now) {}
	std::wstring print() const { return L"KillCommand";}
	bool DoExecute();

private:
	boost::promise<bool>* shutdown_server_now_;
};

class RestartCommand : public AMCPCommandBase<0> 
{
public:
	RestartCommand(IO::ClientInfoPtr client, boost::promise<bool>& shutdown_server_now) : AMCPCommandBase(client), shutdown_server_now_(&shutdown_server_now) {}
	std::wstring print() const { return L"RestartCommand";}
	bool DoExecute();

private:
	boost::promise<bool>* shutdown_server_now_;
};

}	//namespace amcp
}}	//namespace caspar

#endif	//__AMCPCOMMANDSIMPL_H__