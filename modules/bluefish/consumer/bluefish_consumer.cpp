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
 
#include "../StdAfx.h"

#include "bluefish_consumer.h"
#include "../util/blue_velvet.h"
#include "../util/memory.h"

#include <core/video_format.h>
#include <core/frame/frame.h>
#include <core/frame/audio_channel_layout.h>
#include <core/help/help_repository.h>
#include <core/help/help_sink.h>

#include <common/executor.h>
#include <common/diagnostics/graph.h>
#include <common/array.h>
#include <common/memshfl.h>
#include <common/param.h>

#include <core/consumer/frame_consumer.h>
#include <core/mixer/audio/audio_util.h>

#include <tbb/concurrent_queue.h>
#include <tbb/atomic.h>

#include <common/assert.h>
#include <boost/lexical_cast.hpp>
#include <boost/timer.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/algorithm/string.hpp>

#include <asmlib.h>

#include <memory>
#include <array>

namespace caspar { namespace bluefish { 
			
struct bluefish_consumer : boost::noncopyable
{
	spl::shared_ptr<CBlueVelvet4>						blue_;
	const unsigned int									device_index_;
	const core::video_format_desc						format_desc_;
	const core::audio_channel_layout					channel_layout_;
	core::audio_channel_remapper						channel_remapper_;
	const int											channel_index_;

	const std::wstring									model_name_;

	spl::shared_ptr<diagnostics::graph>					graph_;
	boost::timer										frame_timer_;
	boost::timer										tick_timer_;
	boost::timer										sync_timer_;	
			
	unsigned int										vid_fmt_;

	std::array<blue_dma_buffer_ptr, 4>					reserved_frames_;	
	tbb::concurrent_bounded_queue<core::const_frame>	frame_buffer_;
	tbb::atomic<int64_t>								presentation_delay_millis_;
	core::const_frame									previous_frame_				= core::const_frame::empty();

	const bool											embedded_audio_;
	const bool											key_only_;
		
	executor											executor_;
public:
	bluefish_consumer(
			const core::video_format_desc& format_desc,
			const core::audio_channel_layout& in_channel_layout,
			const core::audio_channel_layout& out_channel_layout,
			int device_index,
			bool embedded_audio,
			bool key_only,
			int channel_index)
		: blue_(create_blue(device_index))
		, device_index_(device_index)
		, format_desc_(format_desc)
		, channel_layout_(out_channel_layout)
		, channel_remapper_(in_channel_layout, out_channel_layout)
		, channel_index_(channel_index)
		, model_name_(get_card_desc(*blue_))
		, vid_fmt_(get_video_mode(*blue_, format_desc))
		, embedded_audio_(embedded_audio)
		, key_only_(key_only)
		, executor_(print())
	{
		executor_.set_capacity(1);
		presentation_delay_millis_ = 0;

		graph_->set_color("tick-time", diagnostics::color(0.0f, 0.6f, 0.9f));	
		graph_->set_color("sync-time", diagnostics::color(1.0f, 0.0f, 0.0f));
		graph_->set_color("frame-time", diagnostics::color(0.5f, 1.0f, 0.2f));
		graph_->set_text(print());
		diagnostics::register_graph(graph_);
			
		//Setting output Video mode
		if(BLUE_FAIL(set_card_property(blue_, VIDEO_MODE, vid_fmt_))) 
			CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(print() + L" Failed to set videomode."));

		//Select Update Mode for output
		if(BLUE_FAIL(set_card_property(blue_, VIDEO_UPDATE_TYPE, UPD_FMT_FRAME))) 
			CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(print() + L" Failed to set update type."));
	
		disable_video_output();

		//Enable dual link output
		if(BLUE_FAIL(set_card_property(blue_, VIDEO_DUAL_LINK_OUTPUT, 1)))
			CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(print() + L" Failed to enable dual link."));

