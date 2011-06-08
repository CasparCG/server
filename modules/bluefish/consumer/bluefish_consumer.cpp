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
 
#include "../StdAfx.h"

#include "bluefish_consumer.h"
#include "../util/blue_velvet.h"
#include "../util/memory.h"

#include <core/mixer/read_frame.h>

#include <common/concurrency/executor.h>
#include <common/diagnostics/graph.h>
#include <common/memory/memcpy.h>
#include <common/memory/memclr.h>
#include <common/utility/timer.h>

#include <tbb/concurrent_queue.h>

#include <boost/timer.hpp>

#include <memory>
#include <array>

namespace caspar { 
			
struct bluefish_consumer : boost::noncopyable
{
	safe_ptr<CBlueVelvet4>				blue_;
	const unsigned int					device_index_;
	const core::video_format_desc		format_desc_;

	const std::wstring					model_name_;

	std::shared_ptr<diagnostics::graph> graph_;
	boost::timer						frame_timer_;
	boost::timer						tick_timer_;
	boost::timer						sync_timer_;	
		
	const EVideoMode					vid_fmt_; 
	const EMemoryFormat					mem_fmt_;
	const EUpdateMethod					upd_fmt_;
	const EResoFormat					res_fmt_; 
	EEngineMode							engine_mode_;
	
	std::array<blue_dma_buffer_ptr, 4> reserved_frames_;	
	tbb::concurrent_bounded_queue<std::shared_ptr<const core::read_frame>> frame_buffer_;

	int preroll_count_;

	const bool embedded_audio_;
	
