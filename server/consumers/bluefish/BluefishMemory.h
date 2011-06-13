#pragma once

#ifndef DISABLE_BLUEFISH

#include <BlueVelvet4.h>
#include "../../frame/Frame.h"
#include "BluefishException.h"
#include "../../utils/Process.h"
#include <unordered_map>

#include "../../utils/Lockable.h"

namespace caspar { namespace bluefish {
	
static const size_t MAX_HANC_BUFFER_SIZE = 256*1024;
static const size_t MAX_VBI_BUFFER_SIZE = 36*1920*4;

class page_locked_allocator
{
	static void reserve(size_t size)
	{
		if(free_ < size)
		{
			size_t required = size - free_;
			SIZE_T workingSetMinSize = 0, workingSetMaxSize = 0;
			if(utils::Process::GetCurrentProcess().GetWorkingSetSize(workingSetMinSize, workingSetMaxSize))
			{
				LOG << utils::LogLevel::Debug << TEXT("WorkingSet size: min = ") << workingSetMinSize << TEXT(", max = ") << workingSetMaxSize;
			
				workingSetMinSize += required;
				workingSetMaxSize += required;

				if(!utils::Process::GetCurrentProcess().SetWorkingSetSize(workingSetMinSize, workingSetMaxSize))		
					LOG << utils::LogLevel::Critical << TEXT("Failed to set workingset: min = ") << workingSetMinSize << TEXT(", max = ") << workingSetMaxSize;		
			}
			free_ += required;
		}
	}
public:

	static void* allocate(size_t size)
	{
		utils::LockableObject::Lock lock(mutex_);

		reserve(size);
		auto ptr = ::VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		
		if(!ptr)	
			throw BluefishException("Failed to allocate memory for paged locked buffer.");	
		
		if(::VirtualLock(ptr, size) == 0)	
		{
			deallocate(ptr);
			throw BluefishException("Failed to lock memory for paged locked buffer.");
		}

		size_map_[ptr] = size;
		free_ -= size;
		return ptr;
	}

	static void deallocate(void* ptr)
	{
		utils::LockableObject::Lock lock(mutex_);

		if(ptr == nullptr)	
			return;

		try{::VirtualFree(ptr, 0, MEM_RELEASE);}catch(...){}	

		auto it = size_map_.find(ptr);
		if(it == size_map_.end())
			return;
		
		free_ += it->second;
		size_map_.erase(it);		
	}

	struct deallocator
	{
		void operator()(void* ptr)
		{
			page_locked_allocator::deallocate(ptr);
		}
	};

private:
	static utils::LockableObject mutex_;
	static std::unordered_map<void*, size_t> size_map_;
	static size_t free_;
};

struct blue_dma_buffer
{
public:
	blue_dma_buffer(int image_size, int id) : id_(id), image_size_(image_size), hanc_size_(256*1024),
		image_buffer_(static_cast<PBYTE>(page_locked_allocator::allocate(image_size_))), hanc_buffer_(static_cast<PBYTE>(page_locked_allocator::allocate(hanc_size_))){}
			
	int id() const {return id_;}

	PBYTE image_data() const { return image_buffer_.get(); }
	PBYTE hanc_data() const { return hanc_buffer_.get(); }

	size_t image_size() const { return image_size_; }
	size_t hanc_size() const { return hanc_size_; }

private:	
	int id_;
	size_t image_size_;
	size_t hanc_size_;
	std::unique_ptr<BYTE, page_locked_allocator::deallocator> image_buffer_;
	std::unique_ptr<BYTE, page_locked_allocator::deallocator> hanc_buffer_;
};
typedef std::shared_ptr<blue_dma_buffer> blue_dma_buffer_ptr;

}}

#endif