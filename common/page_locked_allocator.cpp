#include "stdafx.h"

#include "os/windows/windows.h"

#include <unordered_map>
#include <tbb/mutex.h>
#include <tbb/atomic.h>

namespace caspar { namespace detail {
	
tbb::mutex							g_mutex;
std::unordered_map<void*, size_t>	g_map;
size_t								g_free;

void allocate_store(size_t size)
{		
	SIZE_T workingSetMinSize = 0, workingSetMaxSize = 0;
	if(::GetProcessWorkingSetSize(::GetCurrentProcess(), &workingSetMinSize, &workingSetMaxSize))
	{			
		workingSetMinSize += size;
		workingSetMaxSize += size;

		if(!::SetProcessWorkingSetSize(::GetCurrentProcess(), workingSetMinSize, workingSetMaxSize))		
			throw std::bad_alloc();		

		g_free += size;
	}
}

void* alloc_page_locked(size_t size)
{
	tbb::mutex::scoped_lock lock(g_mutex);

	if(g_free < size)
		allocate_store(size);

	auto p = ::VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if(!p)
		throw std::bad_alloc();

	if(::VirtualLock(p, size) == 0)	
	{
		::VirtualFree(p, 0, MEM_RELEASE);
		throw std::bad_alloc();
	}
		
	g_free   -= size;
	g_map[p]	= size;
	return p;

}

void free_page_locked(void* p)
{		
	tbb::mutex::scoped_lock lock(g_mutex);

	if(g_map.find(p) == g_map.end())
		return;

	::VirtualFree(p, 0, MEM_RELEASE);

	g_free += g_map[p];
	g_map.erase(p);
}

}}