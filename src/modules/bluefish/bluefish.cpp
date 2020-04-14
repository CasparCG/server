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
 *         James Wise,  james.wise@bluefish444.com
 */

#include "StdAfx.h"

#include "bluefish.h"

#include "consumer/bluefish_consumer.h"
#include "producer/bluefish_producer.h"

#include "util/blue_velvet.h"

#include <common/utf.h>

#include <core/consumer/frame_consumer.h>

namespace caspar { namespace bluefish {

std::wstring version()
{
    try {
        bvc_wrapper blue;
        return u16(blue.get_version());
    } catch (...) {
        return L"Not found";
    }
}

std::vector<std::wstring> device_list()
{
    std::vector<std::wstring> devices;

    try {
        bvc_wrapper blue;
        int         numCards = 0;
        blue.enumerate(&numCards);

        for (int n = 1; n < numCards + 1; n++) {
            blue.attach(n);
            devices.push_back(std::wstring(get_card_desc(blue, n)) + L" [" + std::to_wstring(n) + L"] " +
                              get_sdi_inputs(blue) + L"i" + get_sdi_outputs(blue) + L"o");
            blue.detach();
        }
    } catch (...) {
    }

    return devices;
}

void init(core::module_dependencies dependencies)
{
    try {
        bvc_wrapper blue;
        int         num_cards = 0;
        blue.enumerate(&num_cards);
    } catch (...) {
    }

    dependencies.consumer_registry->register_consumer_factory(L"Bluefish Consumer", create_consumer);
    dependencies.consumer_registry->register_preconfigured_consumer_factory(L"bluefish", create_preconfigured_consumer);
    dependencies.producer_registry->register_producer_factory(L"Bluefish Producer", create_producer);
}

}} // namespace caspar::bluefish
