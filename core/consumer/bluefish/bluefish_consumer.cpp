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
 
#include "..\..\StdAfx.h"

#ifndef DISABLE_BLUEFISH

#include "fwd.h"
#include "bluefish_consumer.h"
#include "util.h"
#include "exception.h"
#include "memory.h"

#include "../../processor/frame.h"

#include <boost/thread.hpp>

#include <tbb/concurrent_queue.h>

#include <BlueVelvet4.h>
#include <BlueHancUtils.h>

namespace caspar { namespace core { namespace bluefish {
	
struct consumer::implementation
{
	implementation::implementation(const video_format_desc& format_desc, unsigned int device_index, bool embed_audio) 
		: device_index_(device_index), format_desc_(format_desc), sdk_(BlueVelvetFactory4()), current_id_(0), embed_audio_(embed_audio)
	{
		mem_fmt_		= MEM_FMT_ARGB_PC;
		upd_fmt_		= UPD_FMT_FRAME;
		vid_fmt_		= VID_FMT_PAL; 
		res_fmt_		= RES_FMT_NORMAL; 
		engine_mode_	= VIDEO_ENGINE_FRAMESTORE;
				
		if(BLUE_FAIL(sdk_->device_attach(device_index_, FALSE))) 
			BOOST_THROW_EXCEPTION(bluefish_exception() << msg_info("BLUECARD ERROR: Failed to attach device."));
	
		int videoCardType = sdk_->has_video_cardtype();
		CASPAR_LOG(info) << TEXT("BLUECARD INFO: Card type: ") << get_card_desc(videoCardType) << TEXT(". (device ") << device_index_ << TEXT(")");
	
		//void* pBlueDevice = blue_attach_to_device(1);
		//EBlueConnectorPropertySetting video_routing[1];
		//auto channel = BLUE_VIDEO_OUTPUT_CHANNEL_A;
		//video_routing[0].channel = channel;	
		//video_routing[0].propType = BLUE_CONNECTOR_PROP_SINGLE_LINK;
		//video_routing[0].connector = channel == BLUE_VIDEO_OUTPUT_CHANNEL_A ? BLUE_CONNECTOR_SDI_OUTPUT_A : BLUE_CONNECTOR_SDI_OUTPUT_B;
		//blue_set_connector_property(pBlueDevice, 1, video_routing);
		//blue_detach_from_device(&pBlueDevice);
		
		vid_fmt_ = VID_FMT_INVALID;
		auto desiredVideoFormat = vid_fmt_from_video_format(format_desc_.format);
		int videoModeCount = sdk_->count_video_mode();
		for(int videoModeIndex=1; videoModeIndex <= videoModeCount; ++videoModeIndex) 
		{
			EVideoMode videoMode = sdk_->enum_video_mode(videoModeIndex);
			if(videoMode == desiredVideoFormat) 
				vid_fmt_ = videoMode;			
		}
		if(vid_fmt_ == VID_FMT_INVALID)
			BOOST_THROW_EXCEPTION(bluefish_exception() << msg_info("BLUECARD ERROR: Failed to set videomode."));
		
		// Set default video output channel
		//if(BLUE_FAIL(set_card_property(sdk_, DEFAULT_VIDEO_OUTPUT_CHANNEL, channel)))
		//	CASPAR_LOG(error) << TEXT("BLUECARD ERROR: Failed to set default channel. (device ") << device_index_ << TEXT(")");

		//Setting output Video mode
		if(BLUE_FAIL(set_card_property(sdk_, VIDEO_MODE, vid_fmt_))) 
			BOOST_THROW_EXCEPTION(bluefish_exception() << msg_info("BLUECARD ERROR: Failed to set videomode."));

		//Select Update Mode for output
		if(BLUE_FAIL(set_card_property(sdk_, VIDEO_UPDATE_TYPE, upd_fmt_))) 
			BOOST_THROW_EXCEPTION(bluefish_exception() << msg_info("BLUECARD ERROR: Failed to set update type. "));
	
		disable_video_output();

		//Enable dual link output
		if(BLUE_FAIL(set_card_property(sdk_, VIDEO_DUAL_LINK_OUTPUT, 1)))
			BOOST_THROW_EXCEPTION(bluefish_exception() << msg_info("BLUECARD ERROR: Failed to enable dual link."));

		if(BLUE_FAIL(set_card_property(sdk_, VIDEO_DUAL_LINK_OUTPUT_SIGNAL_FORMAT_TYPE, Signal_FormatType_4224)))
			BOOST_THROW_EXCEPTION(bluefish_exception() << msg_info("BLUECARD ERROR: Failed to set dual link format type to 4:2:2:4. (device " + boost::lexical_cast<std::string>(device_index_) + ")"));
			
		//Select output memory format
		if(BLUE_FAIL(set_card_property(sdk_, VIDEO_MEMORY_FORMAT, mem_fmt_))) 
			BOOST_THROW_EXCEPTION(bluefish_exception() << msg_info("BLUECARD ERROR: Failed to set memory format."));
		
		//Select image orientation
		if(BLUE_FAIL(set_card_property(sdk_, VIDEO_IMAGE_ORIENTATION, ImageOrientation_Normal)))
			CASPAR_LOG(error) << TEXT("BLUECARD ERROR: Failed to set image orientation to normal. (device ") << device_index_ << TEXT(")");	

		// Select data range
		if(BLUE_FAIL(set_card_property(sdk_, VIDEO_RGB_DATA_RANGE, CGR_RANGE))) 
			CASPAR_LOG(error) << TEXT("BLUECARD ERROR: Failed to set RGB data range to CGR. (device ") << device_index_ << TEXT(")");	
		
		if(BLUE_FAIL(set_card_property(sdk_, VIDEO_PREDEFINED_COLOR_MATRIX, vid_fmt_ == VID_FMT_PAL ? MATRIX_601_CGR : MATRIX_709_CGR)))
			CASPAR_LOG(error) << TEXT("BLUECARD ERROR: Failed to set colormatrix to ") << (vid_fmt_ == VID_FMT_PAL ? TEXT("601 CGR") : TEXT("709 CGR")) << TEXT(". (device ") << device_index_ << TEXT(")");
		
		//if(BLUE_FAIL(set_card_property(sdk_, EMBEDDED_AUDIO_OUTPUT, 0))) 	
		//	CASPAR_LOG(error) << TEXT("BLUECARD ERROR: Failed to enable embedded audio. (device ") << device_index_ << TEXT(")");	
		//else 
		//{
		//	CASPAR_LOG(info) << TEXT("BLUECARD INFO: Enabled embedded audio. (device ") << device_index_ << TEXT(")");
		//	hasEmbeddedAudio_ = true;
		//}

		CASPAR_LOG(info) << TEXT("BLUECARD INFO: Successfully configured bluecard for ") << format_desc_ << TEXT(". (device ") << device_index_ << TEXT(")");

		if (sdk_->has_output_key()) 
		{
			int dummy = TRUE; int v4444 = FALSE; int invert = FALSE; int white = FALSE;
			sdk_->set_output_key(dummy, v4444, invert, white);
		}

		if(sdk_->GetHDCardType(device_index_) != CRD_HD_INVALID) 
			sdk_->Set_DownConverterSignalType(vid_fmt_ == VID_FMT_PAL ? SD_SDI : HD_SDI);	
	
		if(BLUE_FAIL(sdk_->set_video_engine(engine_mode_)))
			BOOST_THROW_EXCEPTION(bluefish_exception() << msg_info("BLUECARD ERROR: Failed to set vido engine."));

		enable_video_output();
						
		page_locked_buffer::reserve_working_size(MAX_HANC_BUFFER_SIZE * 3);		
		for(int n = 0; n < 3; ++n)
			hanc_buffers_.push_back(std::make_shared<page_locked_buffer>(MAX_HANC_BUFFER_SIZE));

		frame_buffer_.set_capacity(1);
		thread_ = boost::thread([=]{run();});
		
		CASPAR_LOG(info) << TEXT("BLUECARD INFO: Successfully initialized device ") << device_index_;
	}

