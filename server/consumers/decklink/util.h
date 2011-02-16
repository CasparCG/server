#pragma once

#include "..\..\frame\Frame.h"

#include "DeckLinkAPI_h.h"

namespace caspar {
	
static BMDDisplayMode GetDecklinkVideoFormat(FrameFormat fmt) 
{	
	switch(fmt)
	{
	case FFormatPAL:		return bmdModePAL;
	case FFormatNTSC:		return bmdModeNTSC;
	case FFormat576p2500:	return (BMDDisplayMode)ULONG_MAX;
	case FFormat720p5000:	return bmdModeHD720p50;
	case FFormat720p5994:	return bmdModeHD720p5994;
	case FFormat720p6000:	return bmdModeHD720p60;
	case FFormat1080p2397:	return bmdModeHD1080p2398;
	case FFormat1080p2400:	return bmdModeHD1080p24;
	case FFormat1080i5000:	return bmdModeHD1080i50;
	case FFormat1080i5994:	return bmdModeHD1080i5994;
	case FFormat1080i6000:	return bmdModeHD1080i6000;
	case FFormat1080p2500:	return bmdModeHD1080p25;
	case FFormat1080p2997:	return bmdModeHD1080p2997;
	case FFormat1080p3000:	return bmdModeHD1080p30;
	case FFormat1080p5000:	return bmdModeHD1080p50;
	default:				return (BMDDisplayMode)ULONG_MAX;
	}
}
	
static IDeckLinkDisplayMode* get_display_mode(IDeckLinkOutput* output, BMDDisplayMode format)
{
	IDeckLinkDisplayModeIterator*		iterator;
	IDeckLinkDisplayMode*				mode;
	
	if(FAILED(output->GetDisplayModeIterator(&iterator)))
		return nullptr;

	while(SUCCEEDED(iterator->Next(&mode)) && mode != nullptr)
	{	
		if(mode->GetDisplayMode() == format)
			return mode;
	}
	iterator->Release();
	return nullptr;
}

static IDeckLinkDisplayMode* get_display_mode(IDeckLinkOutput* output, FrameFormat fmt)
{
	return get_display_mode(output, GetDecklinkVideoFormat(fmt));
}

template<typename T>
static std::wstring get_version(T& deckLinkIterator)
{
	IDeckLinkAPIInformation* deckLinkAPIInformation;
	if (FAILED(deckLinkIterator->QueryInterface(IID_IDeckLinkAPIInformation, (void**)&deckLinkAPIInformation)))
		return L"Unknown";
	
	BSTR ver;		
	deckLinkAPIInformation->GetString(BMDDeckLinkAPIVersion, &ver);
	deckLinkAPIInformation->Release();	
		
	return ver;					
}

}