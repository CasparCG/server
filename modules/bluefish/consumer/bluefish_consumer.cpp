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

#include <memory>
#include <array>

namespace caspar { namespace bluefish {

#define BLUEFISH_HW_BUFFER_DEPTH 1
#define BLUEFISH_SOFTWARE_BUFFERS 4

enum class hardware_downstream_keyer_mode
{
	disable = 0,
	external = 1,
	internal = 2,		// Bluefish dedicated HW keyer - only available on some models.
};

enum class hardware_downstream_keyer_audio_source
{
	SDIVideoInput = 1,
	VideoOutputChannel = 2
};

enum class bluefish_hardware_output_channel
{
	channel_a,
	channel_b,
	channel_c,
	channel_d,
	default_output_channel = channel_a
};

EBlueVideoChannel get_bluesdk_videochannel_from_streamid(bluefish_hardware_output_channel streamid)
{
	/*This function would return the corresponding EBlueVideoChannel from the device output channel*/
	switch (streamid)
	{
		case bluefish_hardware_output_channel::channel_a:	return BLUE_VIDEO_OUTPUT_CHANNEL_A;
		case bluefish_hardware_output_channel::channel_b:	return BLUE_VIDEO_OUTPUT_CHANNEL_B;
		case bluefish_hardware_output_channel::channel_c:	return BLUE_VIDEO_OUTPUT_CHANNEL_C;
		case bluefish_hardware_output_channel::channel_d:	return BLUE_VIDEO_OUTPUT_CHANNEL_D;
		default: return BLUE_VIDEO_OUTPUT_CHANNEL_A;
	}
}

bool get_videooutput_channel_routing_info_from_streamid(bluefish_hardware_output_channel streamid,
	EEpochRoutingElements & channelSrcElement,
	EEpochRoutingElements & sdioutputDstElement)
{
	switch (streamid)
	{
	case bluefish_hardware_output_channel::channel_a:	channelSrcElement = EPOCH_SRC_OUTPUT_MEM_INTERFACE_CHA;
		sdioutputDstElement = EPOCH_DEST_SDI_OUTPUT_A;
		break;
	case bluefish_hardware_output_channel::channel_b:	channelSrcElement = EPOCH_SRC_OUTPUT_MEM_INTERFACE_CHB;
		sdioutputDstElement = EPOCH_DEST_SDI_OUTPUT_B;
		break;
	case bluefish_hardware_output_channel::channel_c:	channelSrcElement = EPOCH_SRC_OUTPUT_MEM_INTERFACE_CHC;
		sdioutputDstElement = EPOCH_DEST_SDI_OUTPUT_C;
		break;
	case bluefish_hardware_output_channel::channel_d:	channelSrcElement = EPOCH_SRC_OUTPUT_MEM_INTERFACE_CHD;
		sdioutputDstElement = EPOCH_DEST_SDI_OUTPUT_D;
		break;
	default: return false;
	}
	return true;
}

struct bluefish_consumer : boost::noncopyable
{
	spl::shared_ptr<bvc_wrapper>								blue_;
	const unsigned int											device_index_;
	const core::video_format_desc								format_desc_;
	const core::audio_channel_layout							channel_layout_;
	core::audio_channel_remapper								channel_remapper_;
	const int													channel_index_;

	const std::wstring											model_name_;

	spl::shared_ptr<diagnostics::graph>							graph_;
	boost::timer												frame_timer_;
	boost::timer												tick_timer_;
	boost::timer												sync_timer_;

	unsigned int												vid_fmt_;

	std::array<blue_dma_buffer_ptr, BLUEFISH_SOFTWARE_BUFFERS>	all_frames_;
	tbb::concurrent_bounded_queue<blue_dma_buffer_ptr>			reserved_frames_;
	tbb::concurrent_bounded_queue<blue_dma_buffer_ptr>			live_frames_;
	std::shared_ptr<std::thread>								dma_present_thread_;
	tbb::atomic<bool>											end_dma_thread_;

	tbb::concurrent_bounded_queue<core::const_frame>			frame_buffer_;
	tbb::atomic<int64_t>										presentation_delay_millis_;
	core::const_frame											previous_frame_				= core::const_frame::empty();

	const bool													embedded_audio_;
	const bool													key_only_;