		if(BLUE_FAIL(set_card_property(blue_, VIDEO_DUAL_LINK_OUTPUT_SIGNAL_FORMAT_TYPE, Signal_FormatType_4224)))
			CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(print() + L" Failed to set dual link format type to 4:2:2:4."));
			
		//Select output memory format
		if(BLUE_FAIL(set_card_property(blue_, VIDEO_MEMORY_FORMAT, MEM_FMT_ARGB_PC))) 
			CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(print() + L" Failed to set memory format."));
		
		//Select image orientation
		if(BLUE_FAIL(set_card_property(blue_, VIDEO_IMAGE_ORIENTATION, ImageOrientation_Normal)))
			CASPAR_LOG(warning) << print() << L" Failed to set image orientation to normal.";	

		// Select data range
		if(BLUE_FAIL(set_card_property(blue_, VIDEO_RGB_DATA_RANGE, CGR_RANGE))) 
			CASPAR_LOG(warning) << print() << L" Failed to set RGB data range to CGR.";	
		
		if(BLUE_FAIL(set_card_property(blue_, VIDEO_PREDEFINED_COLOR_MATRIX, vid_fmt_ == VID_FMT_PAL ? MATRIX_601_CGR : MATRIX_709_CGR)))
			CASPAR_LOG(warning) << print() << L" Failed to set colormatrix to " << (vid_fmt_ == VID_FMT_PAL ? L"601 CGR" : L"709 CGR") << L".";

		if(!embedded_audio_)
		{
			if(BLUE_FAIL(set_card_property(blue_, EMBEDEDDED_AUDIO_OUTPUT, 0))) 
				CASPAR_LOG(warning) << TEXT("BLUECARD ERROR: Failed to disable embedded audio.");			
			CASPAR_LOG(info) << print() << TEXT(" Disabled embedded-audio.");
		}
		else
		{
			ULONG audio_value =
				EMBEDDED_AUDIO_OUTPUT | blue_emb_audio_group1_enable;

			if (channel_layout_.num_channels > 4)
				audio_value |= blue_emb_audio_group2_enable;

			if (channel_layout_.num_channels > 8)
				audio_value |= blue_emb_audio_group3_enable;

			if (channel_layout_.num_channels > 12)
				audio_value |= blue_emb_audio_group4_enable;

			if(BLUE_FAIL(set_card_property(blue_, EMBEDEDDED_AUDIO_OUTPUT, audio_value)))
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
	
		if(BLUE_FAIL(set_card_property(blue_, VIDEO_OUTPUT_ENGINE, VIDEO_ENGINE_FRAMESTORE))) 
			CASPAR_LOG(warning) << print() << TEXT(" Failed to set video engine.");	
		
		enable_video_output();
						
		int n = 0;
		boost::range::generate(reserved_frames_, [&]{return std::make_shared<blue_dma_buffer>(static_cast<int>(format_desc_.size), n++);});
	}

	~bluefish_consumer()
	{
		try
		{
			executor_.invoke([&]
			{
				disable_video_output();
				blue_->device_detach();		
			});
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
		}
	}
	
	void enable_video_output()
	{
		if(!BLUE_PASS(set_card_property(blue_, VIDEO_BLACKGENERATOR, 0)))
			CASPAR_LOG(error) << print() << TEXT(" Failed to disable video output.");	
	}

	void disable_video_output()
	{
		blue_->video_playback_stop(0,0);
		if(!BLUE_PASS(set_card_property(blue_, VIDEO_BLACKGENERATOR, 1)))
			CASPAR_LOG(error)<< print() << TEXT(" Failed to disable video output.");		
	}
	
	std::future<bool> send(core::const_frame& frame)
	{					
		return executor_.begin_invoke([=]() -> bool
		{
			try
			{	
				display_frame(frame);				
				graph_->set_value("tick-time", static_cast<float>(tick_timer_.elapsed()*format_desc_.fps*0.5));
				tick_timer_.restart();
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
			}

			return true;
		});
	}

	void display_frame(core::const_frame frame)
	{
		// Sync

		sync_timer_.restart();
		unsigned long n_field = 0;
		blue_->wait_output_video_synch(UPD_FMT_FRAME, n_field);
		graph_->set_value("sync-time", sync_timer_.elapsed()*format_desc_.fps*0.5);
		
		frame_timer_.restart();		

		if (previous_frame_ != core::const_frame::empty())
			presentation_delay_millis_ = previous_frame_.get_age_millis();

		previous_frame_ = frame;

		// Copy to local buffers
		
		if(!frame.image_data().empty())
		{
			if(key_only_)						
				aligned_memshfl(reserved_frames_.front()->image_data(), frame.image_data().begin(), frame.image_data().size(), 0x0F0F0F0F, 0x0B0B0B0B, 0x07070707, 0x03030303);
			else
				A_memcpy(reserved_frames_.front()->image_data(), frame.image_data().begin(), frame.image_data().size());
		}
		else
			A_memset(reserved_frames_.front()->image_data(), 0, reserved_frames_.front()->image_size());
								

		// Send and display

		if(embedded_audio_)
		{
			auto remapped_audio	= channel_remapper_.mix_and_rearrange(frame.audio_data());
			auto frame_audio	= core::audio_32_to_24(remapped_audio);
			encode_hanc(reinterpret_cast<BLUE_UINT32*>(reserved_frames_.front()->hanc_data()), 
						frame_audio.data(), 
						static_cast<int>(frame.audio_data().size()/channel_layout_.num_channels), 
						static_cast<int>(channel_layout_.num_channels));
								
			blue_->system_buffer_write_async(const_cast<uint8_t*>(reserved_frames_.front()->image_data()), 
											static_cast<unsigned long>(reserved_frames_.front()->image_size()), 
											nullptr, 
											BlueImage_HANC_DMABuffer(reserved_frames_.front()->id(), BLUE_DATA_IMAGE));

			blue_->system_buffer_write_async(reserved_frames_.front()->hanc_data(),
											static_cast<unsigned long>(reserved_frames_.front()->hanc_size()), 
											nullptr,                 
											BlueImage_HANC_DMABuffer(reserved_frames_.front()->id(), BLUE_DATA_HANC));

			if(BLUE_FAIL(blue_->render_buffer_update(BlueBuffer_Image_HANC(reserved_frames_.front()->id()))))
				CASPAR_LOG(warning) << print() << TEXT(" render_buffer_update failed.");
		}
		else
		{
			blue_->system_buffer_write_async(const_cast<uint8_t*>(reserved_frames_.front()->image_data()),
											static_cast<unsigned long>(reserved_frames_.front()->image_size()), 
											nullptr,                 
											BlueImage_DMABuffer(reserved_frames_.front()->id(), BLUE_DATA_IMAGE));
			
			if(BLUE_FAIL(blue_->render_buffer_update(BlueBuffer_Image(reserved_frames_.front()->id()))))
				CASPAR_LOG(warning) << print() << TEXT(" render_buffer_update failed.");
		}

		boost::range::rotate(reserved_frames_, std::begin(reserved_frames_)+1);
		
		graph_->set_value("frame-time", static_cast<float>(frame_timer_.elapsed()*format_desc_.fps*0.5));
	}

	void encode_hanc(BLUE_UINT32* hanc_data, void* audio_data, int audio_samples, int audio_nchannels)
	{	
		const auto sample_type = AUDIO_CHANNEL_24BIT | AUDIO_CHANNEL_LITTLEENDIAN;
		auto emb_audio_flag = blue_emb_audio_enable | blue_emb_audio_group1_enable;

		if (audio_nchannels > 4)
			emb_audio_flag |= blue_emb_audio_group2_enable;

		if (audio_nchannels > 8)
			emb_audio_flag |= blue_emb_audio_group3_enable;

		if (audio_nchannels > 12)
			emb_audio_flag |= blue_emb_audio_group4_enable;
		
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
		return model_name_ + L" [" + boost::lexical_cast<std::wstring>(channel_index_) + L"-" + 
			boost::lexical_cast<std::wstring>(device_index_) + L"|" +  format_desc_.name + L"]";
	}

	int64_t presentation_delay_millis() const
	{
		return presentation_delay_millis_;
	}
};

