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
* Author: Robert Nagy, ronag89@gmail.com
*/

#pragma once

#include <common/exception/exceptions.h>
#include <common/log/log.h>
#include <common/memory/memshfl.h>
#include <core/video_format.h>
#include <core/mixer/read_frame.h>

#include "../interop/DeckLinkAPI_h.h"

#include <boost/lexical_cast.hpp>

#include <atlbase.h>

#include <string>

namespace caspar { namespace decklink {
	
static BMDDisplayMode get_decklink_video_format(core::video_format::type fmt) 
{
	switch(fmt)
	{
	case core::video_format::pal:			return bmdModePAL;
	case core::video_format::ntsc:			return bmdModeNTSC;
	case core::video_format::x576p2500:		return (BMDDisplayMode)ULONG_MAX;
	case core::video_format::x720p2398:		return (BMDDisplayMode)ULONG_MAX;
	case core::video_format::x720p2400:		return (BMDDisplayMode)ULONG_MAX;
	case core::video_format::x720p2500:		return (BMDDisplayMode)ULONG_MAX;
	case core::video_format::x720p5000:		return bmdModeHD720p50;
	case core::video_format::x720p2997:		return (BMDDisplayMode)ULONG_MAX;
	case core::video_format::x720p5994:		return bmdModeHD720p5994;
	case core::video_format::x720p3000:		return (BMDDisplayMode)ULONG_MAX;
	case core::video_format::x720p6000:		return bmdModeHD720p60;
	case core::video_format::x1080p2398:	return bmdModeHD1080p2398;
	case core::video_format::x1080p2400:	return bmdModeHD1080p24;
	case core::video_format::x1080i5000:	return bmdModeHD1080i50;
	case core::video_format::x1080i5994:	return bmdModeHD1080i5994;
	case core::video_format::x1080i6000:	return bmdModeHD1080i6000;
	case core::video_format::x1080p2500:	return bmdModeHD1080p25;
	case core::video_format::x1080p2997:	return bmdModeHD1080p2997;
	case core::video_format::x1080p3000:	return bmdModeHD1080p30;
	case core::video_format::x1080p5000:	return bmdModeHD1080p50;
	case core::video_format::x1080p5994:	return bmdModeHD1080p5994;
	case core::video_format::x1080p6000:	return bmdModeHD1080p6000;
	case core::video_format::x1556p2398:	return bmdMode2k2398;
	case core::video_format::x1556p2400:	return bmdMode2k24;
	case core::video_format::x1556p2500:	return bmdMode2k25;
	case core::video_format::dci1080p2398:	return bmdMode2kDCI2398;
	case core::video_format::dci1080p2400:	return bmdMode2kDCI24;
	case core::video_format::dci1080p2500:	return bmdMode2kDCI25;
	case core::video_format::x2160p2398:	return bmdMode4K2160p2398;
	case core::video_format::x2160p2400:	return bmdMode4K2160p24;
	case core::video_format::x2160p2500:	return bmdMode4K2160p25;
	case core::video_format::x2160p2997:	return bmdMode4K2160p2997;
	case core::video_format::x2160p3000:	return bmdMode4K2160p30;
	case core::video_format::x2160p5000:	return bmdMode4K2160p50;
	case core::video_format::x2160p5994:	return bmdMode4K2160p5994;
	case core::video_format::x2160p6000:	return bmdMode4K2160p60;
	case core::video_format::dci2160p2398:	return bmdMode4kDCI2398;
	case core::video_format::dci2160p2400:	return bmdMode4kDCI24;
	case core::video_format::dci2160p2500:	return bmdMode4kDCI25;
	default:								return (BMDDisplayMode)ULONG_MAX;
	}
}

static core::video_format::type get_caspar_video_format(BMDDisplayMode fmt) 
{
	switch(fmt)
	{
	case bmdModePAL:						return core::video_format::pal;		
	case bmdModeNTSC:						return core::video_format::ntsc;		
	case bmdModeHD720p50:					return core::video_format::x720p5000;	
	case bmdModeHD720p5994:					return core::video_format::x720p5994;	
	case bmdModeHD720p60:					return core::video_format::x720p6000;	
	case bmdModeHD1080p2398:				return core::video_format::x1080p2398;	
	case bmdModeHD1080p24:					return core::video_format::x1080p2400;	
	case bmdModeHD1080i50:					return core::video_format::x1080i5000;	
	case bmdModeHD1080i5994:				return core::video_format::x1080i5994;	
	case bmdModeHD1080i6000:				return core::video_format::x1080i6000;	
	case bmdModeHD1080p25:					return core::video_format::x1080p2500;	
	case bmdModeHD1080p2997:				return core::video_format::x1080p2997;	
	case bmdModeHD1080p30:					return core::video_format::x1080p3000;	
	case bmdModeHD1080p50:					return core::video_format::x1080p5000;	
	case bmdModeHD1080p5994:				return core::video_format::x1080p5994;	
	case bmdModeHD1080p6000:				return core::video_format::x1080p6000;	
	case bmdMode2k2398:						return core::video_format::x1556p2398;	
	case bmdMode2k24:						return core::video_format::x1556p2400;	
	case bmdMode2k25:						return core::video_format::x1556p2500;	
	case bmdMode2kDCI2398:					return core::video_format::dci1080p2398;	
	case bmdMode2kDCI24:					return core::video_format::dci1080p2400;	
	case bmdMode2kDCI25:					return core::video_format::dci1080p2500;	
	case bmdMode4K2160p2398:				return core::video_format::x2160p2398;	
	case bmdMode4K2160p24:					return core::video_format::x2160p2400;	
	case bmdMode4K2160p25:					return core::video_format::x2160p2500;	
	case bmdMode4K2160p2997:				return core::video_format::x2160p2997;	
	case bmdMode4K2160p30:					return core::video_format::x2160p3000;	
	case bmdMode4K2160p50:					return core::video_format::x2160p5000;	
	case bmdMode4K2160p5994:				return core::video_format::x2160p5994;	
	case bmdMode4K2160p60:					return core::video_format::x2160p6000;	
	case bmdMode4kDCI2398:					return core::video_format::dci2160p2398;	
	case bmdMode4kDCI24:					return core::video_format::dci2160p2400;	
	case bmdMode4kDCI25:					return core::video_format::dci2160p2500;	
	default:								return core::video_format::invalid;	
	}
}

template<typename T, typename F>
BMDDisplayMode get_display_mode(const T& device, BMDDisplayMode format, BMDPixelFormat pix_fmt, F flag)
{
	CComPtr<IDeckLinkDisplayModeIterator> iterator;
	CComPtr<IDeckLinkDisplayMode>    	  mode;
	
	if(SUCCEEDED(device->GetDisplayModeIterator(&iterator)))
	{
		while(SUCCEEDED(iterator->Next(&mode)) && 
				mode != nullptr && 
				mode->GetDisplayMode() != format)
		{
			mode.Release();
		}
	}

	if(!mode)
		BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Device could not find requested video-format.") 
												 << arg_value_info(boost::lexical_cast<std::string>(format))
												 << arg_name_info("format"));
		