	executor executor_;
public:
	bluefish_consumer(const core::video_format_desc& format_desc, unsigned int device_index, bool embedded_audio) 
		: blue_(create_blue())
		, device_index_(device_index)
		, format_desc_(format_desc) 
		, model_name_(get_card_desc(*blue_))
		, vid_fmt_(get_video_mode(*blue_, format_desc)) 
		, mem_fmt_(MEM_FMT_ARGB_PC)
		, upd_fmt_(UPD_FMT_FRAME)
		, res_fmt_(RES_FMT_NORMAL) 
		, engine_mode_(VIDEO_ENGINE_FRAMESTORE)	
		, preroll_count_(0)
		, embedded_audio_(embedded_audio)
		, executor_(print())
	{
		if(BLUE_FAIL(blue_->device_attach(device_index, FALSE))) 
			BOOST_THROW_EXCEPTION(bluefish_exception() << msg_info("Failed to attach device."));

		executor_.set_capacity(CONSUMER_BUFFER_DEPTH);

		graph_ = diagnostics::create_graph(narrow(print()));
		graph_->add_guide("tick-time", 0.5);
		graph_->set_color("tick-time", diagnostics::color(0.1f, 0.7f, 0.8f));
		graph_->add_guide("frame-time", 0.5f);	
		graph_->set_color("frame-time", diagnostics::color(1.0f, 0.0f, 0.0f));
		graph_->set_color("sync-time", diagnostics::color(0.5f, 1.0f, 0.2f));
		graph_->set_color("input-buffer", diagnostics::color(1.0f, 1.0f, 0.0f));
			
		//Setting output Video mode
		if(BLUE_FAIL(set_card_property(blue_, VIDEO_MODE, vid_fmt_))) 
			BOOST_THROW_EXCEPTION(bluefish_exception() << msg_info(narrow(print()) + " Failed to set videomode."));

		//Select Update Mode for output
		if(BLUE_FAIL(set_card_property(blue_, VIDEO_UPDATE_TYPE, upd_fmt_))) 
			BOOST_THROW_EXCEPTION(bluefish_exception() << msg_info(narrow(print()) + " Failed to set update type."));
	
		disable_video_output();

		//Enable dual link output
		if(BLUE_FAIL(set_card_property(blue_, VIDEO_DUAL_LINK_OUTPUT, 1)))
			BOOST_THROW_EXCEPTION(bluefish_exception() << msg_info(narrow(print()) + " Failed to enable dual link."));

		if(BLUE_FAIL(set_card_property(blue_, VIDEO_DUAL_LINK_OUTPUT_SIGNAL_FORMAT_TYPE, Signal_FormatType_4224)))
			BOOST_THROW_EXCEPTION(bluefish_exception() << msg_info(narrow(print()) + " Failed to set dual link format type to 4:2:2:4."));
			
		//Select output memory format
		if(BLUE_FAIL(set_card_property(blue_, VIDEO_MEMORY_FORMAT, mem_fmt_))) 
			BOOST_THROW_EXCEPTION(bluefish_exception() << msg_info(narrow(print()) + " Failed to set memory format."));
		
		//Select image orientation
		if(BLUE_FAIL(set_card_property(blue_, VIDEO_IMAGE_ORIENTATION, ImageOrientation_Normal)))
			CASPAR_LOG(warning) << print() << TEXT(" Failed to set image orientation to normal.");	

		// Select data range
		if(BLUE_FAIL(set_card_property(blue_, VIDEO_RGB_DATA_RANGE, CGR_RANGE))) 
			CASPAR_LOG(warning) << print() << TEXT(" Failed to set RGB data range to CGR.");	
		
		if(BLUE_FAIL(set_card_property(blue_, VIDEO_PREDEFINED_COLOR_MATRIX, vid_fmt_ == VID_FMT_PAL ? MATRIX_601_CGR : MATRIX_709_CGR)))
			CASPAR_LOG(warning) << print() << TEXT(" Failed to set colormatrix to ") << (vid_fmt_ == VID_FMT_PAL ? TEXT("601 CGR") : TEXT("709 CGR")) << TEXT(".");

		if(!embedded_audio_)
		{
			if(BLUE_FAIL(set_card_property(blue_, EMBEDEDDED_AUDIO_OUTPUT, 0))) 
				CASPAR_LOG(warning) << TEXT("BLUECARD ERROR: Failed to disable embedded audio.");			
			CASPAR_LOG(info) << print() << TEXT(" Disabled embedded-audio.");
		}
		else
		{
			if(BLUE_FAIL(set_card_property(blue_, EMBEDEDDED_AUDIO_OUTPUT, blue_emb_audio_enable | blue_emb_audio_group1_enable))) 
				CASPAR_LOG(warning) << print() << TEXT(" Failed to enable embedded audio.");			
			CASPAR_LOG(info) << print() << TEXT(" Enabled embedded-audio.");
		}
		
		if (blue_->has_output_key()) 
		{
			int dummy = TRUE; int v4444 = FALSE; int invert = FALSE; int white = FALSE;
			blue_->set_output_key(dummy, v4444, invert, white);
		}

		if(blue_->GetHDCardType(device_index_) != CRD_HD_INVALID) 
			blue_->Set_DownConverterSignalType(vid_fmt_ == VID_FMT_PAL ? SD_SDI : HD_SDI);	
	
		if(BLUE_FAIL(blue_->set_video_engine(*reinterpret_cast<unsigned long*>(&engine_mode_))))
			BOOST_THROW_EXCEPTION(bluefish_exception() << msg_info(narrow(print()) + " Failed to set video engine."));

		enable_video_output();
						
		int n = 0;
		std::generate(reserved_frames_.begin(), reserved_frames_.end(), [&]
		{
			return std::make_shared<blue_dma_buffer>(format_desc_.size, n++);
		});
								
		CASPAR_LOG(info) << print() << L" Successfully Initialized.";
	}

	~bluefish_consumer()
	{
		executor_.invoke([&]
		{
			disable_video_output();
			blue_->device_detach();		
		});
		
		CASPAR_LOG(info) << print() << L" Shutting down.";	
	}
	
	const core::video_format_desc& get_video_format_desc() const
	{
		return format_desc_;
	}