	executor													executor_;
	hardware_downstream_keyer_mode								hardware_keyer_;
	hardware_downstream_keyer_audio_source						keyer_audio_source_;
	bluefish_hardware_output_channel							device_output_channel_;
public:
	bluefish_consumer(
			const core::video_format_desc& format_desc,
			const core::audio_channel_layout& in_channel_layout,
			const core::audio_channel_layout& out_channel_layout,
			int device_index,
			bool embedded_audio,
			bool key_only,
			hardware_downstream_keyer_mode keyer,
			hardware_downstream_keyer_audio_source keyer_audio_source,
			int channel_index,
			bluefish_hardware_output_channel device_output_channel)
		: blue_(create_blue(device_index))
		, device_index_(device_index)
		, format_desc_(format_desc)
		, channel_layout_(out_channel_layout)
		, channel_remapper_(in_channel_layout, out_channel_layout)
		, channel_index_(channel_index)
		, model_name_(get_card_desc(*blue_, device_index))
		, vid_fmt_(get_video_mode(*blue_, format_desc))
		, embedded_audio_(embedded_audio)
		, hardware_keyer_(keyer)
		, keyer_audio_source_(keyer_audio_source)
		, key_only_(key_only)
		, executor_(print())
		, device_output_channel_(device_output_channel)
	{
		executor_.set_capacity(1);
		presentation_delay_millis_ = 0;

		reserved_frames_.set_capacity(BLUEFISH_SOFTWARE_BUFFERS);
		live_frames_.set_capacity(BLUEFISH_SOFTWARE_BUFFERS);

		graph_->set_color("tick-time", diagnostics::color(0.0f, 0.6f, 0.9f));
		graph_->set_color("sync-time", diagnostics::color(1.0f, 0.0f, 0.0f));
		graph_->set_color("frame-time", diagnostics::color(0.5f, 1.0f, 0.2f));
		graph_->set_text(print());
		diagnostics::register_graph(graph_);

		// Specify the video channel
		setup_hardware_output_channel();

		// Setting output Video mode
		if (BLUE_FAIL(blue_->set_card_property32(VIDEO_MODE, vid_fmt_)))
			CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(print() + L" Failed to set videomode."));

		// Select Update Mode for output
		if(BLUE_FAIL(blue_->set_card_property32(VIDEO_UPDATE_TYPE, UPD_FMT_FRAME)))
			CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(print() + L" Failed to set update type."));

		disable_video_output();
		setup_hardware_output_channel_routing();

		//Select output memory format
		if(BLUE_FAIL(blue_->set_card_property32(VIDEO_MEMORY_FORMAT, MEM_FMT_ARGB_PC)))
			CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(print() + L" Failed to set memory format."));

		//Select image orientation
		if(BLUE_FAIL(blue_->set_card_property32(VIDEO_IMAGE_ORIENTATION, ImageOrientation_Normal)))
			CASPAR_LOG(warning) << print() << L" Failed to set image orientation to normal.";

		// Select data range
		if(BLUE_FAIL(blue_->set_card_property32(VIDEO_RGB_DATA_RANGE, CGR_RANGE)))
			CASPAR_LOG(warning) << print() << L" Failed to set RGB data range to CGR.";

		if(!embedded_audio_ || (hardware_keyer_ == hardware_downstream_keyer_mode::internal && keyer_audio_source_ == hardware_downstream_keyer_audio_source::SDIVideoInput) )
		{
			if(BLUE_FAIL(blue_->set_card_property32(EMBEDEDDED_AUDIO_OUTPUT, 0)))
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

			if(BLUE_FAIL(blue_->set_card_property32(EMBEDEDDED_AUDIO_OUTPUT, audio_value)))
				CASPAR_LOG(warning) << print() << TEXT(" Failed to enable embedded audio.");
			CASPAR_LOG(info) << print() << TEXT(" Enabled embedded-audio.");
		}

		if(BLUE_FAIL(blue_->set_card_property32(VIDEO_OUTPUT_ENGINE, VIDEO_ENGINE_PLAYBACK)))
			CASPAR_LOG(warning) << print() << TEXT(" Failed to set video engine.");

		if (is_epoch_card((*blue_)))
			setup_hardware_downstream_keyer(hardware_keyer_, keyer_audio_source_);

		enable_video_output();

