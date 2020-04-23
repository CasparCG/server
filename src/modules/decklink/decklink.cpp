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

#include "StdAfx.h"

#include "decklink.h"
#include "util/util.h"

#include "consumer/decklink_consumer.h"
#include "producer/decklink_producer.h"

#include <core/consumer/frame_consumer.h>
#include <core/producer/frame_producer.h>

#include "decklink_api.h"

namespace caspar { namespace decklink {

std::wstring get_version()
{
    std::wstring ver = L"Not found";

    struct co_init init;

    try {
        ver = decklink::version(create_iterator());
    } catch (...) {
    }

    return ver;
}

std::vector<std::wstring> device_list()
{
    std::vector<std::wstring> devices;

    struct co_init init;

    try {
        auto       pDecklinkIterator = create_iterator();
        IDeckLink* decklink;
        for (int n = 1; pDecklinkIterator->Next(&decklink) == S_OK; ++n) {
            devices.push_back(get_model_name(decklink) + L" [" + std::to_wstring(n) + L"]");
            decklink->Release();
        }
    } catch (...) {
    }

    return devices;
}

void init(core::module_dependencies dependencies)
{
    dependencies.consumer_registry->register_consumer_factory(L"Decklink Consumer", create_consumer);
    dependencies.consumer_registry->register_preconfigured_consumer_factory(L"decklink", create_preconfigured_consumer);
    dependencies.producer_registry->register_producer_factory(L"Decklink Producer", create_producer);
}

}} // namespace caspar::decklink
