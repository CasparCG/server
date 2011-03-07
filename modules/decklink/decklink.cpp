#include "stdafx.h"

#include "decklink.h"
#include "util/util.h"

#include "consumer/decklink_consumer.h"
#include "producer/decklink_producer.h"

#include <core/consumer/frame_consumer.h>
#include <core/producer/frame_producer.h>

#include "interop/DeckLinkAPI_h.h"

#pragma warning(push)
#pragma warning(disable : 4996)

	#include <atlbase.h>

	#include <atlcom.h>
	#include <atlhost.h>

#pragma warning(push)

namespace caspar{

void init_decklink()
{
	core::register_consumer_factory(create_decklink_consumer);
	core::register_producer_factory(create_decklink_producer);
}

std::wstring get_decklink_version() 
{
	std::wstring version = L"Unknown";

	::CoInitialize(nullptr);
	try
	{
		CComPtr<IDeckLinkIterator> pDecklinkIterator;
		if(SUCCEEDED(pDecklinkIterator.CoCreateInstance(CLSID_CDeckLinkIterator)))		
			version = get_version(pDecklinkIterator);
	}
	catch(...){}
	::CoUninitialize();

	return version;
}

std::vector<std::wstring> get_decklink_device_list()
{
	std::vector<std::wstring> devices;

	::CoInitialize(nullptr);
	try
	{
		CComPtr<IDeckLinkIterator> pDecklinkIterator;
		if(SUCCEEDED(pDecklinkIterator.CoCreateInstance(CLSID_CDeckLinkIterator)))
		{		
			CComPtr<IDeckLink> decklink;
			for(int n = 1; pDecklinkIterator->Next(&decklink) == S_OK; ++n)	
			{
				BSTR model_name = L"Unknown";
				decklink->GetModelName(&model_name);
				devices.push_back(L"[" + boost::lexical_cast<std::wstring>(n) + L"] " + model_name);	
			}
		}
	}
	catch(...){}
	::CoUninitialize();

	return devices;
}

}