	void enable_video_output()
	{
		if(!BLUE_PASS(set_card_property(blue_, VIDEO_BLACKGENERATOR, 0)))
			CASPAR_LOG(error) << print() << TEXT(" Failed to disable video output.");	
	}

	void disable_video_output()
	{
		if(!BLUE_PASS(set_card_property(blue_, VIDEO_BLACKGENERATOR, 1)))
			CASPAR_LOG(error)<< print() << TEXT(" Failed to disable video output.");		
	}
	
	void send(const safe_ptr<const core::read_frame>& frame)
	{	
		if(preroll_count_ < executor_.capacity())
		{
			while(preroll_count_++ < executor_.capacity())
				schedule_next_video(make_safe<core::read_frame>());
		}
		
		schedule_next_video(frame);			
	}
	
	void schedule_next_video(const safe_ptr<const core::read_frame>& frame)
	{
		static std::vector<int16_t> silence(MAX_HANC_BUFFER_SIZE, 0);
		
		executor_.begin_invoke([=]
		{
			try
			{
				const size_t audio_samples	 = format_desc_.audio_samples_per_frame;
				const size_t audio_nchannels = format_desc_.audio_channels;

				frame_timer_.restart();
				
				// Copy to local buffers

				if(!frame->image_data().empty())
					fast_memcpy(reserved_frames_.front()->image_data(), frame->image_data().begin(), frame->image_data().size());
				else
					fast_memclr(reserved_frames_.front()->image_data(), reserved_frames_.front()->image_size());

				// Sync

				sync_timer_.restart();
				unsigned long n_field = 0;
				blue_->wait_output_video_synch(UPD_FMT_FRAME, n_field);
				graph_->update_value("sync-time", static_cast<float>(sync_timer_.elapsed()*format_desc_.fps*0.5));

				// Send and display

				if(embedded_audio_)
				{		
					auto frame_audio_data = frame->audio_data().empty() ? silence.data() : const_cast<int16_t*>(frame->audio_data().begin());

					encode_hanc(reinterpret_cast<BLUE_UINT32*>(reserved_frames_.front()->hanc_data()), frame_audio_data, audio_samples, audio_nchannels);
								
					blue_->system_buffer_write_async(const_cast<uint8_t*>(reserved_frames_.front()->image_data()), 
													reserved_frames_.front()->image_size(), 
													nullptr, 
													BlueImage_HANC_DMABuffer(reserved_frames_.front()->id(), BLUE_DATA_IMAGE));

					blue_->system_buffer_write_async(reserved_frames_.front()->hanc_data(),
													reserved_frames_.front()->hanc_size(), 
													nullptr,                 
													BlueImage_HANC_DMABuffer(reserved_frames_.front()->id(), BLUE_DATA_HANC));

					if(BLUE_FAIL(blue_->render_buffer_update(BlueBuffer_Image_HANC(reserved_frames_.front()->id()))))
						CASPAR_LOG(warning) << print() << TEXT(" render_buffer_update failed.");
				}
				else
				{
					blue_->system_buffer_write_async(const_cast<uint8_t*>(reserved_frames_.front()->image_data()),
													reserved_frames_.front()->image_size(), 
													nullptr,                 
													BlueImage_DMABuffer(reserved_frames_.front()->id(), BLUE_DATA_IMAGE));
			
					if(BLUE_FAIL(blue_->render_buffer_update(BlueBuffer_Image(reserved_frames_.front()->id()))))
						CASPAR_LOG(warning) << print() << TEXT(" render_buffer_update failed.");
				}

				std::rotate(reserved_frames_.begin(), reserved_frames_.begin() + 1, reserved_frames_.end());
				
				graph_->update_value("frame-time", static_cast<float>(frame_timer_.elapsed()*format_desc_.fps*0.5));

				graph_->update_value("tick-time", static_cast<float>(tick_timer_.elapsed()*format_desc_.fps*0.5));
				tick_timer_.restart();
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
			}
			graph_->set_value("input-buffer", static_cast<double>(executor_.size())/static_cast<double>(executor_.capacity()));
		});
		graph_->set_value("input-buffer", static_cast<double>(executor_.size())/static_cast<double>(executor_.capacity()));
	}

