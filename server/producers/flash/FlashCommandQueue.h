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