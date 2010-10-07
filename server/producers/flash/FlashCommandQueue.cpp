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