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
 
#include "../../StdAfx.h"

#include "bluefish_consumer.h"
#include "util.h"
#include "memory.h"

#include <mixer/frame/read_frame.h>

#include <common/concurrency/executor.h>
#include <common/diagnostics/graph.h>
#include <common/utility/timer.h>

#include <tbb/concurrent_queue.h>

#include <BlueVelvet4.h>
#include <BlueHancUtils.h>

#include <windows.h>

#include <boost/thread/once.hpp>

#include <memory>
#include <array>

namespace caspar { namespace core {
	
CBlueVelvet4* (*BlueVelvetFactory4)() = nullptr;
const char*	(*BlueVelvetVersion)() = nullptr;
BLUE_UINT32 (*encode_hanc_frame)(struct hanc_stream_info_struct * hanc_stream_ptr, void * audio_pcm_ptr,BLUE_UINT32 no_audio_ch,BLUE_UINT32 no_audio_samples,BLUE_UINT32 nTypeOfSample,BLUE_UINT32 emb_audio_flag) = nullptr;
BLUE_UINT32 (*encode_hanc_frame_ex)(BLUE_UINT32 card_type, struct hanc_stream_info_struct * hanc_stream_ptr, void * audio_pcm_ptr, BLUE_UINT32 no_audio_ch,	BLUE_UINT32 no_audio_samples, BLUE_UINT32 nTypeOfSample, BLUE_UINT32 emb_audio_flag) = nullptr;

void blue_velvet_initialize()
{
#ifdef _DEBUG
	auto module = LoadLibrary(L"BlueVelvet3_d.dll");
#else
	auto module = LoadLibrary(L"BlueVelvet3.dll");
#endif
	if(!module)
		BOOST_THROW_EXCEPTION(file_not_found() << msg_info("Could not find BlueVelvet3.dll"));
	static std::shared_ptr<void> lib(module, FreeLibrary);
	BlueVelvetFactory4 = reinterpret_cast<decltype(BlueVelvetFactory4)>(GetProcAddress(module, "BlueVelvetFactory4"));
	BlueVelvetVersion = reinterpret_cast<decltype(BlueVelvetVersion)>(GetProcAddress(module, "BlueVelvetVersion"));
}

void blue_hanc_initialize()
{
#ifdef _DEBUG
	auto module = LoadLibrary(L"BlueHancUtils_d.dll");
#else
	auto module = LoadLibrary(L"BlueHancUtils.dll");
#endif
	if(!module)
		BOOST_THROW_EXCEPTION(file_not_found() << msg_info("Could not find BlueHancUtils.dll"));
	static std::shared_ptr<void> lib(module, FreeLibrary);
	encode_hanc_frame = reinterpret_cast<decltype(encode_hanc_frame)>(GetProcAddress(module, "encode_hanc_frame"));
	encode_hanc_frame_ex = reinterpret_cast<decltype(encode_hanc_frame_ex)>(GetProcAddress(module, "encode_hanc_frame_ex"));
}

void do_blue_initialize()
{
	blue_velvet_initialize();
	blue_hanc_initialize();
}

void blue_initialize()
{
	static boost::once_flag flag = BOOST_ONCE_INIT;
	boost::call_once(do_blue_initialize, flag);	
}
		
struct bluefish_consumer::implementation : boost::noncopyable
{
	printer				parent_printer_;
	std::wstring		model_name_;
	const unsigned int	device_index_;

	std::shared_ptr<diagnostics::graph> graph_;
	timer perf_timer_;

	boost::unique_future<void> active_;
			
	std::shared_ptr<CBlueVelvet4> blue_;
	
	video_format_desc	format_desc_;
		
	unsigned long	mem_fmt_;
	unsigned long	upd_fmt_;
	EVideoMode		vid_fmt_; 
	unsigned long	res_fmt_; 
	unsigned long	engine_mode_;
	
	std::array<blue_dma_buffer_ptr, 3> reserved_frames_;	

	const bool embed_audio_;

