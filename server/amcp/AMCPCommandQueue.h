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
 
#ifndef _AMCPCOMMANDQUEUE_H__
#define _AMCPCOMMANDQUEUE_H__

#pragma once

#include <list>
#include "..\utils\thread.h"
#include "..\utils\Lockable.h"

#include "AMCPCommand.h"

namespace caspar {
namespace amcp {

class AMCPCommandQueue : public utils::IRunnable, private utils::LockableObject
{
	AMCPCommandQueue(const AMCPCommandQueue&);
	AMCPCommandQueue& operator=(const AMCPCommandQueue&);
public:
	AMCPCommandQueue();
	~AMCPCommandQueue();

	bool Start();
	void Stop();
	void AddCommand(AMCPCommandPtr pCommand);

private:
	utils::Thread				commandPump_;
	virtual void Run(HANDLE stopEvent);
	virtual bool OnUnhandledException(const std::exception& ex) throw();

	utils::Event newCommandEvent_;

	//Needs synro-protection
	std::list<AMCPCommandPtr>	commands_;
};
typedef std::tr1::shared_ptr<AMCPCommandQueue> AMCPCommandQueuePtr;

}	//namespace amcp
}	//namespace caspar

#endif	//_AMCPCOMMANDQUEUE_H__