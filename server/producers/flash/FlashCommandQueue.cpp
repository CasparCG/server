#include "..\..\stdafx.h"

#include "FlashCommandQueue.h"
#include "FlashCommand.h"
#include "FlashManager.h"
#include <algorithm>

namespace caspar {
namespace flash {

FlashCommandQueue::FlashCommandQueue() : newCommandEvent_(TRUE, FALSE) {
}

FlashCommandQueue::~FlashCommandQueue() {
}

void CancelCommand(FlashCommandPtr& pCommand) {
	if(pCommand != 0)
		pCommand->Cancel();
}

void FlashCommandQueue::Clear()
{
	Lock lock(*this);

	std::for_each (commands_.begin(), commands_.end(), CancelCommand);
	commands_.clear();

	newCommandEvent_.Reset();
}

void FlashCommandQueue::Push(FlashCommandPtr pNewCommand)
{
	{
		Lock lock(*this);
		commands_.push_back(pNewCommand);
	}

	newCommandEvent_.Set();
}

FlashCommandPtr FlashCommandQueue::Pop()
{
	Lock lock(*this);

	FlashCommandPtr result;
	if(commands_.size() > 0)
	{
		result = commands_.front();
		commands_.pop_front();

		if(commands_.size() == 0)
			newCommandEvent_.Reset();
	}
	
	return result;
}

}	//namespace flash
}	//namespace caspar