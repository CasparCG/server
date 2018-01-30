#include "../../stdafx.h"

#include "page_locked_allocator.h"

#include "windows.h"

#include <unordered_map>
#include <mutex>
#include <atomic>

namespace caspar { namespace detail {

std::mutex							g_mutex;
std::unordered_map<void*, size_t>	g_map;
size_t								g_free;

void allocate_store(size_t size)
{
#ifdef WIN32
	SIZE_T workingSetMinSize = 0, workingSetMaxSize = 0;
	if(::GetProcessWorkingSetSize(::GetCurrentProcess(), &workingSetMinSize, &workingSetMaxSize))
	{
		workingSetMinSize += size;
		workingSetMaxSize += size;

		if(!::SetProcessWorkingSetSize(::GetCurrentProcess(), workingSetMinSize, workingSetMaxSize))
			throw std::bad_alloc();

		g_free += size;
	}
#else
    g_free += size;
#endif
}

void* alloc_page_locked(size_t size)
{
	std::lock_guard<std::mutex> lock(g_mutex);

	if(g_free < size)
		allocate_store(size);

#ifdef WIN32
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
#else
    auto p = malloc(size);
#endif
	return p;
}

void free_page_locked(void* p)
{
    std::lock_guard<std::mutex> lock(g_mutex);

	if(g_map.find(p) == g_map.end())
		return;

#ifdef WIN32
	::VirtualFree(p, 0, MEM_RELEASE);
#else
    free(size);
#endif

	g_free += g_map[p];
	g_map.erase(p);
}

}}
