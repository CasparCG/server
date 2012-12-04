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

namespace caspar { namespace protocol {
	
std::wstring ListMedia();
std::wstring ListTemplates();

namespace amcp {
	
class ChannelGridCommand : public AMCPCommandBase<false, AddToQueue, 0>
{
	std::wstring print() const { return L"ChannelGridCommand";}
	bool DoExecute();
};

class DiagnosticsCommand : public AMCPCommandBase<false, AddToQueue, 0>
{
	std::wstring print() const { return L"DiagnosticsCommand";}
	bool DoExecute();
};

class CallCommand : public AMCPCommandBase<true, AddToQueue, 1>
{
	std::wstring print() const { return L"CallCommand";}
	bool DoExecute();
};

class MixerCommand : public AMCPCommandBase<true, AddToQueue, 1>
{
	std::wstring print() const { return L"MixerCommand";}
	bool DoExecute();
};
	
class AddCommand : public AMCPCommandBase<true, AddToQueue, 1>
{
	std::wstring print() const { return L"AddCommand";}
	bool DoExecute();
};

class RemoveCommand : public AMCPCommandBase<true, AddToQueue, 0>
{
	std::wstring print() const { return L"RemoveCommand";}
	bool DoExecute();
};

class SwapCommand : public AMCPCommandBase<true, AddToQueue, 1>
{
	std::wstring print() const { return L"SwapCommand";}
	bool DoExecute();
};

class LoadCommand : public AMCPCommandBase<true, AddToQueue, 1>
{
	std::wstring print() const { return L"LoadCommand";}
	bool DoExecute();
};

class LoadbgCommand : public AMCPCommandBase<true, AddToQueue, 1>
{
	std::wstring print() const { return L"LoadbgCommand";}
	bool DoExecute();
};

class PlayCommand: public AMCPCommandBase<true, AddToQueue, 0>
{
	std::wstring print() const { return L"PlayCommand";}
	bool DoExecute();
};

class PauseCommand: public AMCPCommandBase<true, AddToQueue, 0>
{
	std::wstring print() const { return L"PauseCommand";}
	bool DoExecute();
};

class StopCommand : public AMCPCommandBase<true, AddToQueue, 0>
{
	std::wstring print() const { return L"StopCommand";}
	bool DoExecute();
};

class ClearCommand : public AMCPCommandBase<true, ImmediatelyAndClear, 0>
{
	std::wstring print() const { return L"ClearCommand";}
	bool DoExecute();
};

class PrintCommand : public AMCPCommandBase<true, AddToQueue, 0>
{
	std::wstring print() const { return L"PrintCommand";}
	bool DoExecute();
};

class LogCommand : public AMCPCommandBase<false, AddToQueue, 0>
{
	std::wstring print() const { return L"LogCommand";}
	bool DoExecute();
};

class CGCommand : public AMCPCommandBase<true, AddToQueue, 1>
{
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

class DataCommand : public AMCPCommandBase<false, AddToQueue, 1>
{
	std::wstring print() const { return L"DataCommand";}
	bool DoExecute();
	bool DoExecuteStore();
	bool DoExecuteRetrieve();
	bool DoExecuteRemove();
	bool DoExecuteList();
};

class ClsCommand : public AMCPCommandBase<false, AddToQueue, 0>
{
	std::wstring print() const { return L"ClsCommand";}
	bool DoExecute();
};

class TlsCommand : public AMCPCommandBase<false, AddToQueue, 0>
{
	std::wstring print() const { return L"TlsCommand";}
	bool DoExecute();
};

class CinfCommand : public AMCPCommandBase<false, AddToQueue, 1>
{
	std::wstring print() const { return L"CinfCommand";}
	bool DoExecute();
};

class InfoCommand : public AMCPCommandBase<false, AddToQueue, 0>
{
public:
	std::wstring print() const { return L"InfoCommand";}
	InfoCommand(const std::vector<safe_ptr<core::video_channel>>& channels) : channels_(channels){}
	bool DoExecute();
private:
	const std::vector<safe_ptr<core::video_channel>>& channels_;
};

class VersionCommand : public AMCPCommandBase<false, AddToQueue, 0>
{
	std::wstring print() const { return L"VersionCommand";}
	bool DoExecute();
};

class ByeCommand : public AMCPCommandBase<false, AddToQueue, 0>
{
	std::wstring print() const { return L"ByeCommand";}
	bool DoExecute();
};

class SetCommand : public AMCPCommandBase<true, AddToQueue, 2>
{
	std::wstring print() const { return L"SetCommand";}
	bool DoExecute();
};

//class KillCommand : public AMCPCommand
//{
//public:
//	KillCommand() {}
//	virtual bool DoExecute();
//	virtual AMCPCommandCondition CheckConditions();
//
//	virtual bool NeedChannel() {
//		return false;
//	}
//	virtual AMCPCommandScheduling GetDefaultScheduling() {
//		return AddToQueue;
//	}
//	virtual int GetMinimumParameters() {
//		return 0;
//	}
//};

}	//namespace amcp
}}	//namespace caspar

#endif	//__AMCPCOMMANDSIMPL_H__