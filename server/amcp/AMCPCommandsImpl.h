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
namespace amcp {


class LoadCommand : public AMCPCommand
{
public:
	LoadCommand() {}
	virtual bool Execute();
	virtual AMCPCommandCondition CheckConditions();
	virtual bool NeedChannel() {
		return true;
	}
	virtual AMCPCommandScheduling GetDefaultScheduling() {
		return ImmediatelyAndClear;
	}
	virtual int GetMinimumParameters() {
		return 1;
	}
};

class LoadbgCommand : public AMCPCommand
{
public:
	LoadbgCommand() {}
	virtual bool Execute();
	virtual AMCPCommandCondition CheckConditions();

	virtual bool NeedChannel() {
		return true;
	}
	virtual AMCPCommandScheduling GetDefaultScheduling() {
		return AddToQueue;
	}
	virtual int GetMinimumParameters() {
		return 1;
	}
};

class PlayCommand : public AMCPCommand
{
public:
	PlayCommand() {}
	virtual bool Execute();
	virtual AMCPCommandCondition CheckConditions();

	virtual bool NeedChannel() {
		return true;
	}
	virtual AMCPCommandScheduling GetDefaultScheduling() {
		return AddToQueue;
	}
	virtual int GetMinimumParameters() {
		return 0;
	}
};

class StopCommand : public AMCPCommand
{
public:
	StopCommand() {}
	virtual bool Execute();
	virtual AMCPCommandCondition CheckConditions();

	virtual bool NeedChannel() {
		return true;
	}
	virtual AMCPCommandScheduling GetDefaultScheduling() {
		return ImmediatelyAndClear;
	}
	virtual int GetMinimumParameters() {
		return 0;
	}
};

class ClearCommand : public AMCPCommand
{
public:
	ClearCommand() {}
	virtual bool Execute();
	virtual AMCPCommandCondition CheckConditions();

	virtual bool NeedChannel() {
		return true;
	}
	virtual AMCPCommandScheduling GetDefaultScheduling() {
		return ImmediatelyAndClear;
	}
	virtual int GetMinimumParameters() {
		return 0;
	}
};

class ParamCommand : public AMCPCommand
{
public:
	ParamCommand() {}
	virtual bool Execute();
	virtual AMCPCommandCondition CheckConditions();

	virtual bool NeedChannel() {
		return true;
	}
	virtual AMCPCommandScheduling GetDefaultScheduling() {
		return AddToQueue;
	}
	virtual int GetMinimumParameters() {
		return 1;
	}
};

class CGCommand : public AMCPCommand
{
public:
	CGCommand() {}
	virtual bool Execute();
	virtual AMCPCommandCondition CheckConditions();

	virtual bool NeedChannel() {
		return true;
	}
	virtual AMCPCommandScheduling GetDefaultScheduling() {
		return AddToQueue;
	}
	virtual int GetMinimumParameters() {
		return 1;
	}

private:
	bool ValidateLayer(const tstring& layerstring);

	bool ExecuteAdd();
	bool ExecutePlay();
	bool ExecuteStop();
	bool ExecuteNext();
	bool ExecuteRemove();
	bool ExecuteClear();
	bool ExecuteUpdate();
	bool ExecuteInvoke();
	bool ExecuteInfo();
};

class DataCommand : public AMCPCommand
{
public:
	DataCommand() {}
	virtual bool Execute();
	virtual AMCPCommandCondition CheckConditions();

	virtual bool NeedChannel() {
		return false;
	}
	virtual AMCPCommandScheduling GetDefaultScheduling() {
		return AddToQueue;
	}
	virtual int GetMinimumParameters() {
		return 1;
	}

private:
	bool ExecuteStore();
	bool ExecuteRetrieve();
	bool ExecuteList();
};

class ClsCommand : public AMCPCommand
{
public:
	ClsCommand() {}
	virtual bool Execute();
	virtual AMCPCommandCondition CheckConditions();

	virtual bool NeedChannel() {
		return false;
	}
	virtual AMCPCommandScheduling GetDefaultScheduling() {
		return AddToQueue;
	}
	virtual int GetMinimumParameters() {
		return 0;
	}
};

class TlsCommand : public AMCPCommand
{
public:
	TlsCommand() {}
	virtual bool Execute();
	virtual AMCPCommandCondition CheckConditions();

	virtual bool NeedChannel() {
		return false;
	}
	virtual AMCPCommandScheduling GetDefaultScheduling() {
		return AddToQueue;
	}
	virtual int GetMinimumParameters() {
		return 0;
	}

private:
	void FindInDirectory(const tstring& dir, tstringstream&);
};

class CinfCommand : public AMCPCommand
{
public:
	CinfCommand() {}
	virtual bool Execute();
	virtual AMCPCommandCondition CheckConditions();

	virtual bool NeedChannel() {
		return false;
	}
	virtual AMCPCommandScheduling GetDefaultScheduling() {
		return AddToQueue;
	}
	virtual int GetMinimumParameters() {
		return 1;
	}
};

class InfoCommand : public AMCPCommand
{
public:
	InfoCommand() {}
	virtual bool Execute();
	virtual AMCPCommandCondition CheckConditions();

	virtual bool NeedChannel() {
		return false;
	}
	virtual AMCPCommandScheduling GetDefaultScheduling() {
		return AddToQueue;
	}
	virtual int GetMinimumParameters() {
		return 0;
	}

private:
	void GenerateChannelInfo(ChannelPtr&, tstringstream&);
};

class VersionCommand : public AMCPCommand
{
public:
	VersionCommand() {}
	virtual bool Execute();
	virtual AMCPCommandCondition CheckConditions();

	virtual bool NeedChannel() {
		return false;
	}
	virtual AMCPCommandScheduling GetDefaultScheduling() {
		return AddToQueue;
	}
	virtual int GetMinimumParameters() {
		return 0;
	}
};

class ByeCommand : public AMCPCommand
{
public:
	ByeCommand() {}
	virtual bool Execute();
	virtual AMCPCommandCondition CheckConditions();

	virtual bool NeedChannel() {
		return false;
	}
	virtual AMCPCommandScheduling GetDefaultScheduling() {
		return AddToQueue;
	}
	virtual int GetMinimumParameters() {
		return 0;
	}
};

class SetCommand : public AMCPCommand
{
public:
	SetCommand() 
	{}

	virtual bool Execute();
	virtual AMCPCommandCondition CheckConditions();

	virtual bool NeedChannel() {
		return true;
	}
	virtual AMCPCommandScheduling GetDefaultScheduling() {
		return ImmediatelyAndClear;
	}
	virtual int GetMinimumParameters() {
		return 2;
	}
};

#ifdef ENABLE_MONITOR
class MonitorCommand : public AMCPCommand
{
public:
	MonitorCommand() 
	{}

	virtual bool Execute();
	virtual AMCPCommandCondition CheckConditions();

	virtual bool NeedChannel() {
		return true;
	}
	virtual AMCPCommandScheduling GetDefaultScheduling() {
		return AddToQueue;
	}
	virtual int GetMinimumParameters() {
		return 1;
	}
};
#endif
//class KillCommand : public AMCPCommand
//{
//public:
//	KillCommand() {}
//	virtual bool Execute();
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