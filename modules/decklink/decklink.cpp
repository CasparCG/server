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

namespace caspar { namespace decklink {

void init()
{
	struct co_init
	{
		co_init(){::CoInitialize(nullptr);}
		~co_init(){::CoUninitialize();}
	} init;
	
	CComPtr<IDeckLinkIterator> pDecklinkIterator;
	if(FAILED(pDecklinkIterator.CoCreateInstance(CLSID_CDeckLinkIterator)))		
		return;
		
	core::register_consumer_factory([](const std::vector<std::wstring>& params){return create_consumer(params);});
	core::register_producer_factory(create_producer);
}

std::wstring version() 
{
	std::wstring version = L"Not found";
	
	struct co_init
	{
		co_init(){::CoInitialize(nullptr);}
		~co_init(){::CoUninitialize();}
	} init;

	try
	{
		CComPtr<IDeckLinkIterator> pDecklinkIterator;
		if(SUCCEEDED(pDecklinkIterator.CoCreateInstance(CLSID_CDeckLinkIterator)))		
			version = decklink::version(pDecklinkIterator);
	}
	catch(...){}

	return version;
}

std::vector<std::wstring> device_list()
{
	std::vector<std::wstring> devices;
	
	struct co_init
	{
		co_init(){::CoInitialize(nullptr);}
		~co_init(){::CoUninitialize();}
	} init;

	try
	{
		CComPtr<IDeckLinkIterator> pDecklinkIterator;
		if(SUCCEEDED(pDecklinkIterator.CoCreateInstance(CLSID_CDeckLinkIterator)))
		{		
			IDeckLink* decklink;
			for(int n = 1; pDecklinkIterator->Next(&decklink) == S_OK; ++n)	
			{
				BSTR model_name = L"Unknown";
				decklink->GetModelName(&model_name);
				decklink->Release();
				devices.push_back(std::wstring(model_name) + L" [" + boost::lexical_cast<std::wstring>(n) + L"]");	
			}
		}
	}
	catch(...){}

	return devices;
}

}}