	BMDDisplayModeSupport displayModeSupport;
	if(FAILED(device->DoesSupportVideoMode(mode->GetDisplayMode(), pix_fmt, flag, &displayModeSupport, nullptr)) || displayModeSupport == bmdDisplayModeNotSupported)
		CASPAR_LOG(warning) << L"Device does not support video-format: " << mode->GetDisplayMode();
		//BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Device does not support requested video-format.")
		//										 << arg_value_info(boost::lexical_cast<std::string>(format))
		//										 << arg_name_info("format"));
	else if(displayModeSupport == bmdDisplayModeSupportedWithConversion)
		CASPAR_LOG(warning) << L"Device supports video-format with conversion: " << mode->GetDisplayMode();

	return mode->GetDisplayMode();
}

template<typename T, typename F>
static BMDDisplayMode get_display_mode(const T& device, core::video_format::type fmt, BMDPixelFormat pix_fmt, F flag)
{	
	return get_display_mode(device, get_decklink_video_format(fmt), pix_fmt, flag);
}

template<typename T>
static std::wstring get_version(T& iterator)
{
	CComQIPtr<IDeckLinkAPIInformation> info = iterator;
	if (!info)
		return L"Unknown";
	
	BSTR ver;		
	info->GetString(BMDDeckLinkAPIVersion, &ver);
		
	return ver;					
}

static CComPtr<IDeckLink> get_device(size_t device_index)
{
	CComPtr<IDeckLinkIterator> pDecklinkIterator;
	if(FAILED(pDecklinkIterator.CoCreateInstance(CLSID_CDeckLinkIterator)))
		BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Decklink drivers not found."));
		
	size_t n = 0;
	CComPtr<IDeckLink> decklink;
	CComPtr<IDeckLink> current;
	while (n < device_index && pDecklinkIterator->Next(&current) == S_OK)
	{
		++n;
		decklink = current;
		current.Release();
	}

	if(n != device_index || !decklink)
		BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Decklink device not found.") << arg_name_info("device_index") << arg_value_info(boost::lexical_cast<std::string>(device_index)));
		
	return decklink;
}

template <typename T>
static std::wstring get_model_name(const T& device)
{	
	BSTR pModelName;
	device->GetModelName(&pModelName);
	return std::wstring(pModelName);
}

static std::vector<uint8_t, tbb::cache_aligned_allocator<uint8_t>> extract_key(
		const safe_ptr<core::read_frame>& frame)
{
	std::vector<uint8_t, tbb::cache_aligned_allocator<uint8_t>> result;

	result.resize(frame->image_data().size());
	fast_memshfl(
			result.data(),
			frame->image_data().begin(),
			frame->image_data().size(),
			0x0F0F0F0F, 0x0B0B0B0B, 0x07070707, 0x03030303);

	return std::move(result);
}

class decklink_frame : public IDeckLinkVideoFrame
{
	tbb::atomic<int>											ref_count_;
	std::shared_ptr<core::read_frame>							frame_;
	const core::video_format_desc								format_desc_;

