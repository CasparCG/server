#ifndef _FLASHCOMMANDQUEUE_H__
#define _FLASHCOMMANDQUEUE_H__

#pragma once

#include <list>
#include <string>
#include "..\..\utils\Lockable.h"
#include "..\..\utils\event.h"

namespace caspar {

class FlashCommand;
typedef std::tr1::shared_ptr<FlashCommand> FlashCommandPtr;

namespace flash {

//LoadCommand - creates axControl and loads it with the swf
//StartCommand - Creates the buffers, DC and BITMAP and starts generating frames
//ParamCommand - Sends a parameter to the swf


class FlashCommandQueue : private utils::LockableObject
{
	FlashCommandQueue(const FlashCommandQueue&);
	FlashCommandQueue& operator=(const FlashCommandQueue&);
public:
	FlashCommandQueue();
	~FlashCommandQueue();

	void Push(FlashCommandPtr pCommand);
	FlashCommandPtr Pop();

	void Clear();

	HANDLE GetWaitHandle() {
		return newCommandEvent_;
	}

private:
	utils::Event newCommandEvent_;

	//Needs synro-protection
	std::list<FlashCommandPtr>	commands_;
};

}	//namespace flash
}	//namespace caspar

#endif	//_FLASHCOMMANDQUEUE_H__