struct bluefish_consumer_proxy : public core::frame_consumer
{
	core::monitor::subject				monitor_subject_;

	std::unique_ptr<bluefish_consumer>	consumer_;
	const int							device_index_;
	const bool							embedded_audio_;
	const bool							key_only_;

	std::vector<int>					audio_cadence_;
	core::video_format_desc				format_desc_;
	core::audio_channel_layout			in_channel_layout_		= core::audio_channel_layout::invalid();
	core::audio_channel_layout			out_channel_layout_;

public:

	bluefish_consumer_proxy(int device_index, bool embedded_audio, bool key_only, const core::audio_channel_layout& out_channel_layout)
		: device_index_(device_index)
		, embedded_audio_(embedded_audio)
		, key_only_(key_only)
		, out_channel_layout_(out_channel_layout)
	{
	}
	
	// frame_consumer
	
	void initialize(const core::video_format_desc& format_desc, const core::audio_channel_layout& channel_layout, int channel_index) override
	{
		format_desc_		= format_desc;
		in_channel_layout_	= channel_layout;
		audio_cadence_		= format_desc.audio_cadence;

		if (out_channel_layout_ == core::audio_channel_layout::invalid())
			out_channel_layout_ = in_channel_layout_;

		consumer_.reset();
		consumer_.reset(new bluefish_consumer(format_desc, in_channel_layout_, out_channel_layout_, device_index_, embedded_audio_, key_only_, channel_index));
	}
	
	std::future<bool> send(core::const_frame frame) override
	{
		CASPAR_VERIFY(audio_cadence_.front() * in_channel_layout_.num_channels == static_cast<size_t>(frame.audio_data().size()));
		boost::range::rotate(audio_cadence_, std::begin(audio_cadence_)+1);
		return consumer_->send(frame);
	}
		