	executor executor_;
public:
	implementation::implementation(unsigned int device_index, bool embed_audio) 
		: model_name_(L"BLUEFISH")
		, device_index_(device_index) 
		, mem_fmt_(MEM_FMT_ARGB_PC)
		, upd_fmt_(UPD_FMT_FRAME)
		, vid_fmt_(VID_FMT_INVALID) 
		, res_fmt_(RES_FMT_NORMAL) 
		, engine_mode_(VIDEO_ENGINE_FRAMESTORE)		
		, embed_audio_(embed_audio)
		, executor_(print()){}

	~implementation()
	{
		if(executor_.is_running())
		{
			executor_.invoke([&]
			{
				disable_video_output();

				if(blue_)
					blue_->device_detach();		
			});
		}
		
		CASPAR_LOG(info) << print() << L" Shutting down.";	
	}

	void initialize(const video_format_desc& format_desc, const printer& parent_printer)
	{
		blue_initialize();
		
		format_desc_ = format_desc;
		parent_printer_ = parent_printer;

		blue_.reset(BlueVelvetFactory4());

		if(BLUE_FAIL(blue_->device_attach(device_index_, FALSE))) 
			BOOST_THROW_EXCEPTION(bluefish_exception() << msg_info(narrow(print()) + " Failed to attach device."));
	
		int videoCardType = blue_->has_video_cardtype();
		model_name_ = get_card_desc(videoCardType);

		graph_ = diagnostics::create_graph(narrow(print()));
		graph_->guide("tick-time", 0.5);
		graph_->set_color("tick-time", diagnostics::color(0.1f, 0.7f, 0.8f));
			
		//void* pBlueDevice = blue_attach_to_device(1);
		//EBlueConnectorPropertySetting video_routing[1];
		//auto channel = BLUE_VIDEO_OUTPUT_CHANNEL_A;
		//video_routing[0].channel = channel;	
		//video_routing[0].propType = BLUE_CONNECTOR_PROP_SINGLE_LINK;
		//video_routing[0].connector = channel == BLUE_VIDEO_OUTPUT_CHANNEL_A ? BLUE_CONNECTOR_SDI_OUTPUT_A : BLUE_CONNECTOR_SDI_OUTPUT_B;
		//blue_set_connector_property(pBlueDevice, 1, video_routing);
		//blue_detach_from_device(&pBlueDevice);
		
		auto desiredVideoFormat = vid_fmt_from_video_format(format_desc_.format);
		int videoModeCount = blue_->count_video_mode();
		for(int videoModeIndex = 1; videoModeIndex <= videoModeCount; ++videoModeIndex) 
		{
			EVideoMode videoMode = blue_->enum_video_mode(videoModeIndex);
			if(videoMode == desiredVideoFormat) 
				vid_fmt_ = videoMode;			
		}
		if(vid_fmt_ == VID_FMT_INVALID)
			BOOST_THROW_EXCEPTION(bluefish_exception() << msg_info(narrow(print()) + " Failed to set videomode."));
		
		// Set default video output channel
		//if(BLUE_FAIL(set_card_property(blue_, DEFAULT_VIDEO_OUTPUT_CHANNEL, channel)))
		//	CASPAR_LOG(error) << TEXT("BLUECARD ERROR: Failed to set default channel. (device ") << device_index_ << TEXT(")");

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

		if(!embed_audio_)
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
	
		if(BLUE_FAIL(blue_->set_video_engine(engine_mode_)))
			BOOST_THROW_EXCEPTION(bluefish_exception() << msg_info(narrow(print()) + " Failed to set video engine."));

		enable_video_output();
						
		for(size_t n = 0; n < reserved_frames_.size(); ++n)
			reserved_frames_[n] = std::make_shared<blue_dma_buffer>(format_desc_.size, n);		
				
		executor_.start();
		active_ = executor_.begin_invoke([]{});

		CASPAR_LOG(info) << print() << TEXT(" Successfully initialized for ") << format_desc_ << TEXT(".");
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

	virtual void send(const safe_ptr<const read_frame>& frame)
	{			
		static std::vector<short> silence(MAX_HANC_BUFFER_SIZE, 0);
		
		size_t audio_samples = static_cast<size_t>(48000.0 / format_desc_.fps);
		size_t audio_nchannels = 2;
		
		active_.get();
		active_ = executor_.begin_invoke([=]
		{
			try
			{
				std::copy_n(frame->image_data().begin(), frame->image_data().size(), reserved_frames_.front()->image_data());

				unsigned long n_field = 0;
				blue_->wait_output_video_synch(UPD_FMT_FRAME, n_field);
				
				if(embed_audio_)
				{		
					auto frame_audio_data = frame->audio_data().empty() ? silence.data() : const_cast<short*>(frame->audio_data().begin());

					encode_hanc(reinterpret_cast<BLUE_UINT32*>(reserved_frames_.front()->hanc_data()), frame_audio_data, audio_samples, audio_nchannels);
								
					blue_->system_buffer_write_async(const_cast<unsigned char*>(reserved_frames_.front()->image_data()), 
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
					blue_->system_buffer_write_async(const_cast<unsigned char*>(reserved_frames_.front()->image_data()),
													reserved_frames_.front()->image_size(), 
													nullptr,                 
													BlueImage_DMABuffer(reserved_frames_.front()->id(), BLUE_DATA_IMAGE));
			
					if(BLUE_FAIL(blue_->render_buffer_update(BlueBuffer_Image(reserved_frames_.front()->id()))))
						CASPAR_LOG(warning) << print() << TEXT(" render_buffer_update failed.");
				}

				std::rotate(reserved_frames_.begin(), reserved_frames_.begin() + 1, reserved_frames_.end());
				graph_->update("tick-time", static_cast<float>(perf_timer_.elapsed()/format_desc_.interval*0.5));
				perf_timer_.reset();
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
			}
		});
	}

	virtual size_t buffer_depth() const{return 1;}

	void encode_hanc(BLUE_UINT32* hanc_data, void* audio_data, size_t audio_samples, size_t audio_nchannels)
	{	
		auto card_type = blue_->has_video_cardtype();
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
			encode_hanc_frame(&hanc_stream_info, audio_data, audio_nchannels, audio_samples, sample_type, emb_audio_flag);	
		else
			encode_hanc_frame_ex(card_type, &hanc_stream_info, audio_data, audio_nchannels, audio_samples, sample_type, emb_audio_flag);
	}
	
	std::wstring print() const
	{
		return (parent_printer_ ? parent_printer_() + L"/" : L"") + model_name_ + L" [" + boost::lexical_cast<std::wstring>(device_index_) + L"]";
	}
};

bluefish_consumer::bluefish_consumer(bluefish_consumer&& other) : impl_(std::move(other.impl_)){}
bluefish_consumer::bluefish_consumer(unsigned int device_index, bool embed_audio) : impl_(new implementation(device_index, embed_audio)){}	
void bluefish_consumer::initialize(const video_format_desc& format_desc, const printer& parent_printer){impl_->initialize(format_desc, parent_printer);}
void bluefish_consumer::send(const safe_ptr<const read_frame>& frame){impl_->send(frame);}
size_t bluefish_consumer::buffer_depth() const{return impl_->buffer_depth();}
std::wstring bluefish_consumer::print() const {return impl_->print();}	

std::wstring get_bluefish_version()
{
	try
	{
		blue_initialize();
	}
	catch(...)
	{
		return L"Not found";
	}
	if(!BlueVelvetVersion)
		return L"Unknown";

	return widen(std::string(BlueVelvetVersion()));
}

std::vector<std::wstring> get_bluefish_device_list()
{
	std::vector<std::wstring> devices;

	try
	{
		blue_initialize();
		
		std::shared_ptr<CBlueVelvet4> blue(BlueVelvetFactory4());

		for(int n = 0; BLUE_PASS(blue->device_attach(n, FALSE)); ++n)
		{				
			devices.push_back(L"[" + boost::lexical_cast<std::wstring>(n+1) + L"] " + get_card_desc(blue->has_video_cardtype()));
			blue->device_detach();		
		}
	}
	catch(...){}

	return devices;
}

safe_ptr<frame_consumer> create_bluefish_consumer(const std::vector<std::wstring>& params)
{
	if(params.size() < 1 || params[0] != L"BLUEFISH")
		return frame_consumer::empty();
		
	int device_index = 1;
	bool embed_audio = false;

	if(params.size() > 1)
		device_index = lexical_cast_or_default<int>(params[2]);

	if(params.size() > 2) 
		embed_audio = lexical_cast_or_default<bool>(params[3]);

	return make_safe<bluefish_consumer>(device_index, embed_audio);
}

}}