	~implementation()
	{
		frame_buffer_.push(nullptr),
		thread_.join();

		disable_video_output();

		if(sdk_)
			sdk_->device_detach();		

		CASPAR_LOG(info) << "BLUECARD INFO: Successfully released device " << device_index_;
	}
	
	void enable_video_output()
	{
		if(!BLUE_PASS(set_card_property(sdk_, VIDEO_BLACKGENERATOR, 0)))
			CASPAR_LOG(error) << "BLUECARD ERROR: Failed to disable video output. (device " << device_index_ << TEXT(")");	
	}

	void disable_video_output()
	{
		if(!BLUE_PASS(set_card_property(sdk_, VIDEO_BLACKGENERATOR, 1)))
			CASPAR_LOG(error) << "BLUECARD ERROR: Failed to disable video output. (device " << device_index_ << TEXT(")");		
	}

	void display(const frame_ptr& frame)
	{
		if(frame == nullptr)
			return;

		if(exception_ != nullptr)
			std::rethrow_exception(exception_);

		frame_buffer_.push(frame);
	}

	void do_display(const frame_ptr& frame)
	{
		auto hanc = hanc_buffers_[current_id_];		
		current_id_ = (current_id_+1) % hanc_buffers_.size();		
			
		static size_t audio_samples = 1920;
		static size_t audio_nchannels = 2;
		static std::vector<short> silence(audio_samples*audio_nchannels*2, 0);

		auto& frame_audio_data = frame->get_audio_data().empty() ? silence : frame->get_audio_data();

		unsigned long fieldCount = 0;
		sdk_->wait_output_video_synch(UPD_FMT_FRAME, fieldCount);
				
		if(embed_audio_)
		{		
			encode_hanc(reinterpret_cast<BLUE_UINT32*>(hanc->data()), frame_audio_data.data(), audio_samples, audio_nchannels);

			sdk_->system_buffer_write_async(frame->data().begin(), 
											 frame->data().size(), 
											 nullptr, 
											 BlueImage_HANC_DMABuffer(current_id_, BLUE_DATA_IMAGE));

			sdk_->system_buffer_write_async(hanc->data(),
											 hanc->size(), 
											 nullptr,                 
											 BlueImage_HANC_DMABuffer(current_id_, BLUE_DATA_HANC));

			transferring_frame_ = frame;

			if(BLUE_FAIL(sdk_->render_buffer_update(BlueBuffer_Image_HANC(current_id_))))
				CASPAR_LOG(trace) << TEXT("BLUEFISH: render_buffer_update failed");
		}
		else
		{
			sdk_->system_buffer_write_async(frame->data().begin(),
											 frame->data().size(), 
											 nullptr,                 
											 BlueImage_DMABuffer(current_id_, BLUE_DATA_IMAGE));
			
			if(BLUE_FAIL(sdk_->render_buffer_update(BlueBuffer_Image(current_id_))))
				CASPAR_LOG(trace) << TEXT("BLUEFISH: render_buffer_update failed");
		}
	}