	std::wstring print() const override
	{
		return consumer_ ? consumer_->print() : L"[bluefish_consumer]";
	}

	std::wstring name() const override
	{
		return L"bluefish";
	}

	boost::property_tree::wptree info() const override
	{
		boost::property_tree::wptree info;
		info.add(L"type", L"bluefish");
		info.add(L"key-only", key_only_);
		info.add(L"device", device_index_);
		info.add(L"embedded-audio", embedded_audio_);
		info.add(L"presentation-frame-age", presentation_frame_age_millis());
		return info;
	}

	int buffer_depth() const override
	{
		return 1;
	}
	
	int index() const override
	{
		return 400 + device_index_;
	}

	int64_t presentation_frame_age_millis() const override
	{
		return consumer_ ? consumer_->presentation_delay_millis() : 0;
	}

	core::monitor::subject& monitor_output()
	{
		return monitor_subject_;
	}
};	


void describe_consumer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Sends video on an SDI output using Bluefish video cards.");
	sink.syntax(L"BLUEFISH {[device_index:int]|1} {[embedded_audio:EMBEDDED_AUDIO]} {[key_only:KEY_ONLY]} {CHANNEL_LAYOUT [channel_layout:string]}");
	sink.para()
		->text(L"Sends video on an SDI output using Bluefish video cards. Multiple video cards can be ")
		->text(L"installed in the same machine and used at the same time, they will be addressed via ")
		->text(L"different ")->code(L"device_index")->text(L" parameters.");
	sink.para()->text(L"Specify ")->code(L"embedded_audio")->text(L" to embed audio into the SDI signal.");
	sink.para()
		->text(L"Specifying ")->code(L"key_only")->text(L" will extract only the alpha channel from the ")
		->text(L"channel. This is useful when you have two SDI video cards, and neither has native support ")
		->text(L"for separate fill/key output");
	sink.para()->text(L"Specify ")->code(L"channel_layout")->text(L" to output a different audio channel layout than the channel uses.");
	sink.para()->text(L"Examples:");
	sink.example(L">> ADD 1 BLUEFISH", L"uses the default device_index of 1.");
	sink.example(L">> ADD 1 BLUEFISH 2", L"for device_index 2.");
	sink.example(
		L">> ADD 1 BLUEFISH 1 EMBEDDED_AUDIO\n"
		L">> ADD 1 BLUEFISH 2 KEY_ONLY", L"uses device with index 1 as fill output with audio and device with index 2 as key output.");
}

spl::shared_ptr<core::frame_consumer> create_consumer(
		const std::vector<std::wstring>& params, core::interaction_sink*)
{
	if(params.size() < 1 || !boost::iequals(params.at(0), L"BLUEFISH"))
		return core::frame_consumer::empty();

	const auto device_index = params.size() > 1 ? boost::lexical_cast<int>(params.at(1)) : 1;

	const auto embedded_audio	= contains_param(	L"EMBEDDED_AUDIO",	params);
	const auto key_only			= contains_param(	L"KEY_ONLY",		params);
	const auto channel_layout	= get_param(		L"CHANNEL_LAYOUT",	params);

	auto layout = core::audio_channel_layout::invalid();

	if (!channel_layout.empty())
	{
		auto found_layout = core::audio_channel_layout_repository::get_default()->get_layout(channel_layout);

		if (!found_layout)
			CASPAR_THROW_EXCEPTION(user_error() << msg_info(L"Channel layout " + channel_layout + L" not found"));

		layout = *found_layout;
	}

	return spl::make_shared<bluefish_consumer_proxy>(device_index, embedded_audio, key_only, layout);
}

spl::shared_ptr<core::frame_consumer> create_preconfigured_consumer(
		const boost::property_tree::wptree& ptree, core::interaction_sink*)
{	
	const auto device_index		= ptree.get(						L"device",			1);
	const auto embedded_audio	= ptree.get(						L"embedded-audio",	false);
	const auto key_only			= ptree.get(						L"key-only",		false);
	const auto channel_layout	= ptree.get_optional<std::wstring>(	L"channel-layout");

	auto layout = core::audio_channel_layout::invalid();

	if (channel_layout)
	{
		CASPAR_SCOPED_CONTEXT_MSG("/channel-layout")

		auto found_layout = core::audio_channel_layout_repository::get_default()->get_layout(*channel_layout);

		if (!found_layout)
			CASPAR_THROW_EXCEPTION(user_error() << msg_info(L"Channel layout " + *channel_layout + L" not found"));

		layout = *found_layout;
	}

	return spl::make_shared<bluefish_consumer_proxy>(device_index, embedded_audio, key_only, layout);
}

}}