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