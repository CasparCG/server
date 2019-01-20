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

#include <comutil.h>

#include "interop/DeckLinkAPI.h"

#pragma warning(push)
#pragma warning(disable : 4996)

#include <atlbase.h>

#pragma warning(pop)

namespace caspar { namespace decklink {

using String = BSTR;
using UINT32 = unsigned int;

static std::wstring to_string(String bstr_string)
{
    std::wstring result = bstr_t(bstr_string, false);
    return result;
}

static void com_initialize() { ::CoInitialize(nullptr); }

static void com_uninitialize() { ::CoUninitialize(); }

struct co_init
{
    co_init() { ::CoInitialize(nullptr); }
    ~co_init() { ::CoUninitialize(); }
};

template <typename T>
using com_ptr = CComPtr<T>;

template <typename T>
using com_iface_ptr = CComQIPtr<T>;

template <template <typename> class P, typename T>
static P<T> wrap_raw(T* ptr, bool already_referenced = false)
{
    if (already_referenced) {
        P<T> p;
        p.Attach(ptr);
        return p;
    }
    return P<T>(ptr);
}

static com_ptr<IDeckLinkIterator> create_iterator()
{
    CComPtr<IDeckLinkIterator> pDecklinkIterator;
    if (FAILED(pDecklinkIterator.CoCreateInstance(CLSID_CDeckLinkIterator)))
        CASPAR_THROW_EXCEPTION(not_supported() << msg_info("Decklink drivers not found."));
    return pDecklinkIterator;
}

template <typename I, typename T>
static com_iface_ptr<I> iface_cast(const com_ptr<T>& ptr, bool optional = false)
{
    com_iface_ptr<I> result = ptr;

    if (!optional && !result)
        CASPAR_THROW_EXCEPTION(not_supported()
                               << msg_info(std::string("Could not cast from ") + typeid(T).name() + " to " +
                                           typeid(I).name() + ". This is probably due to old Decklink drivers."));

    return result;
}

template <typename T>
T* get_raw(const CComPtr<T>& ptr)
{
    return ptr;
}

}} // namespace caspar::decklink

#else

#include "linux_interop/DeckLinkAPI.h"
#include "linux_interop/DeckLinkAPIConfiguration_v10_2.h"
#include <memory>
#include <typeinfo>

namespace caspar { namespace decklink {

using String = const char*;
using BOOL   = bool;
#define TRUE true
#define FALSE false
using UINT32 = uint32_t;

static std::wstring to_string(String utf16_string) { return u16(utf16_string); }

static void com_initialize() {}

static void com_uninitialize() {}

struct co_init
{
    co_init() {}
    ~co_init() {}
};

template <typename T>
using com_ptr = std::shared_ptr<T>;

template <typename T>
using com_iface_ptr = std::shared_ptr<T>;

template <template <typename> class P, typename T>
static P<T> wrap_raw(T* ptr, bool already_referenced = false)
{
    if (!already_referenced && ptr)
        ptr->AddRef();

    return P<T>(ptr, [](T* p) {
        if (p) {
            auto remaining_refs = p->Release();

            // CASPAR_LOG(debug) << "Remaining references for " << typeid(p).name() << " = " << remaining_refs;
        }
    });
}

static com_ptr<IDeckLinkIterator> create_iterator()
{
    IDeckLinkIterator* iterator = CreateDeckLinkIteratorInstance();

    if (iterator == nullptr)
        CASPAR_THROW_EXCEPTION(not_supported() << msg_info("Decklink drivers not found."));

    return wrap_raw<com_ptr>(iterator, true);
}

template <typename T>
static REFIID iface_id()
{
    return T::REFID;
}
template <>
REFIID iface_id<IDeckLink>()
{
    return IID_IDeckLink;
}
template <>
REFIID iface_id<IDeckLinkOutput>()
{
    return IID_IDeckLinkOutput;
}
template <>
REFIID iface_id<IDeckLinkAPIInformation>()
{
    return IID_IDeckLinkAPIInformation;
}
template <>
REFIID iface_id<IDeckLinkConfiguration>()
{
    return IID_IDeckLinkConfiguration;
}
template <>
REFIID iface_id<IDeckLinkConfiguration_v10_2>()
{
    return IID_IDeckLinkConfiguration_v10_2;
}
template <>
REFIID iface_id<IDeckLinkKeyer>()
{
    return IID_IDeckLinkKeyer;
}
template <>
REFIID iface_id<IDeckLinkAttributes>()
{
    return IID_IDeckLinkAttributes;
}
template <>
REFIID iface_id<IDeckLinkInput>()
{
    return IID_IDeckLinkInput;
}

template <typename I, typename T>
static com_iface_ptr<I> iface_cast(com_ptr<T> ptr, bool optional = false)
{
    I* iface;
    ptr->QueryInterface(iface_id<I>(), reinterpret_cast<void**>(&iface));

    return wrap_raw<com_iface_ptr>(iface, true);
}

template <typename T>
T* get_raw(const std::shared_ptr<T>& ptr)
{
    return ptr.get();
}

}} // namespace caspar::decklink

#endif
