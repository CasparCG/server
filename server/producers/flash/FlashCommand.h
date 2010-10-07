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
 
#ifndef _FLASHCOMMAND_H_
#define _FLASHCOMMAND_H_

#include "..\..\utils\Event.h"

namespace caspar
{

class FlashProducer;
typedef bool (FlashProducer::*FlashMemberFnPtr)(const tstring&);

class FrameManager;
typedef std::tr1::shared_ptr<FrameManager> FrameManagerPtr;

class FlashCommand
{
protected:
	explicit FlashCommand(FlashProducer* pHost);
	virtual bool DoExecute() = 0;

	FlashProducer* GetHost() {
		return pHost_;
	}

public:
	virtual ~FlashCommand();

	void Execute();
	void Cancel();
	bool Wait(DWORD timeout);

	bool GetResult() {
		return result;
	}

private:
	bool result;

	utils::Event	eventDone_;
	FlashProducer* pHost_;
};

class GenericFlashCommand : public FlashCommand 
{
public:
	GenericFlashCommand(FlashProducer* pHost, FlashMemberFnPtr pFunction);
	GenericFlashCommand(FlashProducer* pHost, FlashMemberFnPtr pFunction, const tstring& parameter);
	virtual ~GenericFlashCommand();

private:
	typedef std::tr1::shared_ptr<FlashCommand> FlashCommandPtr;

	virtual bool DoExecute();
	FlashMemberFnPtr pFunction_;
	tstring parameter_;
};

class InitializeFlashCommand : public FlashCommand
{
public:
	InitializeFlashCommand(FlashProducer* pHost, FrameManagerPtr pFrameManager);
	virtual ~InitializeFlashCommand();

private:
	virtual bool DoExecute();
	FrameManagerPtr pFrameManager_;
};

}

#endif