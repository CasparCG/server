/*
* copyright (c) 2010 Sveriges Television AB <info@casparcg.com>
*
*  This file is part of CasparCG.
*
*    CasparCG is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    CasparCG is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.

*    You should have received a copy of the GNU General Public License
*    along with CasparCG.  If not, see <http://www.gnu.org/licenses/>.
*
*/
 
#ifndef __AMCPCOMMANDSIMPL_H__
#define __AMCPCOMMANDSIMPL_H__

#include "AMCPCommand.h"

namespace caspar {
	
std::wstring ListMedia();
std::wstring ListTemplates();

namespace amcp {

class LoadCommand : public AMCPCommandBase<true, ImmediatelyAndClear, 1>
{
	bool DoExecute();
};

class LoadbgCommand : public AMCPCommandBase<true, AddToQueue, 1>
{
	bool DoExecute();
};

class PlayCommand: public AMCPCommandBase<true, AddToQueue, 0>
{
	bool DoExecute();
};

class PauseCommand: public AMCPCommandBase<true, AddToQueue, 0>
{
	bool DoExecute();
};

class StopCommand : public AMCPCommandBase<true, ImmediatelyAndClear, 0>
{
	bool DoExecute();
};

class ClearCommand : public AMCPCommandBase<true, ImmediatelyAndClear, 0>
{
	bool DoExecute();
};

class CGCommand : public AMCPCommandBase<true, AddToQueue, 1>
{
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
	bool DoExecute();
	bool DoExecuteStore();
	bool DoExecuteRetrieve();
	bool DoExecuteList();
};

class ClsCommand : public AMCPCommandBase<false, AddToQueue, 0>
{
	bool DoExecute();
};

class TlsCommand : public AMCPCommandBase<false, AddToQueue, 0>
{
	bool DoExecute();
};

class CinfCommand : public AMCPCommandBase<false, AddToQueue, 1>
{
	bool DoExecute();
};

class InfoCommand : public AMCPCommandBase<false, AddToQueue, 0>
{
public:
	InfoCommand(const std::vector<renderer::render_device_ptr>& channels) : channels_(channels){}
	bool DoExecute();
private:
	const std::vector<renderer::render_device_ptr>& channels_;
};

class VersionCommand : public AMCPCommandBase<false, AddToQueue, 0>
{
	bool DoExecute();
};

class ByeCommand : public AMCPCommandBase<false, AddToQueue, 0>
{
	bool DoExecute();
};

class SetCommand : public AMCPCommandBase<true, AddToQueue, 2>
{
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
}	//namespace caspar

#endif	//__AMCPCOMMANDSIMPL_H__