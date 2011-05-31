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