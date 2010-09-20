/*
* copyright (c) 2010 Sveriges Television AB <info@casparcg.com>
*
*  This file is part of CasparCG.
*
*    CasparCG is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    CasparCG is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.

*    You should have received a copy of the GNU General Public License
*    along with CasparCG.  If not, see <http://www.gnu.org/licenses/>.
*
*/ 
#include "../../stdafx.h"

#include "transition_producer.h"

#include "../../frame/format.h"
#include "../../frame/algorithm.h"

#include "../../../common/image/image.h"
#include "../../frame/system_frame.h"
#include "../../frame/audio_chunk.h"
#include "../../renderer/render_device.h"

#include <boost/range/algorithm/copy.hpp>

namespace caspar{	
	
class empty_producer : public frame_producer
{
public:
	explicit empty_producer(const frame_format_desc& format_desc) 
		: format_desc_(format_desc), frame_(clear_frame(std::make_shared<system_frame>(format_desc_.size)))
	{}	

	frame_ptr get_frame() { return frame_; }
	const frame_format_desc& get_frame_format_desc() const { return format_desc_; }
private:
	frame_format_desc format_desc_;
	frame_ptr frame_;
};

struct transition_producer::implementation : boost::noncopyable
{
	implementation(const frame_producer_ptr& dest, const transition_info& info, const frame_format_desc& format_desc) 
		: current_frame_(0), info_(info), border_color_(0), format_desc_(format_desc), 
			empty_(std::make_shared<empty_producer>(format_desc)), source_(empty_), dest_(dest)
	{
		if(!dest)
			BOOST_THROW_EXCEPTION(null_argument() << arg_name_info("dest"));
	}
		
	frame_producer_ptr get_following_producer() const
	{
		return dest_;
	}
	
	void set_leading_producer(const frame_producer_ptr& producer)
	{
		source_ = producer != nullptr ? producer : empty_;
	}
		
	frame_ptr get_frame()
	{
		if(++current_frame_ >= info_.duration)
			return nullptr;

		return compose(get_producer_frame(dest_), get_producer_frame(source_));
	}

	frame_ptr get_producer_frame(frame_producer_ptr& producer)
	{
		assert(producer != nullptr);

		frame_ptr frame;
		try
		{
			frame = producer->get_frame();
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
			CASPAR_LOG(warning) << "Removed renderer from transition.";
		}

		if(frame == nullptr && producer->get_following_producer() != nullptr)
		{
			auto following = producer->get_following_producer();
			following->set_leading_producer(producer);
			producer = following;
			return get_producer_frame(producer);
		}
		return frame;
	}
			
	frame_ptr compose(const frame_ptr& dest_frame, const frame_ptr& src_frame) 
	{		
		frame_ptr result_frame = dest_frame;		
		if(src_frame != nullptr && dest_frame != nullptr)
		{
			result_frame = std::make_shared<system_frame>(format_desc_.size);
			tbb::parallel_invoke(
			[&]
			{
				GenerateFrame(result_frame->data(), src_frame->data(), dest_frame->data());
			},
			[&]
			{
				float delta = static_cast<float>(current_frame_)/static_cast<float>(info_.duration);
				set_frame_volume(dest_frame, delta*100.0f);
				set_frame_volume(src_frame, (1.0f-delta)*100.0f);		

				boost::range::copy(src_frame->audio_data(), std::back_inserter(result_frame->audio_data()));				
				boost::range::copy(dest_frame->audio_data(), std::back_inserter(result_frame->audio_data()));;
			});
		}
		return result_frame;
	}
	
