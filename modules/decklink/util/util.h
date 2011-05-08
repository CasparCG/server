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
#pragma once

#include <common/exception/exceptions.h>
#include <core/video_format.h>

#include "../interop/DeckLinkAPI_h.h"

#include <atlbase.h>

namespace caspar { 
	
static BMDDisplayMode GetDecklinkVideoFormat(core::video_format::type fmt) 
{
	switch(fmt)
	{
	case core::video_format::pal:			return bmdModePAL;
	case core::video_format::ntsc:			return bmdModeNTSC;
	case core::video_format::x576p2500:		return (BMDDisplayMode)ULONG_MAX;
	case core::video_format::x720p2500:		return (BMDDisplayMode)ULONG_MAX;
	case core::video_format::x720p5000:		return bmdModeHD720p50;
	case core::video_format::x720p5994:		return bmdModeHD720p5994;
	case core::video_format::x720p6000:		return bmdModeHD720p60;
	case core::video_format::x1080p2397:	return bmdModeHD1080p2398;
	case core::video_format::x1080p2400:	return bmdModeHD1080p24;
	case core::video_format::x1080i5000:	return bmdModeHD1080i50;
	case core::video_format::x1080i5994:	return bmdModeHD1080i5994;
	case core::video_format::x1080i6000:	return bmdModeHD1080i6000;
	case core::video_format::x1080p2500:	return bmdModeHD1080p25;
	case core::video_format::x1080p2997:	return bmdModeHD1080p2997;
	case core::video_format::x1080p3000:	return bmdModeHD1080p30;
	default:						return (BMDDisplayMode)ULONG_MAX;
	}
}
	
template<typename T>
static IDeckLinkDisplayMode* get_display_mode(const T& output, BMDDisplayMode format)
{
	CComPtr<IDeckLinkDisplayModeIterator> iterator;
	IDeckLinkDisplayMode*				mode;
	
	if(FAILED(output->GetDisplayModeIterator(&iterator)))
		return nullptr;

	while(SUCCEEDED(iterator->Next(&mode)) && mode != nullptr)
	{	
		if(mode->GetDisplayMode() == format)
			return mode;
	}

	return nullptr;
}

template<typename T>
static IDeckLinkDisplayMode* get_display_mode(const T& output, core::video_format::type fmt)
{
	return get_display_mode(output, GetDecklinkVideoFormat(fmt));
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
	CComPtr<IDeckLink> decklink;

	CComPtr<IDeckLinkIterator> pDecklinkIterator;
	if(FAILED(pDecklinkIterator.CoCreateInstance(CLSID_CDeckLinkIterator)))
		BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("No Decklink drivers installed."));
		
	size_t n = 0;
	while(n < device_index && pDecklinkIterator->Next(&decklink) == S_OK){++n;}	

	if(n != device_index || !decklink)
		BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Decklink card not found.") << arg_name_info("device_index") << arg_value_info(boost::lexical_cast<std::string>(device_index)));
		
	return decklink;
}

template <typename T>
static std::wstring get_model_name(const T& device)
{	
	BSTR pModelName;
	device->GetModelName(&pModelName);
	return std::wstring(pModelName);
}

}