	void encode_hanc(BLUE_UINT32* hanc_data, void* audio_data, size_t audio_samples, size_t audio_nchannels)
	{	
		static const auto sample_type = AUDIO_CHANNEL_16BIT | AUDIO_CHANNEL_LITTLEENDIAN;
		static const auto emb_audio_flag = blue_emb_audio_enable | blue_emb_audio_group1_enable;
		
		hanc_stream_info_struct hanc_stream_info;
		memset(&hanc_stream_info, 0, sizeof(hanc_stream_info));
		
		hanc_stream_info.AudioDBNArray[0] = -1;
		hanc_stream_info.AudioDBNArray[1] = -1;
		hanc_stream_info.AudioDBNArray[2] = -1;
		hanc_stream_info.AudioDBNArray[3] = -1;
		hanc_stream_info.hanc_data_ptr	  = hanc_data;
		hanc_stream_info.video_mode		  = vid_fmt_;		
		
		if (!is_epoch_card(*blue_))
			encode_hanc_frame(&hanc_stream_info, audio_data, audio_nchannels, audio_samples, sample_type, emb_audio_flag);	
		else
			encode_hanc_frame_ex(blue_->has_video_cardtype(), &hanc_stream_info, audio_data, audio_nchannels, audio_samples, sample_type, emb_audio_flag);
	}
	
	std::wstring print() const
	{
		return model_name_ + L" [" + boost::lexical_cast<std::wstring>(device_index_) + L"|" +  format_desc_.name + L"]";
	}
};

struct bluefish_consumer_proxy : public core::frame_consumer
{
	std::unique_ptr<bluefish_consumer>	consumer_;
	const size_t						device_index_;
	const bool							embedded_audio_;
	const bool							key_only_;
public:

	bluefish_consumer_proxy(size_t device_index, bool embedded_audio, bool key_only)
		: device_index_(device_index)
		, embedded_audio_(embedded_audio)
		, key_only_(key_only){}
	
	virtual void initialize(const core::video_format_desc& format_desc)
	{
		consumer_.reset(new bluefish_consumer(format_desc, device_index_, embedded_audio_));
	}
	
	virtual void send(const safe_ptr<const core::read_frame>& frame)
	{
		consumer_->send(frame);
	}

	virtual const core::video_format_desc& get_video_format_desc() const
	{
		return consumer_->get_video_format_desc();
	}
	
	virtual std::wstring print() const
	{
		return consumer_->print();
	}

	virtual bool key_only() const
	{
		return key_only_;
	}
};	

safe_ptr<core::frame_consumer> create_bluefish_consumer(const std::vector<std::wstring>& params)
{
	if(params.size() < 1 || params[0] != L"BLUEFISH")
		return core::frame_consumer::empty();
		
	const auto device_index = params.size() > 1 ? lexical_cast_or_default<int>(params[1], 1) : 1;

	const auto embedded_audio = std::find(params.begin(), params.end(), L"EMBEDDED_AUDIO") != params.end();
	const auto key_only		  = std::find(params.begin(), params.end(), L"KEY_ONLY")	   != params.end();

	return make_safe<bluefish_consumer_proxy>(device_index, embedded_audio, key_only);
}

safe_ptr<core::frame_consumer> create_bluefish_consumer(const boost::property_tree::ptree& ptree) 
{	
	const auto device_index		= ptree.get("device",		  1);
	const auto embedded_audio	= ptree.get("embedded-audio", false);
	const auto key_only			= ptree.get("key-only",		  false);

	return make_safe<bluefish_consumer_proxy>(device_index, embedded_audio, key_only);
}

}