		int n = 0;
		boost::range::generate(all_frames_, [&]{return std::make_shared<blue_dma_buffer>(static_cast<int>(format_desc_.size), n++);});

		for (size_t i = 0; i < all_frames_.size(); i++)
			reserved_frames_.push(all_frames_[i]);
	}

	~bluefish_consumer()
	{
		try
		{
			executor_.invoke([&]
			{
				end_dma_thread_ = true;
				disable_video_output();
				blue_->detach();

				if (dma_present_thread_)
					dma_present_thread_->join();
			});
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
		}
	}

	void setup_hardware_output_channel()
	{
		// This function would be used to setup the logic video channel in the bluefish hardware
		EBlueVideoChannel out_vid_channel = get_bluesdk_videochannel_from_streamid(device_output_channel_);
		if (is_epoch_card((*blue_)))
		{
			if (out_vid_channel != BLUE_VIDEOCHANNEL_INVALID)
			{
				if (BLUE_FAIL(blue_->set_card_property32(DEFAULT_VIDEO_OUTPUT_CHANNEL, out_vid_channel)))
					CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(" Failed to set video stream."));

				blue_->video_playback_stop(0, 0);
			}
		}
	}

	void setup_hardware_output_channel_routing()
	{
		//This function would be used to setup the dual link and any other routing that would be required .
		if (is_epoch_card(*blue_))
		{
			EBlueVideoChannel blueVideoOutputChannel = get_bluesdk_videochannel_from_streamid(device_output_channel_);
			EEpochRoutingElements src_element = (EEpochRoutingElements)0;
			EEpochRoutingElements dst_element = (EEpochRoutingElements)0;
			get_videooutput_channel_routing_info_from_streamid(device_output_channel_, src_element, dst_element);
			bool duallink_4224_enabled = false;

			if ((device_output_channel_ == bluefish_hardware_output_channel::channel_a || device_output_channel_ == bluefish_hardware_output_channel::channel_c) &&
				(hardware_keyer_ == hardware_downstream_keyer_mode::external) || (hardware_keyer_ == hardware_downstream_keyer_mode::internal) )
			{
				duallink_4224_enabled = true;
			}

			// Enable/Disable dual link output
			if (BLUE_FAIL(blue_->set_card_property32(VIDEO_DUAL_LINK_OUTPUT, duallink_4224_enabled)))
				CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(print() + L" Failed to enable/disable dual link."));

			if (!duallink_4224_enabled)
			{
				if (BLUE_FAIL(blue_->set_card_property32(VIDEO_DUAL_LINK_OUTPUT_SIGNAL_FORMAT_TYPE, Signal_FormatType_Independent_422)))
					CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(print() + L" Failed to set dual link format type to 4:2:2."));

				ULONG routingValue = EPOCH_SET_ROUTING(src_element, dst_element, BLUE_CONNECTOR_PROP_SINGLE_LINK);
				if (BLUE_FAIL(blue_->set_card_property32(MR2_ROUTING, routingValue)))
					CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(" Failed to MR 2 routing."));

				// If single link 422, but on second channel AND on Neutron we need to set Genlock to Aux.
				if (is_epoch_neutron_1i2o_card((*blue_)))
				{
					if (blueVideoOutputChannel == BLUE_VIDEO_OUTPUT_CHANNEL_B)
					{
						ULONG genLockSource = BlueGenlockAux;
						if (BLUE_FAIL(blue_->set_card_property32(VIDEO_GENLOCK_SIGNAL, genLockSource)))
							CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info("Failed to set GenLock to Aux Input."));
					}
				}
				if (is_epoch_neutron_3o_card((*blue_)))
				{
					if (blueVideoOutputChannel == BLUE_VIDEO_OUTPUT_CHANNEL_C)
					{
						ULONG genLockSource = BlueGenlockAux;
						if (BLUE_FAIL(blue_->set_card_property32(VIDEO_GENLOCK_SIGNAL, genLockSource)))
							CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info("Failed to set GenLock to Aux Input."));
					}
				}
			}
			else		// dual Link IS enabled, ie. 4224 Fill and Key
			{
				if (BLUE_FAIL(blue_->set_card_property32(VIDEO_DUAL_LINK_OUTPUT_SIGNAL_FORMAT_TYPE, Signal_FormatType_4224)))
					CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(print() + L" Failed to set dual link format type to 4:2:2:4."));

				if (is_epoch_neutron_1i2o_card((*blue_)))		// Neutron cards require setting the Genlock conector to Aux to enable them to do Dual-Link
				{
					ULONG genLockSource = BlueGenlockAux;
					if (BLUE_FAIL(blue_->set_card_property32(VIDEO_GENLOCK_SIGNAL, genLockSource)))
						CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info("Failed to set GenLock to Aux Input."));
				}
				else if (is_epoch_neutron_3o_card((*blue_)))
				{
					if (blueVideoOutputChannel == BLUE_VIDEO_OUTPUT_CHANNEL_C)
					{
						ULONG genLockSource = BlueGenlockAux;
						if (BLUE_FAIL(blue_->set_card_property32(VIDEO_GENLOCK_SIGNAL, genLockSource)))
							CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info("Failed to set GenLock to Aux Input."));
					}
				}
			}
		}
	}

	void setup_hardware_downstream_keyer(hardware_downstream_keyer_mode keyer, hardware_downstream_keyer_audio_source audio_source)
	{
		unsigned int keyer_control_value = 0, card_feature_value = 0;
		unsigned int card_connector_value = 0;
		unsigned int nOutputStreams = 0;
		unsigned int nInputStreams = 0;
		unsigned int nInputSDIConnector = 0;
		unsigned int nOutputSDIConnector = 0;
		if (BLUE_OK(blue_->get_card_property32(CARD_FEATURE_STREAM_INFO, card_feature_value)))
		{
			nOutputStreams = CARD_FEATURE_GET_SDI_OUTPUT_STREAM_COUNT(card_feature_value);
			nInputStreams = CARD_FEATURE_GET_SDI_INPUT_STREAM_COUNT(card_feature_value);
		}
		if (BLUE_OK(blue_->get_card_property32(CARD_FEATURE_CONNECTOR_INFO, card_connector_value)))
		{
			nOutputSDIConnector = CARD_FEATURE_GET_SDI_OUTPUT_CONNECTOR_COUNT(card_connector_value);
			nInputSDIConnector = CARD_FEATURE_GET_SDI_INPUT_CONNECTOR_COUNT(card_connector_value);
		}
		if (nInputSDIConnector == 0 || nInputStreams == 0)
			return;

		if (keyer == hardware_downstream_keyer_mode::disable || keyer == hardware_downstream_keyer_mode::external)
		{
			keyer_control_value = VIDEO_ONBOARD_KEYER_SET_STATUS_DISABLED(keyer_control_value);
			keyer_control_value = VIDEO_ONBOARD_KEYER_SET_STATUS_DISABLE_OVER_BLACK(keyer_control_value);
		}
		else if (keyer == hardware_downstream_keyer_mode::internal)
		{
			unsigned int invalidVideoModeFlag = 0;
			unsigned int inputVideoSignal = 0;
			if (BLUE_FAIL(blue_->get_card_property32(INVALID_VIDEO_MODE_FLAG, invalidVideoModeFlag)))
				CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(" Failed to get invalid video mode flag"));

			// The bluefish HW keyer is NOT going to pre-multiply the RGB with the A.
			keyer_control_value = VIDEO_ONBOARD_KEYER_SET_STATUS_DATA_IS_PREMULTIPLIED(keyer_control_value);

			keyer_control_value = VIDEO_ONBOARD_KEYER_SET_STATUS_ENABLED(keyer_control_value);
			if (BLUE_FAIL(blue_->get_card_property32(VIDEO_INPUT_SIGNAL_VIDEO_MODE, inputVideoSignal)))
				CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(" Failed to get video input signal mode"));

			if (inputVideoSignal >= invalidVideoModeFlag)
				keyer_control_value = VIDEO_ONBOARD_KEYER_SET_STATUS_ENABLE_OVER_BLACK(keyer_control_value);
			else
				keyer_control_value = VIDEO_ONBOARD_KEYER_SET_STATUS_DISABLE_OVER_BLACK(keyer_control_value);

			// lock to input
			if (BLUE_FAIL(blue_->set_card_property32(VIDEO_GENLOCK_SIGNAL, BlueSDI_A_BNC)))
				CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(" Failed to set the genlock to the input for the HW keyer"));
		}

		if (audio_source == hardware_downstream_keyer_audio_source::SDIVideoInput && (keyer == hardware_downstream_keyer_mode::internal))
			keyer_control_value = VIDEO_ONBOARD_KEYER_SET_STATUS_USE_INPUT_ANCILLARY(keyer_control_value);
		else if (audio_source == hardware_downstream_keyer_audio_source::VideoOutputChannel)
			keyer_control_value = VIDEO_ONBOARD_KEYER_SET_STATUS_USE_OUTPUT_ANCILLARY(keyer_control_value);

		if (BLUE_FAIL(blue_->set_card_property32(VIDEO_ONBOARD_KEYER, keyer_control_value)))
			CASPAR_LOG(error) << print() << TEXT(" Failed to set keyer control.");
	}

	void enable_video_output()
	{
		if(BLUE_FAIL(blue_->set_card_property32(VIDEO_BLACKGENERATOR, 0)))
			CASPAR_LOG(error) << print() << TEXT(" Failed to disable video output.");
	}

	void disable_video_output()
	{
		blue_->video_playback_stop(0,0);
		blue_->set_card_property32(VIDEO_DUAL_LINK_OUTPUT, 0);
		ULONG routingValue = EPOCH_SET_ROUTING(EPOCH_SRC_OUTPUT_MEM_INTERFACE_CHA, EPOCH_DEST_SDI_OUTPUT_B, BLUE_CONNECTOR_PROP_SINGLE_LINK);
		blue_->set_card_property32(MR2_ROUTING, routingValue);

		if(BLUE_FAIL(blue_->set_card_property32(VIDEO_BLACKGENERATOR, 1)))
			CASPAR_LOG(error)<< print() << TEXT(" Failed to disable video output.");
		if (BLUE_FAIL(blue_->set_card_property32(EMBEDEDDED_AUDIO_OUTPUT, 0)))
			CASPAR_LOG(error) << print() << TEXT(" Failed to disable audio output.");

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

	void dma_present_thread_actual()
	{
		ensure_gpf_handler_installed_for_thread("bluefish consumer DMA thread");

		bvc_wrapper wait_b;
		wait_b.attach(device_index_);
		EBlueVideoChannel out_vid_channel = get_bluesdk_videochannel_from_streamid(device_output_channel_);
		wait_b.set_card_property32(DEFAULT_VIDEO_OUTPUT_CHANNEL, out_vid_channel);
		int frames_to_buffer = BLUEFISH_HW_BUFFER_DEPTH;
		unsigned long buffer_id = 0;
		unsigned long underrun = 0;

		while (!end_dma_thread_)
		{
			blue_dma_buffer_ptr buf = nullptr;
			if (live_frames_.try_pop(buf) && BLUE_OK(blue_->video_playback_allocate(buffer_id, underrun)))
			{
				// Send and display
				if (embedded_audio_)
				{
					// Do video first, then do hanc DMA...
					blue_->system_buffer_write(const_cast<uint8_t*>(buf->image_data()),
						static_cast<unsigned long>(buf->image_size()),
						BlueImage_HANC_DMABuffer(buffer_id, BLUE_DATA_IMAGE),
						0);

					blue_->system_buffer_write(buf->hanc_data(),
						static_cast<unsigned long>(buf->hanc_size()),
						BlueImage_HANC_DMABuffer(buffer_id, BLUE_DATA_HANC),
						0);

					if (BLUE_FAIL(blue_->video_playback_present(BlueBuffer_Image_HANC(buffer_id), 1, 0, 0)))
					{
						CASPAR_LOG(warning) << print() << TEXT(" video_playback_present failed.");
					}
				}
				else
				{
					blue_->system_buffer_write(const_cast<uint8_t*>(buf->image_data()),
						static_cast<unsigned long>(buf->image_size()),
						BlueImage_DMABuffer(buffer_id, BLUE_DATA_IMAGE),
						0);

					if (BLUE_FAIL(blue_->video_playback_present(BlueBuffer_Image(buffer_id), 1, 0, 0)))
						CASPAR_LOG(warning) << print() << TEXT(" video_playback_present failed.");
				}

				reserved_frames_.push(buf);
			}
			else
			{
				// do WFS
				unsigned long n_field = 0;
				wait_b.wait_video_output_sync(UPD_FMT_FRAME, n_field);
			}

			if (frames_to_buffer > 0)
			{
				frames_to_buffer--;
				if (frames_to_buffer == 0)
				{
					if (BLUE_FAIL(blue_->video_playback_start(0, 0)))
						CASPAR_LOG(warning) << print() << TEXT("Error video playback start failed");
				}
			}
		}
		wait_b.detach();
	}

	void display_frame(core::const_frame frame)
	{
		frame_timer_.restart();

		if (previous_frame_ != core::const_frame::empty())
			presentation_delay_millis_ = previous_frame_.get_age_millis();

		previous_frame_ = frame;
		blue_dma_buffer_ptr buf = nullptr;

		// Copy to local buffers
		if (reserved_frames_.try_pop(buf))
		{
			void* dest = buf->image_data();
			if (!frame.image_data().empty())
			{
				if (key_only_)
					aligned_memshfl(dest, frame.image_data().begin(), frame.image_data().size(), 0x0F0F0F0F, 0x0B0B0B0B, 0x07070707, 0x03030303);
				else
					std::memcpy(dest, frame.image_data().begin(), frame.image_data().size());
			}
			else
				std::memset(dest, 0, buf->image_size());

			frame_timer_.restart();

			// remap, encode and copy hanc data
			if (embedded_audio_)
			{
				auto remapped_audio = channel_remapper_.mix_and_rearrange(frame.audio_data());
				auto frame_audio = core::audio_32_to_24(remapped_audio);
				encode_hanc(reinterpret_cast<BLUE_UINT32*>(buf->hanc_data()),
					frame_audio.data(),
					static_cast<int>(frame.audio_data().size() / channel_layout_.num_channels),
					static_cast<int>(channel_layout_.num_channels));
			}
			live_frames_.push(buf);

			// start the thread if required.
			if (dma_present_thread_ == 0)
			{
				end_dma_thread_ = false;
				dma_present_thread_ = std::make_shared<std::thread>([this] {dma_present_thread_actual(); });
#if defined(_WIN32)
				HANDLE handle = (HANDLE)dma_present_thread_->native_handle();
				SetThreadPriority(handle, THREAD_PRIORITY_HIGHEST);
#endif
			}
		}
		graph_->set_value("frame-time", static_cast<float>(frame_timer_.elapsed()*format_desc_.fps*0.5));

		// Sync
		sync_timer_.restart();
		unsigned long n_field = 0;
		blue_->wait_video_output_sync(UPD_FMT_FRAME, n_field);
		graph_->set_value("sync-time", sync_timer_.elapsed()*format_desc_.fps*0.5);
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

		int cardType = CRD_INVALID;
		blue_->query_card_type(cardType, device_index_);
		blue_->encode_hanc_frame(cardType, &hanc_stream_info, audio_data, audio_nchannels, audio_samples, sample_type, emb_audio_flag);
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
	core::monitor::subject					monitor_subject_;

	std::unique_ptr<bluefish_consumer>		consumer_;
	const int								device_index_;
	const bool								embedded_audio_;
	const bool								key_only_;

	std::vector<int>						audio_cadence_;
	core::video_format_desc					format_desc_;
	core::audio_channel_layout				in_channel_layout_		= core::audio_channel_layout::invalid();
	core::audio_channel_layout				out_channel_layout_;
	hardware_downstream_keyer_mode			hardware_keyer_;
	hardware_downstream_keyer_audio_source	hardware_keyer_audio_source_;
	bluefish_hardware_output_channel		hardware_output_channel_;

public:

	bluefish_consumer_proxy(int device_index,
							bool embedded_audio,
							bool key_only,
							hardware_downstream_keyer_mode keyer,
							hardware_downstream_keyer_audio_source keyer_audio_source,
							const core::audio_channel_layout& out_channel_layout,
							bluefish_hardware_output_channel hardware_output_channel)

		: device_index_(device_index)
		, embedded_audio_(embedded_audio)
		, key_only_(key_only)
		, hardware_keyer_(keyer)
		, hardware_keyer_audio_source_(keyer_audio_source)
		, out_channel_layout_(out_channel_layout)
		, hardware_output_channel_(hardware_output_channel)
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
		consumer_.reset(new bluefish_consumer(	format_desc,
												in_channel_layout_,
												out_channel_layout_,
												device_index_,
												embedded_audio_,
												key_only_,
												hardware_keyer_,
												hardware_keyer_audio_source_,
												channel_index,
												hardware_output_channel_));
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
		return BLUEFISH_HW_BUFFER_DEPTH;
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
	sink.syntax(L"BLUEFISH {[device_index:int]|1} {[sdi_device:int]|a} {[embedded_audio:EMBEDDED_AUDIO]} {[key_only:KEY_ONLY]} {CHANNEL_LAYOUT [channel_layout:string]} {[keyer:string|disabled]} ");
	sink.para()
		->text(L"Sends video on an SDI output using Bluefish video cards. Multiple devices can be ")
		->text(L"installed in the same machine and used at the same time, they will be addressed via ")
		->text(L"different ")->code(L"device_index")->text(L" parameters.");
	sink.para()->text(L"Multiple output channels can be accessed via the ")->code(L"sdi_device")->text(L" parameter.");
	sink.para()->text(L"Specify ")->code(L"embedded_audio")->text(L" to embed audio into the SDI signal.");
	sink.para()
		->text(L"Specifying ")->code(L"key_only")->text(L" will extract only the alpha channel from the ")
		->text(L"channel. This is useful when you have two SDI video cards, and neither has native support ")
		->text(L"for separate fill/key output");
	sink.para()->text(L"Specify ")->code(L"channel_layout")->text(L" to output a different audio channel layout than the channel uses.");
	sink.para()->text(L"Specify ")->code(L"keyer")->text(L" to control the output channel configuration and hardware keyer")
		->text(L"disabled results in a single SDI stream of 422 output - This is the default")
		->text(L"external results in a 4224 stream across 2 SDI connectors, ")
		->text(L"internal results in a 422 output keyed over the incoming SDI input using the dedicated hardware keyer on the Bleufish hadrware");
	sink.para()->text(L"Specify ")->code(L"internal-keyer-audio-source")->text(L" to control the source of the audio and ANC data when using the internal/hardware keyer");

	sink.para()->text(L"Examples:");
	sink.example(L">> ADD 1 BLUEFISH", L"uses the default device_index of 1.");
	sink.example(L">> ADD 1 BLUEFISH 2", L"for device_index 2.");
	sink.example(
		L">> ADD 1 BLUEFISH 1 EMBEDDED_AUDIO\n"

		L">> ADD 1 BLUEFISH 2 KEY_ONLY", L"uses device with index 1 as fill output with audio and device with index 2 as key output.");

}


spl::shared_ptr<core::frame_consumer> create_consumer(	const std::vector<std::wstring>& params,
														core::interaction_sink*,
														std::vector<spl::shared_ptr<core::video_channel>> channels,
														spl::shared_ptr<core::consumer_delayed_responder> responder)
{
	if(params.size() < 1 || !boost::iequals(params.at(0), L"BLUEFISH"))
		return core::frame_consumer::empty();

	const auto device_index			= params.size() > 1 ? boost::lexical_cast<int>(params.at(1)) : 1;
	const auto device_stream		= contains_param(	L"SDI-STREAM", params);
	const auto embedded_audio		= contains_param(	L"EMBEDDED_AUDIO",	params);
	const auto key_only				= contains_param(	L"KEY_ONLY",		params);
	const auto channel_layout		= get_param(		L"CHANNEL_LAYOUT",	params);
	const auto keyer_option			= contains_param(	L"KEYER",			params);
	const auto keyer_audio_option	= contains_param(	L"INTERNAL-KEYER-AUDIO-SOURCE", params);

	auto layout = core::audio_channel_layout::invalid();

	if (!channel_layout.empty())
	{
		auto found_layout = core::audio_channel_layout_repository::get_default()->get_layout(channel_layout);

		if (!found_layout)
			CASPAR_THROW_EXCEPTION(user_error() << msg_info(L"Channel layout " + channel_layout + L" not found"));

		layout = *found_layout;
	}

	bluefish_hardware_output_channel device_output_channel = bluefish_hardware_output_channel::channel_a;
	if (contains_param(L"A", params))
		device_output_channel = bluefish_hardware_output_channel::channel_a;
	else if (contains_param(L"B", params))
		device_output_channel = bluefish_hardware_output_channel::channel_b;
	else if (contains_param(L"C", params))
		device_output_channel = bluefish_hardware_output_channel::channel_c;
	else if (contains_param(L"D", params))
		device_output_channel = bluefish_hardware_output_channel::channel_d;

	hardware_downstream_keyer_mode keyer = hardware_downstream_keyer_mode::disable;
	if (contains_param(L"DISABLED", params))
		keyer = hardware_downstream_keyer_mode::disable;
	else if (contains_param(L"EXTERNAL", params))
		keyer = hardware_downstream_keyer_mode::external;
	else if (contains_param(L"INTERNAL", params))
		keyer = hardware_downstream_keyer_mode::internal;

	hardware_downstream_keyer_audio_source keyer_audio_source = hardware_downstream_keyer_audio_source::VideoOutputChannel;
	if (contains_param(L"SDIVIDEOINPUT", params))
		keyer_audio_source = hardware_downstream_keyer_audio_source::SDIVideoInput;
	else
	if (contains_param(L"VIDEOOUTPUTCHANNEL", params))
		keyer_audio_source = hardware_downstream_keyer_audio_source::VideoOutputChannel;

	return spl::make_shared<bluefish_consumer_proxy>(device_index, embedded_audio, key_only, keyer, keyer_audio_source, layout, device_output_channel);
}

spl::shared_ptr<core::frame_consumer> create_preconfigured_consumer(
											const boost::property_tree::wptree& ptree, core::interaction_sink*,
											std::vector<spl::shared_ptr<core::video_channel>> channels)
{
	const auto device_index		= ptree.get(						L"device",			1);
	const auto device_stream	= ptree.get(						L"sdi-stream", L"a");
	const auto embedded_audio	= ptree.get(						L"embedded-audio",	false);
	const auto key_only			= ptree.get(						L"key-only",		false);
	const auto channel_layout	= ptree.get_optional<std::wstring>(	L"channel-layout");
	const auto hardware_keyer_value = ptree.get(					L"keyer", L"disabled");
	const auto keyer_audio_source_value = ptree.get(				L"internal-keyer-audio-source", L"videooutputchannel");

	auto layout = core::audio_channel_layout::invalid();

	if (channel_layout)
	{
		CASPAR_SCOPED_CONTEXT_MSG("/channel-layout")

		auto found_layout = core::audio_channel_layout_repository::get_default()->get_layout(*channel_layout);

		if (!found_layout)
			CASPAR_THROW_EXCEPTION(user_error() << msg_info(L"Channel layout " + *channel_layout + L" not found"));

		layout = *found_layout;
	}

	bluefish_hardware_output_channel device_output_channel = bluefish_hardware_output_channel::channel_a;
	if (device_stream == L"a")
		device_output_channel = bluefish_hardware_output_channel::channel_a;
	else if (device_stream == L"b")
		device_output_channel = bluefish_hardware_output_channel::channel_b;
	else if (device_stream == L"c")
		device_output_channel = bluefish_hardware_output_channel::channel_c;
	else if (device_stream == L"d")
		device_output_channel = bluefish_hardware_output_channel::channel_d;

	hardware_downstream_keyer_mode keyer_mode = hardware_downstream_keyer_mode::disable;
	if (hardware_keyer_value == L"disabled")
		keyer_mode = hardware_downstream_keyer_mode::disable;
	else if (hardware_keyer_value == L"external")
		keyer_mode = hardware_downstream_keyer_mode::external;
	else if (hardware_keyer_value == L"internal")
		keyer_mode = hardware_downstream_keyer_mode::internal;

	hardware_downstream_keyer_audio_source keyer_audio_source = hardware_downstream_keyer_audio_source::VideoOutputChannel;
	if (keyer_audio_source_value == L"videooutputchannel")
		keyer_audio_source = hardware_downstream_keyer_audio_source::VideoOutputChannel;
	else
		if (keyer_audio_source_value == L"sdivideoinput")
			keyer_audio_source = hardware_downstream_keyer_audio_source::SDIVideoInput;

	return spl::make_shared<bluefish_consumer_proxy>(device_index, embedded_audio, key_only, keyer_mode, keyer_audio_source, layout, device_output_channel);
}

}}
