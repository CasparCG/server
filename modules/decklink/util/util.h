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

#include <common/exception/exceptions.h>
#include <common/log/log.h>
#include <core/video_format.h>

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
				mode->GetDisplayMode() != format){}
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
	while(n < device_index && pDecklinkIterator->Next(&decklink) == S_OK){++n;}	

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

}}