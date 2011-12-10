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

#include "../utility/assert.h"
#include "../memory/safe_ptr.h"

#include <assert.h>
#include <intrin.h>

namespace caspar {

namespace detail {

struct true_type{};
struct false_type{};

template<typename D_A = false_type, typename S_A = false_type>
struct memcpy_impl
{
	void* operator()(void* dest, const void* source, size_t count)
	{		
		CASPAR_ASSERT(dest != nullptr);
		CASPAR_ASSERT(source != nullptr);
		
		auto dest128   = reinterpret_cast<__m128i*>(dest);
		auto source128 = reinterpret_cast<const __m128i*>(source);
		
		size_t rest = count & 255;
		count &= ~255;

		for(int n = 0; n < count; n += 256)
		{
			auto xmm0  = _mm_loadu_si128(source128++);
			auto xmm1  = _mm_loadu_si128(source128++);
			auto xmm2  = _mm_loadu_si128(source128++);
			auto xmm3  = _mm_loadu_si128(source128++);
			auto xmm4  = _mm_loadu_si128(source128++);
			auto xmm5  = _mm_loadu_si128(source128++);
			auto xmm6  = _mm_loadu_si128(source128++);
			auto xmm7  = _mm_loadu_si128(source128++);
			auto xmm8  = _mm_loadu_si128(source128++);
			auto xmm9  = _mm_loadu_si128(source128++);
			auto xmm10 = _mm_loadu_si128(source128++);
			auto xmm11 = _mm_loadu_si128(source128++);
			auto xmm12 = _mm_loadu_si128(source128++);
			auto xmm13 = _mm_loadu_si128(source128++);
			auto xmm14 = _mm_loadu_si128(source128++);
			auto xmm15 = _mm_loadu_si128(source128++);
			auto xmm16 = _mm_loadu_si128(source128++);

			_mm_storeu_si128(dest128++, xmm0);
			_mm_storeu_si128(dest128++, xmm1);
			_mm_storeu_si128(dest128++, xmm2);
			_mm_storeu_si128(dest128++, xmm3);
			_mm_storeu_si128(dest128++, xmm4);
			_mm_storeu_si128(dest128++, xmm5);
			_mm_storeu_si128(dest128++, xmm6);
			_mm_storeu_si128(dest128++, xmm7);
			_mm_storeu_si128(dest128++, xmm8);
			_mm_storeu_si128(dest128++, xmm9);
			_mm_storeu_si128(dest128++, xmm10);
			_mm_storeu_si128(dest128++, xmm11);
			_mm_storeu_si128(dest128++, xmm12);
			_mm_storeu_si128(dest128++, xmm13);
			_mm_storeu_si128(dest128++, xmm14);
			_mm_storeu_si128(dest128++, xmm15);
			_mm_storeu_si128(dest128++, xmm16);
		}
		
		return memcpy(reinterpret_cast<int8_t*>(dest)+count, reinterpret_cast<const int8_t*>(source)+count, rest);
	}
};

template<>
struct memcpy_impl<true_type, true_type>
{
	void* operator()(void* dest, const void* source, size_t count)
	{
		CASPAR_ASSERT(dest != nullptr);
		CASPAR_ASSERT(source != nullptr);
		CASPAR_ASSERT(reinterpret_cast<int>(dest) % 16 == 0);
		CASPAR_ASSERT(reinterpret_cast<int>(source) % 16 == 0);
		
		auto dest128   = reinterpret_cast<__m128i*>(dest);
		auto source128 = reinterpret_cast<const __m128i*>(source);
		
		size_t rest = count % 256;
		count -= rest;

		for(int n = 0; n < count; n += 256)
		{
			auto xmm0  = _mm_load_si128(source128++);
			auto xmm1  = _mm_load_si128(source128++);
			auto xmm2  = _mm_load_si128(source128++);
			auto xmm3  = _mm_load_si128(source128++);
			auto xmm4  = _mm_load_si128(source128++);
			auto xmm5  = _mm_load_si128(source128++);
			auto xmm6  = _mm_load_si128(source128++);
			auto xmm7  = _mm_load_si128(source128++);
			auto xmm8  = _mm_load_si128(source128++);
			auto xmm9  = _mm_load_si128(source128++);
			auto xmm10 = _mm_load_si128(source128++);
			auto xmm11 = _mm_load_si128(source128++);
			auto xmm12 = _mm_load_si128(source128++);
			auto xmm13 = _mm_load_si128(source128++);
			auto xmm14 = _mm_load_si128(source128++);
			auto xmm15 = _mm_load_si128(source128++);
			auto xmm16 = _mm_load_si128(source128++);

			_mm_stream_si128(dest128++, xmm0);
			_mm_stream_si128(dest128++, xmm1);
			_mm_stream_si128(dest128++, xmm2);
			_mm_stream_si128(dest128++, xmm3);
			_mm_stream_si128(dest128++, xmm4);
			_mm_stream_si128(dest128++, xmm5);
			_mm_stream_si128(dest128++, xmm6);
			_mm_stream_si128(dest128++, xmm7);
			_mm_stream_si128(dest128++, xmm8);
			_mm_stream_si128(dest128++, xmm9);
			_mm_stream_si128(dest128++, xmm10);
			_mm_stream_si128(dest128++, xmm11);
			_mm_stream_si128(dest128++, xmm12);
			_mm_stream_si128(dest128++, xmm13);
			_mm_stream_si128(dest128++, xmm14);
			_mm_stream_si128(dest128++, xmm15);
			_mm_stream_si128(dest128++, xmm16);
		}
		
		return memcpy(reinterpret_cast<int8_t*>(dest)+count, reinterpret_cast<const int8_t*>(source)+count, rest);
	}
};

template<>
struct memcpy_impl<true_type, false_type>
{
	void* operator()(void* dest, const void* source, size_t count)
	{
		CASPAR_ASSERT(dest != nullptr);
		CASPAR_ASSERT(source != nullptr);
		CASPAR_ASSERT(reinterpret_cast<int>(dest) % 16 == 0);
		
		auto dest128   = reinterpret_cast<__m128i*>(dest);
		auto source128 = reinterpret_cast<const __m128i*>(source);
		
		size_t rest = count % 256;
		count -= rest;

		for(int n = 0; n < count; n += 256)
		{
			auto xmm0  = _mm_loadu_si128(source128++);
			auto xmm1  = _mm_loadu_si128(source128++);
			auto xmm2  = _mm_loadu_si128(source128++);
			auto xmm3  = _mm_loadu_si128(source128++);
			auto xmm4  = _mm_loadu_si128(source128++);
			auto xmm5  = _mm_loadu_si128(source128++);
			auto xmm6  = _mm_loadu_si128(source128++);
			auto xmm7  = _mm_loadu_si128(source128++);
			auto xmm8  = _mm_loadu_si128(source128++);
			auto xmm9  = _mm_loadu_si128(source128++);
			auto xmm10 = _mm_loadu_si128(source128++);
			auto xmm11 = _mm_loadu_si128(source128++);
			auto xmm12 = _mm_loadu_si128(source128++);
			auto xmm13 = _mm_loadu_si128(source128++);
			auto xmm14 = _mm_loadu_si128(source128++);
			auto xmm15 = _mm_loadu_si128(source128++);
			auto xmm16 = _mm_loadu_si128(source128++);

			_mm_stream_si128(dest128++, xmm0);
			_mm_stream_si128(dest128++, xmm1);
			_mm_stream_si128(dest128++, xmm2);
			_mm_stream_si128(dest128++, xmm3);
			_mm_stream_si128(dest128++, xmm4);
			_mm_stream_si128(dest128++, xmm5);
			_mm_stream_si128(dest128++, xmm6);
			_mm_stream_si128(dest128++, xmm7);
			_mm_stream_si128(dest128++, xmm8);
			_mm_stream_si128(dest128++, xmm9);
			_mm_stream_si128(dest128++, xmm10);
			_mm_stream_si128(dest128++, xmm11);
			_mm_stream_si128(dest128++, xmm12);
			_mm_stream_si128(dest128++, xmm13);
			_mm_stream_si128(dest128++, xmm14);
			_mm_stream_si128(dest128++, xmm15);
			_mm_stream_si128(dest128++, xmm16);
		}
		
		return memcpy(reinterpret_cast<int8_t*>(dest)+count, reinterpret_cast<const int8_t*>(source)+count, rest);
	}
};

template<>
struct memcpy_impl<false_type, true_type>
{
	void* operator()(void* dest, const void* source, size_t count)
	{
		CASPAR_ASSERT(dest != nullptr);
		CASPAR_ASSERT(source != nullptr);
		CASPAR_ASSERT(reinterpret_cast<int>(source) % 16 == 0);
		
		auto dest128   = reinterpret_cast<__m128i*>(dest);
		auto source128 = reinterpret_cast<const __m128i*>(source);
		
		size_t rest = count % 256;
		count -= rest;

		for(int n = 0; n < count; n += 256)
		{
			auto xmm0  = _mm_load_si128(source128++);
			auto xmm1  = _mm_load_si128(source128++);
			auto xmm2  = _mm_load_si128(source128++);
			auto xmm3  = _mm_load_si128(source128++);
			auto xmm4  = _mm_load_si128(source128++);
			auto xmm5  = _mm_load_si128(source128++);
			auto xmm6  = _mm_load_si128(source128++);
			auto xmm7  = _mm_load_si128(source128++);
			auto xmm8  = _mm_load_si128(source128++);
			auto xmm9  = _mm_load_si128(source128++);
			auto xmm10 = _mm_load_si128(source128++);
			auto xmm11 = _mm_load_si128(source128++);
			auto xmm12 = _mm_load_si128(source128++);
			auto xmm13 = _mm_load_si128(source128++);
			auto xmm14 = _mm_load_si128(source128++);
			auto xmm15 = _mm_load_si128(source128++);
			auto xmm16 = _mm_load_si128(source128++);

			_mm_storeu_si128(dest128++, xmm0);
			_mm_storeu_si128(dest128++, xmm1);
			_mm_storeu_si128(dest128++, xmm2);
			_mm_storeu_si128(dest128++, xmm3);
			_mm_storeu_si128(dest128++, xmm4);
			_mm_storeu_si128(dest128++, xmm5);
			_mm_storeu_si128(dest128++, xmm6);
			_mm_storeu_si128(dest128++, xmm7);
			_mm_storeu_si128(dest128++, xmm8);
			_mm_storeu_si128(dest128++, xmm9);
			_mm_storeu_si128(dest128++, xmm10);
			_mm_storeu_si128(dest128++, xmm11);
			_mm_storeu_si128(dest128++, xmm12);
			_mm_storeu_si128(dest128++, xmm13);
			_mm_storeu_si128(dest128++, xmm14);
			_mm_storeu_si128(dest128++, xmm15);
			_mm_storeu_si128(dest128++, xmm16);
		}

		return memcpy(reinterpret_cast<int8_t*>(dest)+count, reinterpret_cast<const int8_t*>(source)+count, rest);
	}
};

}

template<typename T>
T* fast_memcpy(T* dest, const void* source, size_t count)
{  
	//auto s_align = reinterpret_cast<int>(source) & 15;
	//auto d_align = reinterpret_cast<int>(dest) & 15;
	
	//if(s_align == 0 && s_align == 0)
	//	return reinterpret_cast<T*>(detail::memcpy_impl<detail::true_type, detail::true_type>()(dest, source, count));
	//else if(d_align == 0)
	//	return reinterpret_cast<T*>(detail::memcpy_impl<detail::true_type, detail::false_type>()(dest, source, count));
	//else if(s_align == 0)
	//	return reinterpret_cast<T*>(detail::memcpy_impl<detail::false_type, detail::true_type>()(dest, source, count));

	//return reinterpret_cast<T*>(detail::memcpy_impl<detail::false_type, detail::false_type>()(dest, source, count));
	return reinterpret_cast<T*>(memcpy(dest, source, count));
}

}