	void encode_hanc(BLUE_UINT32* hanc_data, void* audio_data, size_t audio_samples, size_t audio_nchannels)
	{	
		auto card_type = sdk_->has_video_cardtype();
		auto sample_type = (AUDIO_CHANNEL_16BIT | AUDIO_CHANNEL_LITTLEENDIAN);
		
		hanc_stream_info_struct hanc_stream_info;
		memset(&hanc_stream_info, 0, sizeof(hanc_stream_info));

		hanc_stream_info.AudioDBNArray[0] = -1;
		hanc_stream_info.AudioDBNArray[1] = -1;
		hanc_stream_info.AudioDBNArray[2] = -1;
		hanc_stream_info.AudioDBNArray[3] = -1;
		hanc_stream_info.hanc_data_ptr = hanc_data;
		hanc_stream_info.video_mode = vid_fmt_;
		
		auto emb_audio_flag = (blue_emb_audio_enable | blue_emb_audio_group1_enable);

		if (!is_epoch_card(card_type))
		{
			encode_hanc_frame(&hanc_stream_info, audio_data, audio_nchannels, 
								audio_samples, sample_type, emb_audio_flag);	
		}
		else
		{
			encode_hanc_frame_ex(card_type, &hanc_stream_info, audio_data, audio_nchannels,
								 audio_samples, sample_type, emb_audio_flag);
		}						
	}

	void run()
	{
		while(true)
		{
			try
			{
				frame_ptr frame;
				frame_buffer_.pop(frame);
				if(frame == nullptr)
					return;

				do_display(frame);
			}
			catch(...)
			{
				exception_ = std::current_exception();
			}
		}	
	}
		
	BlueVelvetPtr sdk_;
	
	unsigned int device_index_;
	video_format_desc format_desc_;
	
	std::exception_ptr exception_;
	boost::thread thread_;
	tbb::concurrent_bounded_queue<frame_ptr> frame_buffer_;
	
	unsigned long	mem_fmt_;
	unsigned long	upd_fmt_;
	EVideoMode		vid_fmt_; 
	unsigned long	res_fmt_; 
	unsigned long	engine_mode_;

	frame_ptr transferring_frame_;

	std::vector<page_locked_buffer_ptr> hanc_buffers_;
	int current_id_;
	bool embed_audio_;
};

consumer::consumer(const video_format_desc& format_desc, unsigned int device_index, bool embed_audio) : impl_(new implementation(format_desc, device_index, embed_audio)){}	
void consumer::display(const frame_ptr& frame){impl_->display(frame);}

}}}

#endif