	void GenerateFrame(unsigned char* pResultData, const unsigned char* pSourceData, const unsigned char* pDestData)
	{
		if(info_.type == transition_type::cut)
		{
			common::image::copy(pResultData, pSourceData, format_desc_.size);
			return;
		}

		if(current_frame_ >= info_.duration) 			
		{
			common::image::copy(pResultData, pDestData, format_desc_.size);
			return;
		}

		if(info_.type == transition_type::mix)
		{
			common::image::lerp(pResultData, pSourceData, pDestData, 1.0f-static_cast<float>(current_frame_)/static_cast<float>(info_.duration), format_desc_.size);
			return;
		}

		size_t totalWidth = format_desc_.width + info_.border_width;
			
		float fStep   = totalWidth / static_cast<float>(info_.duration);
		float fOffset = fStep * static_cast<float>(current_frame_);

		size_t halfStep = static_cast<size_t>(fStep/2.0);
		size_t offset   = static_cast<size_t>(fOffset+0.5f);
			
		//read source to buffer
		for(size_t row = 0, even = 0; row < format_desc_.height; ++row, even ^= 1)
		{
			size_t fieldCorrectedOffset = offset + (halfStep*even);
			if(fieldCorrectedOffset < format_desc_.width)
			{
				if(info_.direction != transition_direction::from_left)
				{
					if(info_.type == transition_type::push)
						memcpy(&(pResultData[4*row*format_desc_.width]), &(pSourceData[4*(row*format_desc_.width+fieldCorrectedOffset)]), (format_desc_.width-fieldCorrectedOffset)*4);
					else	//Slide | Wipe
						memcpy(&(pResultData[4*row*format_desc_.width]), &(pSourceData[4*row*format_desc_.width]), (format_desc_.width-fieldCorrectedOffset)*4);
				}
				else // if (direction == LEFT)
				{				
					if(info_.type == transition_type::push)
						memcpy(&(pResultData[4*(row*format_desc_.width+fieldCorrectedOffset)]), &(pSourceData[4*(row*format_desc_.width)]), (format_desc_.width-fieldCorrectedOffset)*4);
					else	//slide eller wipe
						memcpy(&(pResultData[4*(row*format_desc_.width+fieldCorrectedOffset)]), &(pSourceData[4*(row*format_desc_.width+fieldCorrectedOffset)]), (format_desc_.width-fieldCorrectedOffset)*4);
				}
			}
		}

		//write border to buffer
		if(info_.border_width > 0)
		{
			for(size_t row = 0, even = 0; row < format_desc_.height; ++row, even ^= 1)
			{
				size_t fieldCorrectedOffset = offset + (halfStep*even);
				size_t length = info_.border_width;
				size_t start = 0;

				if(info_.direction != transition_direction::from_left)
				{
					if(fieldCorrectedOffset > format_desc_.width)
					{
						length -= fieldCorrectedOffset-format_desc_.width;
						start += fieldCorrectedOffset-format_desc_.width;
						fieldCorrectedOffset = format_desc_.width;
					}
					else if(fieldCorrectedOffset < length)
					{
						length = fieldCorrectedOffset;
					}

					for(size_t i = 0; i < length; ++i)
						memcpy(&(pResultData[4*(row*format_desc_.width+format_desc_.width-fieldCorrectedOffset+i)]), &border_color_, 4);
						
				}
				else // if (direction == LEFT)
				{
					if(fieldCorrectedOffset > format_desc_.width)
					{
						length -= fieldCorrectedOffset-format_desc_.width;
						start = 0;
						fieldCorrectedOffset -= info_.border_width-length;
					}
					else if(fieldCorrectedOffset < length)
					{
						length = fieldCorrectedOffset;
						start = info_.border_width-fieldCorrectedOffset;
					}

					for(size_t i = 0; i < length; ++i)
						memcpy(&(pResultData[4*(row*format_desc_.width+fieldCorrectedOffset-length+i)]), &border_color_, 4);						
				}

			}
		}

		//read dest to buffer
		offset -= info_.border_width;
		if(offset > 0)
		{
			for(size_t row = 0, even = 0; row < format_desc_.height; ++row, even ^= 1)
			{
				int fieldCorrectedOffset = offset + (halfStep*even);

				if(info_.direction != transition_direction::from_left)
				{
					if(info_.type == transition_type::wipe)
						memcpy(&(pResultData[4*(row*format_desc_.width+format_desc_.width-fieldCorrectedOffset)]), &(pDestData[4*(row*format_desc_.width+format_desc_.width-fieldCorrectedOffset)]), fieldCorrectedOffset*4);
					else
						memcpy(&(pResultData[4*(row*format_desc_.width+format_desc_.width-fieldCorrectedOffset)]), &(pDestData[4*row*format_desc_.width]), fieldCorrectedOffset*4);
				}
				else // if (direction == LEFT)
				{				
					if(info_.type == transition_type::wipe)
						memcpy(&(pResultData[4*(row*format_desc_.width)]), &(pDestData[4*(row*format_desc_.width)]), fieldCorrectedOffset*4);
					else
						memcpy(&(pResultData[4*(row*format_desc_.width)]), &(pDestData[4*(row*format_desc_.width+format_desc_.width-fieldCorrectedOffset)]), fieldCorrectedOffset*4);	
				}
			}
		}
	}

	const frame_format_desc format_desc_;

	frame_producer_ptr		empty_;
	frame_producer_ptr		source_;
	frame_producer_ptr		dest_;
	
	unsigned short			current_frame_;
	
	const transition_info	info_;
	const unsigned long		border_color_;
};

transition_producer::transition_producer(const frame_producer_ptr& dest, const transition_info& info, const frame_format_desc& format_desc) 
	: impl_(new implementation(dest, info, format_desc)){}
frame_ptr transition_producer::get_frame(){return impl_->get_frame();}
frame_producer_ptr transition_producer::get_following_producer() const{return impl_->get_following_producer();}
void transition_producer::set_leading_producer(const frame_producer_ptr& producer) { impl_->set_leading_producer(producer); }
const frame_format_desc& transition_producer::get_frame_format_desc() const { return impl_->format_desc_; } 

}