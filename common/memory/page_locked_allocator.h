/*
* Copyright (c) 2011 Sveriges Television AB <info@casparcg.com>
*
* This file is part of CasparCG (www.casparcg.com).
*
* CasparCG is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* CasparCG is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with CasparCG. If not, see <http://www.gnu.org/licenses/>.
*
* Author: Robert Nagy, ronag89@gmail.com
*/

#pragma once

#include <unordered_map>
#include <tbb/mutex.h>

namespace caspar
{
	
template <class T>
class page_locked_allocator
{
public:
	typedef size_t    size_type;
	typedef ptrdiff_t difference_type;
	typedef T*        pointer;
	typedef const T*  const_pointer;
	typedef T&        reference;
	typedef const T&  const_reference;
	typedef T         value_type;

	page_locked_allocator() {}
	page_locked_allocator(const page_locked_allocator&) {}
  
	pointer allocate(size_type n, const void * = 0) 
	{
		tbb::mutex::scoped_lock lock(get().mutex);

		size_type size = n * sizeof(T);		
		if(get().free < size)
			allocate_store(size);

		auto p = ::VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		if(!p)
			throw std::bad_alloc();

		if(::VirtualLock(p, size) == 0)	
		{
			::VirtualFree(p, 0, MEM_RELEASE);
			throw std::bad_alloc();
		}
		
		get().free -= size;
		get().map[p] = size;
		return reinterpret_cast<T*>(p);
	}
  
	void deallocate(void* p, size_type) 
	{
		tbb::mutex::scoped_lock lock(get().mutex);

		if(!p || get().map.find(p) == get().map.end())
			return;

		try
		{
			::VirtualFree(p, 0, MEM_RELEASE);
			get().free += get().map[p];
			get().map.erase(p);
		}
		catch(...){}		
	}

	pointer           address(reference x) const { return &x; }
	const_pointer     address(const_reference x) const { return &x; }
	page_locked_allocator<T>&  operator=(const page_locked_allocator&) { return *this; }
	void              construct(pointer p, const T& val) { new ((T*) p) T(val); }
	void              destroy(pointer p) { p->~T(); }

	size_type         max_size() const { return size_t(-1); }

	template <class U>
	struct rebind { typedef page_locked_allocator<U> other; };

	template <class U>
	page_locked_allocator(const page_locked_allocator<U>&) {}

	template <class U>
	page_locked_allocator& operator=(const page_locked_allocator<U>&) { return *this; }

private:

	void allocate_store(size_type size)
	{		
		SIZE_T workingSetMinSize = 0, workingSetMaxSize = 0;
		if(::GetProcessWorkingSetSize(::GetCurrentProcess(), &workingSetMinSize, &workingSetMaxSize))
		{			
			workingSetMinSize += size;
			workingSetMaxSize += size;

			if(!::SetProcessWorkingSetSize(::GetCurrentProcess(), workingSetMinSize, workingSetMaxSize))		
				throw std::bad_alloc();		

			get().free += size;
		}
	}

	struct impl
	{
		impl() : free(0){}
		std::unordered_map<void*, size_type> map;
		size_type free;
		tbb::mutex mutex;
	};

	static impl& get()
	{
		static impl i;
		return i;
	}
};
}