	const bool													key_only_;
	std::vector<uint8_t, tbb::cache_aligned_allocator<uint8_t>> data_;
public:
	decklink_frame(const safe_ptr<core::read_frame>& frame, const core::video_format_desc& format_desc, bool key_only)
		: frame_(frame)
		, format_desc_(format_desc)
		, key_only_(key_only)
	{
		ref_count_ = 0;
	}

	decklink_frame(const safe_ptr<core::read_frame>& frame, const core::video_format_desc& format_desc, std::vector<uint8_t, tbb::cache_aligned_allocator<uint8_t>>&& key_data)
		: frame_(frame)
		, format_desc_(format_desc)
		, key_only_(true)
		, data_(std::move(key_data))
	{
		ref_count_ = 0;
	}
	
	// IUnknown

	STDMETHOD (QueryInterface(REFIID, LPVOID*))		
	{
		return E_NOINTERFACE;
	}
	
	STDMETHOD_(ULONG,			AddRef())			
	{
		return ++ref_count_;
	}

	STDMETHOD_(ULONG,			Release())			
	{
		if(--ref_count_ == 0)
		{
			delete this;
			return 0;
		}

		return ref_count_;
	}

	// IDecklinkVideoFrame

	STDMETHOD_(long,			GetWidth())			{return format_desc_.width;}        
    STDMETHOD_(long,			GetHeight())		{return format_desc_.height;}        
    STDMETHOD_(long,			GetRowBytes())		{return format_desc_.width*4;}        
	STDMETHOD_(BMDPixelFormat,	GetPixelFormat())	{return bmdFormat8BitBGRA;}        
    STDMETHOD_(BMDFrameFlags,	GetFlags())			{return bmdFrameFlagDefault;}
        
    STDMETHOD(GetBytes(void** buffer))
	{
		try
		{
			if(static_cast<size_t>(frame_->image_data().size()) != format_desc_.size)
			{
				data_.resize(format_desc_.size, 0);
				*buffer = data_.data();
			}
			else if(key_only_)
			{
				if(data_.empty())
				{
					data_.resize(frame_->image_data().size());
					fast_memshfl(data_.data(), frame_->image_data().begin(), frame_->image_data().size(), 0x0F0F0F0F, 0x0B0B0B0B, 0x07070707, 0x03030303);
				}
				*buffer = data_.data();
			}
			else
				*buffer = const_cast<uint8_t*>(frame_->image_data().begin());
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
			return E_FAIL;
		}

		return S_OK;
	}
        
    STDMETHOD(GetTimecode(BMDTimecodeFormat format, IDeckLinkTimecode** timecode)){return S_FALSE;}        
    STDMETHOD(GetAncillaryData(IDeckLinkVideoFrameAncillary** ancillary))		  {return S_FALSE;}

	// decklink_frame	

	const boost::iterator_range<const int32_t*> audio_data()
	{
		return frame_->audio_data();
	}

	int64_t get_age_millis() const
	{
		return frame_->get_age_millis();
	}
};

struct configuration
{
	enum keyer_t
	{
		internal_keyer,
		external_keyer,
		external_separate_device_keyer,
		default_keyer
	};

	enum latency_t
	{
		low_latency,
		normal_latency,
		default_latency
	};

