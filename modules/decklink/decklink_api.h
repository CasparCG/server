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
* Author: Helge Norberg, helge.norberg@svt.se
*/

#pragma once

#include <common/except.h>

#if defined(_MSC_VER)

#include "interop/DeckLinkAPI.h"

#pragma warning(push)
#pragma warning(disable : 4996)

    #include <atlbase.h>

    #include <atlcom.h>
    #include <atlhost.h>

#pragma warning(pop)

namespace caspar { namespace decklink {

typedef BSTR String;
typedef unsigned int UINT32;

static void com_initialize()
{
	::CoInitialize(nullptr);
}

static void com_uninitialize()
{
	::CoUninitialize();
}

struct co_init
{
    co_init(){ ::CoInitialize(nullptr); }
    ~co_init(){ ::CoUninitialize(); }
};

template<typename T>
using com_ptr = CComPtr<T>;

// MSVC 2013 crashes when this alias template is instantiated
/*template<typename T>
using com_iface_ptr = CComQIPtr<T>;*/

template<typename T>
class com_iface_ptr : public CComQIPtr<T>
{
public:
	template<typename T2>
	com_iface_ptr(const com_ptr<T2>& lp)
		: CComQIPtr<T>(lp)
	{
	}
};

template<template<typename> class P, typename T>
static P<T> wrap_raw(T* ptr, bool already_referenced = false)
{
    if (already_referenced)
    {
        P<T> p;
        p.Attach(ptr);
        return p;
    }
    else
        return P<T>(ptr);
}

static com_ptr<IDeckLinkIterator> create_iterator()
{
    CComPtr<IDeckLinkIterator> pDecklinkIterator;
    if(FAILED(pDecklinkIterator.CoCreateInstance(CLSID_CDeckLinkIterator)))
        CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info("Decklink drivers not found."));
    return pDecklinkIterator;
}

template<typename I, typename T>
static com_iface_ptr<I> iface_cast(const com_ptr<T>& ptr)
{
    return com_iface_ptr<I>(ptr);
}

template<typename T>
T* get_raw(const CComPtr<T>& ptr)
{
	return ptr;
}

}}

#else

#include "linux_interop/DeckLinkAPI.h"
#include <memory>
#include <typeinfo>

namespace caspar { namespace decklink {

typedef const char* String;
typedef bool BOOL;
#define TRUE true
#define FALSE false
typedef uint32_t UINT32;

static void com_initialize()
{
}

static void com_uninitialize()
{
}

struct co_init
{
    co_init(){ }
    ~co_init(){ }
};

template<typename T>
using com_ptr = std::shared_ptr<T>;

template<typename T>
using com_iface_ptr = std::shared_ptr<T>;

template<template<typename> class P, typename T>
static P<T> wrap_raw(T* ptr, bool already_referenced = false)
{
    if (!already_referenced && ptr)
        ptr->AddRef();

	return P<T>(ptr, [](T* p)
	{
		if (p)
		{
			auto remaining_refs = p->Release();

			//CASPAR_LOG(debug) << "Remaining references for " << typeid(p).name() << " = " << remaining_refs;
		}
	});
}

static com_ptr<IDeckLinkIterator> create_iterator()
{
    IDeckLinkIterator* iterator = CreateDeckLinkIteratorInstance();

    if (iterator == nullptr)
        CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info("Decklink drivers not found."));

	return wrap_raw<com_ptr>(iterator, true);
}

template<typename T>    static REFIID iface_id() { return T::REFID; }
template<>              REFIID iface_id<IDeckLink>() { return IID_IDeckLink; }
template<>              REFIID iface_id<IDeckLinkOutput>() { return IID_IDeckLinkOutput; }
template<>              REFIID iface_id<IDeckLinkAPIInformation>() { return IID_IDeckLinkAPIInformation; }
template<>              REFIID iface_id<IDeckLinkConfiguration>() { return IID_IDeckLinkConfiguration; }
template<>              REFIID iface_id<IDeckLinkKeyer>() { return IID_IDeckLinkKeyer; }
template<>              REFIID iface_id<IDeckLinkAttributes>() { return IID_IDeckLinkAttributes; }
template<>              REFIID iface_id<IDeckLinkInput>() { return IID_IDeckLinkInput; }

template<typename I, typename T>
static com_iface_ptr<I> iface_cast(com_ptr<T> ptr)
{
    I* iface;
    ptr->QueryInterface(iface_id<I>(), reinterpret_cast<void**>(&iface));

	return wrap_raw<com_iface_ptr>(iface, true);
}

template<typename T>
T* get_raw(const std::shared_ptr<T>& ptr)
{
	return ptr.get();
}

}}

#endif
