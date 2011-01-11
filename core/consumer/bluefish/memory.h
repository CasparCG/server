#pragma once

#include <Windows.h>

#include <BlueVelvet4.h>
#include "../../format/video_format.h"

#include <tbb/mutex.h>

namespace caspar { namespace core { namespace bluefish {
	
static const size_t MAX_HANC_BUFFER_SIZE = 256*1024;
static const size_t MAX_VBI_BUFFER_SIZE = 36*1920*4;

struct page_locked_buffer
{
public:
	page_locked_buffer(size_t size) 
		: size_(size)
		, data_(static_cast<unsigned char*>(::VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE)))
	{
		if(!data_)	
			BOOST_THROW_EXCEPTION(bluefish_exception() << msg_info("Failed to allocate memory for paged locked buffer."));	
		if(::VirtualLock(data_.get(), size_) == 0)	
			BOOST_THROW_EXCEPTION(bluefish_exception() << msg_info("Failed to lock memory for paged locked buffer."));
	}
	
	static void reserve_working_size(size_t size)
	{
		auto lock = get_lock();
		SIZE_T workingSetMinSize = 0, workingSetMaxSize = 0;
		if(::GetProcessWorkingSetSize(::GetCurrentProcess(), &workingSetMinSize, &workingSetMaxSize))
		{
			CASPAR_LOG(debug) << TEXT("WorkingSet size: min = ") << workingSetMinSize << TEXT(", max = ") << workingSetMaxSize;
			
			workingSetMinSize += size;
			workingSetMaxSize += size;

			if(!::SetProcessWorkingSetSize(::GetCurrentProcess(), workingSetMinSize, workingSetMaxSize))		
				BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Failed set workingset."));		
		}
	}

	static void unreserve_working_size(size_t size)
	{
		auto lock = get_lock();
		SIZE_T workingSetMinSize = 0, workingSetMaxSize = 0;
		if(::GetProcessWorkingSetSize(::GetCurrentProcess(), &workingSetMinSize, &workingSetMaxSize))
		{
			CASPAR_LOG(debug) << TEXT("WorkingSet size: min = ") << workingSetMinSize << TEXT(", max = ") << workingSetMaxSize;
			
			workingSetMinSize += static_cast<SIZE_T>(static_cast<int>(workingSetMinSize) - static_cast<int>(size));
			workingSetMaxSize += static_cast<SIZE_T>(static_cast<int>(workingSetMaxSize) - static_cast<int>(size));

			if(!::SetProcessWorkingSetSize(::GetCurrentProcess(), workingSetMinSize, workingSetMaxSize))		
				BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Failed set workingset."));		
		}
	}

	PBYTE data() const { return data_.get(); }
	size_t size() const { return size_; }
private:

	static std::unique_ptr<tbb::mutex::scoped_lock> get_lock()
	{
		static tbb::mutex mutex;
		return std::move(std::unique_ptr<tbb::mutex::scoped_lock>(new tbb::mutex::scoped_lock(mutex)));
	}
	
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

}}}