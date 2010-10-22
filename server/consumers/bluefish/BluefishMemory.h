#pragma once

#include <BlueVelvet4.h>
#include "../../frame/Frame.h"
#include "BluefishException.h"
#include "../../utils/Process.h"

namespace caspar { namespace bluefish {
	
static const size_t MAX_HANC_BUFFER_SIZE = 256*1024;
static const size_t MAX_VBI_BUFFER_SIZE = 36*1920*4;

struct page_locked_buffer
{
public:
	page_locked_buffer(size_t size) : size_(size), data_(static_cast<unsigned char*>(::VirtualAlloc(NULL, size_, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE)))
	{
		if(!data_)	
			throw BluefishException("Failed to allocate memory for paged locked buffer.");	
		if(::VirtualLock(data_.get(), size_) == 0)	
			throw BluefishException("Failed to lock memory for paged locked buffer.");
	}

	static void reserve_working_size(size_t size)
	{
		SIZE_T workingSetMinSize = 0, workingSetMaxSize = 0;
		if(utils::Process::GetCurrentProcess().GetWorkingSetSize(workingSetMinSize, workingSetMaxSize))
		{
			LOG << utils::LogLevel::Debug << TEXT("WorkingSet size: min = ") << workingSetMinSize << TEXT(", max = ") << workingSetMaxSize;
			
			workingSetMinSize += size;
			workingSetMaxSize += size;

			if(!utils::Process::GetCurrentProcess().SetWorkingSetSize(workingSetMinSize, workingSetMaxSize))		
				LOG << utils::LogLevel::Critical << TEXT("Failed to set workingset: min = ") << workingSetMinSize << TEXT(", max = ") << workingSetMaxSize;		
		}
	}

	PBYTE data() const { return data_.get(); }
	size_t size() const { return size_; }
private:

	struct virtual_free
	{
		void operator()(LPVOID lpAddress)
		{
			if(lpAddress != nullptr)		
				try{::VirtualFree(lpAddress, 0, MEM_RELEASE);}catch(...){}		
		}
	};

	size_t size_;
	std::unique_ptr<BYTE, virtual_free> data_;
};
typedef std::shared_ptr<page_locked_buffer> page_locked_buffer_ptr;

struct blue_dma_buffer
{
public:
	blue_dma_buffer(int image_size, int id) : id_(id), image_buffer_(image_size), hanc_buffer_(256*1024){}
			
	int id() const {return id_;}

	PBYTE image_data() const { return image_buffer_.data(); }
	unsigned int* hanc_data() const { return reinterpret_cast<unsigned int*>(hanc_buffer_.data()); }

	size_t image_size() const { return image_buffer_.size(); }
	size_t hanc_size() const { return hanc_buffer_.size(); }

private:	
	int id_;
	page_locked_buffer image_buffer_;
	page_locked_buffer hanc_buffer_;
};
typedef std::shared_ptr<blue_dma_buffer> blue_dma_buffer_ptr;

}}