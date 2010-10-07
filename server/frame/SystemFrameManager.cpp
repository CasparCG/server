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
 
#include "..\stdafx.h"
#include "SystemFrameManager.h"

#include "Frame.h"
#include "SystemFrame.h"

namespace caspar {

SystemFrameManager::LockableAllocatorAssoc SystemFrameManager::allocators;

class SystemFrameManager::FrameDeallocator
{
public:
	FrameDeallocator(const utils::FixedAllocatorPtr& pAllocator) : pAllocator_(pAllocator){}
	void operator()(Frame* frame)
	{ 
		pAllocator_->Deallocate(frame->GetDataPtr()); 
		delete frame;
	}
private:
	const utils::FixedAllocatorPtr pAllocator_;
};

SystemFrameManager::SystemFrameManager(const FrameFormatDescription& fmtDesc) : fmtDesc_(fmtDesc)
{
	utils::LockableObject::Lock lock(allocators);
	utils::FixedAllocatorPtr& pAllocator = allocators[fmtDesc_.size];
	if(!pAllocator)
		pAllocator.reset(new utils::FixedAllocator<>(fmtDesc_.size));
	pAllocator_ = pAllocator;	
}

SystemFrameManager::~SystemFrameManager()
{}

FramePtr SystemFrameManager::CreateFrame()
{
	return FramePtr(new SystemFrame(static_cast<unsigned char*>(pAllocator_->Allocate()), fmtDesc_.size, this->ID()), FrameDeallocator(pAllocator_));
}

const FrameFormatDescription& SystemFrameManager::GetFrameFormatDescription() const 
{
	return fmtDesc_;
}



}	//namespace caspar