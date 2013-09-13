/*
* Copyright 2013 Sveriges Television AB http://casparcg.com/
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
* Author: Helge Norberg, helge.norberg@svt.se
*/

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <stack>
#include <map>

#include <boost/thread.hpp>

#include <tbb/concurrent_queue.h>
#include <tbb/concurrent_unordered_map.h>
#include <tbb/atomic.h>

#include <common/utility/assert.h>
//#include <common/memory/page_locked_allocator.h>
#include <common/exception/win32_exception.h>

#include "../interop/DeckLinkAPI_h.h"

namespace caspar { namespace decklink {

class thread_safe_decklink_allocator : public IDeckLinkMemoryAllocator
{
	typedef std::shared_ptr<
			std::vector<uint8_t, tbb::cache_aligned_allocator<uint8_t>>> buffer;

	std::wstring print_;
	tbb::concurrent_unordered_map<size_t, tbb::concurrent_queue<void*>> free_;
	tbb::concurrent_unordered_map<void*, buffer> buffers_;
	tbb::atomic<size_t> total_allocated_;
	tbb::atomic<int64_t> total_calls_;
public:
	thread_safe_decklink_allocator(const std::wstring& print)
		: print_(print)
	{
		total_allocated_ = 0;
		total_calls_ = 0;
	}

	~thread_safe_decklink_allocator()
	{
		CASPAR_LOG(debug)
			<< print_
			<< L" allocated a total of " << total_allocated_ << L" bytes"
			<< L" and was called " << total_calls_ << L" times"
			<< L" during playout";
	}

	virtual HRESULT STDMETHODCALLTYPE AllocateBuffer(
			unsigned long buffer_size, void** allocated_buffer)
	{
		win32_exception::ensure_handler_installed_for_thread(
				"decklink-allocator");

		try
		{
			*allocated_buffer = allocate_buffer(buffer_size);
		}
		catch (const std::bad_alloc&)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();

			return E_OUTOFMEMORY;
		}

		return S_OK;
	}

	void* allocate_buffer(unsigned long buffer_size)
	{
		++total_calls_;
		void* result;

		if (!free_[buffer_size].try_pop(result))
		{
			auto buf = std::make_shared<std::vector<uint8_t,
					tbb::cache_aligned_allocator<uint8_t>>>(buffer_size);
			result = buf->data();
			buffers_.insert(std::make_pair(result, buf));

			total_allocated_ += buffer_size;

			CASPAR_LOG(debug)
					<< print_
					<< L" allocated buffer of size: " << buffer_size
					<< L" for a total of " << total_allocated_;
		}

		return result;
	}

	virtual HRESULT STDMETHODCALLTYPE ReleaseBuffer(void* b)
	{
		win32_exception::ensure_handler_installed_for_thread(
				"decklink-allocator");

		try
		{
			release_buffer(b);
		}
		catch (const std::bad_alloc&)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
		}

		return S_OK;
	}

	void release_buffer(void* b)
	{
		auto buf = buffers_[b];

		CASPAR_VERIFY(buf);

		free_[buf->size()].push(b);
	}

	virtual HRESULT STDMETHODCALLTYPE Commit()
	{
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE Decommit()
	{
		return S_OK;
	}

	STDMETHOD (QueryInterface(REFIID, LPVOID*))	{return E_NOINTERFACE;}
	STDMETHOD_(ULONG, AddRef())					{return 1;}
	STDMETHOD_(ULONG, Release())				{return 1;}
};

}}
