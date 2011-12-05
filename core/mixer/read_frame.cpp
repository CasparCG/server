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

#include "../stdafx.h"

#include "read_frame.h"

#include "gpu/fence.h"
#include "gpu/host_buffer.h"	
#include "gpu/ogl_device.h"

#include <tbb/mutex.h>

namespace caspar { namespace core {
																																							
struct read_frame::implementation : boost::noncopyable
{
	safe_ptr<ogl_device>		ogl_;
	size_t						size_;
	safe_ptr<host_buffer>		image_data_;
	tbb::mutex					mutex_;
	audio_buffer				audio_data_;

public:
	implementation(const safe_ptr<ogl_device>& ogl, size_t size, safe_ptr<host_buffer>&& image_data, audio_buffer&& audio_data) 
		: ogl_(ogl)
		, size_(size)
		, image_data_(std::move(image_data))
		, audio_data_(std::move(audio_data)){}	
	
	const boost::iterator_range<const uint8_t*> image_data()
	{
		{
			tbb::mutex::scoped_lock lock(mutex_);

			if(!image_data_->data())
			{
				image_data_.get()->wait(*ogl_);
				ogl_->invoke([=]{image_data_.get()->map();}, high_priority);
			}
		}

		auto ptr = static_cast<const uint8_t*>(image_data_->data());
		return boost::iterator_range<const uint8_t*>(ptr, ptr + image_data_->size());
	}
	const boost::iterator_range<const int32_t*> audio_data()
	{
		return boost::iterator_range<const int32_t*>(audio_data_.data(), audio_data_.data() + audio_data_.size());
	}
};

read_frame::read_frame(const safe_ptr<ogl_device>& ogl, size_t size, safe_ptr<host_buffer>&& image_data, audio_buffer&& audio_data) 
	: impl_(new implementation(ogl, size, std::move(image_data), std::move(audio_data))){}
read_frame::read_frame(){}
const boost::iterator_range<const uint8_t*> read_frame::image_data()
{
	return impl_ ? impl_->image_data() : boost::iterator_range<const uint8_t*>();
}

const boost::iterator_range<const int32_t*> read_frame::audio_data()
{
	return impl_ ? impl_->audio_data() : boost::iterator_range<const int32_t*>();
}

size_t read_frame::image_size() const{return impl_ ? impl_->size_ : 0;}

//#include <tbb/scalable_allocator.h>
//#include <tbb/parallel_for.h>
//#include <tbb/enumerable_thread_specific.h>
//#define		CACHED_BUFFER_SIZE	4096	
//typedef		unsigned int		UINT;
//
//struct cache_buffer
//{
//	cache_buffer() : data(scalable_aligned_malloc(CACHED_BUFFER_SIZE, 64)){}
//	~cache_buffer() {scalable_aligned_free(data);}
//	void* data;
//};
//
//void	CopyFrame( void * pSrc, void * pDest, UINT width, UINT height, UINT pitch );
//
//void* copy_frame(void* dest, const safe_ptr<read_frame>& frame)
//{
//	auto src		= frame->image_data().begin();
//	auto height		= 720;
//	auto width4		= frame->image_data().size()/height;
//
//	CASPAR_ASSERT(frame->image_data().size() % height == 0);
//			
//	tbb::affinity_partitioner ap;
//	tbb::parallel_for(tbb::blocked_range<size_t>(0, height), [&](tbb::blocked_range<size_t>& r)
//	{
//		CopyFrame(const_cast<uint8_t*>(src)+r.begin()*width4, reinterpret_cast<uint8_t*>(dest)+r.begin()*width4, width4, r.size(), width4);
//	}, ap);
//
//	return dest;
//}
//
////  CopyFrame( )
////
////  COPIES VIDEO FRAMES FROM USWC MEMORY TO WB SYSTEM MEMORY VIA CACHED BUFFER
////    ASSUMES PITCH IS A MULTIPLE OF 64B CACHE LINE SIZE, WIDTH MAY NOT BE
//// http://software.intel.com/en-us/articles/copying-accelerated-video-decode-frame-buffers/
//void CopyFrame( void * pSrc, void * pDest, UINT width, UINT height, UINT pitch )
//{
//	tbb::enumerable_thread_specific<cache_buffer> cache_buffers;
//
//	void *		pCacheBlock = cache_buffers.local().data;
//
//	__m128i		x0, x1, x2, x3;
//	__m128i		*pLoad;
//	__m128i		*pStore;
//	__m128i		*pCache;
//	UINT		x, y, yLoad, yStore;
//	UINT		rowsPerBlock;
//	UINT		width64;
//	UINT		extraPitch;	
//
//	rowsPerBlock = CACHED_BUFFER_SIZE / pitch;
//	width64 = (width + 63) & ~0x03f;
//	extraPitch = (pitch - width64) / 16;
//
//	pLoad  = (__m128i *)pSrc;
//	pStore = (__m128i *)pDest;
//
//	//  COPY THROUGH 4KB CACHED BUFFER
//	for( y = 0; y < height; y += rowsPerBlock  )
//	{
//		//  ROWS LEFT TO COPY AT END
//		if( y + rowsPerBlock > height )
//			rowsPerBlock = height - y;
//
//		pCache = (__m128i *)pCacheBlock;
//
//		_mm_mfence();				
//		
//		// LOAD ROWS OF PITCH WIDTH INTO CACHED BLOCK
//		for( yLoad = 0; yLoad < rowsPerBlock; yLoad++ )
//		{
//			// COPY A ROW, CACHE LINE AT A TIME
//			for( x = 0; x < pitch; x +=64 )
//			{
//				x0 = _mm_stream_load_si128( pLoad +0 );
//				x1 = _mm_stream_load_si128( pLoad +1 );
//				x2 = _mm_stream_load_si128( pLoad +2 );
//				x3 = _mm_stream_load_si128( pLoad +3 );
//
//				_mm_store_si128( pCache +0,	x0 );
//				_mm_store_si128( pCache +1, x1 );
//				_mm_store_si128( pCache +2, x2 );
//				_mm_store_si128( pCache +3, x3 );
//
//				pCache += 4;
//				pLoad += 4;
//			}
//		}
//
//		_mm_mfence();
//
//		pCache = (__m128i *)pCacheBlock;
//
//		// STORE ROWS OF FRAME WIDTH FROM CACHED BLOCK
//		for( yStore = 0; yStore < rowsPerBlock; yStore++ )
//		{
//			// copy a row, cache line at a time
//			for( x = 0; x < width64; x +=64 )
//			{
//				x0 = _mm_load_si128( pCache );
//				x1 = _mm_load_si128( pCache +1 );
//				x2 = _mm_load_si128( pCache +2 );
//				x3 = _mm_load_si128( pCache +3 );
//
//				_mm_stream_si128( pStore,	x0 );
//				_mm_stream_si128( pStore +1, x1 );
//				_mm_stream_si128( pStore +2, x2 );
//				_mm_stream_si128( pStore +3, x3 );
//
//				pCache += 4;
//				pStore += 4;
//			}
//
//			pCache += extraPitch;
//			pStore += extraPitch;
//		}
//	}
//}

}}