	size_t					device_index;
	size_t					key_device_idx;
	bool					embedded_audio;
	core::channel_layout	audio_layout;
	keyer_t					keyer;
	latency_t				latency;
	bool					key_only;
	size_t					base_buffer_depth;
	bool					custom_allocator;
	
	configuration()
		: device_index(1)
		, key_device_idx(0)
		, embedded_audio(false)
		, audio_layout(core::default_channel_layout_repository().get_by_name(L"STEREO"))
		, keyer(default_keyer)
		, latency(default_latency)
		, key_only(false)
		, base_buffer_depth(3)
		, custom_allocator(true)
	{
	}
	
	int buffer_depth() const
	{
		return base_buffer_depth + (latency == low_latency ? 0 : 1) + (embedded_audio ? 1 : 0);
	}

	size_t key_device_index() const
	{
		return key_device_idx == 0 ? device_index + 1 : key_device_idx;
	}

	int num_out_channels() const
	{
		if (audio_layout.num_channels <= 2)
			return 2;
		
		if (audio_layout.num_channels <= 8)
			return 8;

		return 16;
	}
};

static void set_latency(
		const CComQIPtr<IDeckLinkConfiguration>& config,
		configuration::latency_t latency,
		const std::wstring& print)
{		
	if (latency == configuration::low_latency)
	{
		config->SetFlag(bmdDeckLinkConfigLowLatencyVideoOutput, true);
		CASPAR_LOG(info) << print << L" Enabled low-latency mode.";
	}
	else if (latency == configuration::normal_latency)
	{			
		config->SetFlag(bmdDeckLinkConfigLowLatencyVideoOutput, false);
		CASPAR_LOG(info) << print << L" Disabled low-latency mode.";
	}
}

static void set_keyer(
		const CComQIPtr<IDeckLinkAttributes>& attributes,
		const CComQIPtr<IDeckLinkKeyer>& decklink_keyer,
		configuration::keyer_t keyer,
		const std::wstring& print)
{
	if (keyer == configuration::internal_keyer
			|| keyer == configuration::external_separate_device_keyer) 
	{
		BOOL value = true;
		if (SUCCEEDED(attributes->GetFlag(BMDDeckLinkSupportsInternalKeying, &value)) && !value)
			CASPAR_LOG(error) << print << L" Failed to enable internal keyer.";	
		else if (FAILED(decklink_keyer->Enable(FALSE)))			
			CASPAR_LOG(error) << print << L" Failed to enable internal keyer.";			
		else if (FAILED(decklink_keyer->SetLevel(255)))			
			CASPAR_LOG(error) << print << L" Failed to set key-level to max.";
		else
			CASPAR_LOG(info) << print << L" Enabled internal keyer.";		
	}
	else if (keyer == configuration::external_keyer)
	{
		BOOL value = true;
		if (SUCCEEDED(attributes->GetFlag(BMDDeckLinkSupportsExternalKeying, &value)) && !value)
			CASPAR_LOG(error) << print << L" Failed to enable external keyer.";	
		else if (FAILED(decklink_keyer->Enable(TRUE)))			
			CASPAR_LOG(error) << print << L" Failed to enable external keyer.";	
		else if (FAILED(decklink_keyer->SetLevel(255)))			
			CASPAR_LOG(error) << print << L" Failed to set key-level to max.";
		else
			CASPAR_LOG(info) << print << L" Enabled external keyer.";			
	}
}

template<typename Output>
class reference_signal_detector
{
	CComQIPtr<Output> output_;
	BMDReferenceStatus last_reference_status_;
public:
	reference_signal_detector(const CComQIPtr<Output>& output)
		: output_(output)
		, last_reference_status_(static_cast<BMDReferenceStatus>(-1))
	{
	}

	template<typename Print>
	void detect_change(const Print& print)
	{
		BMDReferenceStatus reference_status;

		if (output_->GetReferenceStatus(&reference_status) != S_OK)
		{
			CASPAR_LOG(error) << print() << L" Reference signal: failed while querying status";
		}
		else if (reference_status != last_reference_status_)
		{
			last_reference_status_ = reference_status;

			if (reference_status == 0)
				CASPAR_LOG(info) << print() << L" Reference signal: not detected.";
			else if (reference_status & bmdReferenceNotSupportedByHardware)
				CASPAR_LOG(info) << print() << L" Reference signal: not supported by hardware.";
			else if (reference_status & bmdReferenceLocked)
				CASPAR_LOG(info) << print() << L" Reference signal: locked.";
			else
				CASPAR_LOG(info) << print() << L" Reference signal: Unhandled enum bitfield: " << reference_status;
		}
	}
};

}}