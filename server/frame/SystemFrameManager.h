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
 
#ifndef _CASPAR_SYSTEMFRAMEMANAGER_H__
#define _CASPAR_SYSTEMFRAMEMANAGER_H__

#pragma once

#include "..\utils\Allocator.h"
#include "..\utils\Noncopyable.hpp"

#include "FrameManager.h"

#include <unordered_map>

namespace caspar {

class Frame;
typedef std::tr1::shared_ptr<Frame> FramePtr;

/*
	SystemFrameManager

	Changes:
	2009-06-14 (R.N) : Refactored, note: Is thread-safe since "FixedAllocator" is thread-safe
*/

class SystemFrameManager : public FrameManager, private utils::Noncopyable, private utils::LockableObject
{
public:
	explicit SystemFrameManager(const FrameFormatDescription&);
	virtual ~SystemFrameManager();

	virtual FramePtr CreateFrame();
	virtual const FrameFormatDescription& GetFrameFormatDescription() const;

private:
	class FrameDeallocator;
	const FrameFormatDescription fmtDesc_;
	utils::FixedAllocatorPtr pAllocator_;

	class LockableAllocatorAssoc : public std::tr1::unordered_map<size_t, utils::FixedAllocatorPtr>, public utils::LockableObject{};

	static LockableAllocatorAssoc allocators;
};
typedef std::tr1::shared_ptr<SystemFrameManager> SystemFrameManagerPtr;

}	//namespace caspar
#endif	//_CASPAR_SYSTEMFRAMEMANAGER_H__