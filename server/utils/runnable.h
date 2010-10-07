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
 
#ifndef _CASPAR_RUNNABLE_H__
#define _CASPAR_RUNNABLE_H__

#pragma once

namespace caspar {
namespace utils {

class IRunnable
{
public:
	virtual ~IRunnable() {}
	virtual void Run(HANDLE stopEvent) = 0;
	virtual bool OnUnhandledException(const std::exception&) throw() = 0;
};

typedef std::tr1::shared_ptr<IRunnable> RunnablePtr;

}	//namespace utils
}	//namespace caspar

#endif	//_CASPAR_RUNNABLE_H__