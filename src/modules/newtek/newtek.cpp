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

#include "newtek.h"

#include "consumer/newtek_ndi_consumer.h"
#include "producer/newtek_ndi_producer.h"

#include "util/ndi.h"

#include <core/consumer/frame_consumer.h>
#include <protocol/amcp/amcp_command_repository_wrapper.h>

#include <boost/property_tree/ptree.hpp>
#include <common/env.h>

namespace caspar { namespace newtek {

void init(const core::module_dependencies& dependencies)
{
    try {
        dependencies.consumer_registry->register_consumer_factory(L"NDI Consumer", create_ndi_consumer);
        dependencies.consumer_registry->register_preconfigured_consumer_factory(L"ndi",
                                                                                create_preconfigured_ndi_consumer);

        dependencies.producer_registry->register_producer_factory(L"NDI Producer", create_ndi_producer);

        dependencies.command_repository->register_command(L"Query Commands", L"NDI LIST", ndi::list_command, 0);

        bool autoload = caspar::env::properties().get(L"configuration.ndi.auto-load", false);
        if (autoload)
            ndi::load_library();

    } catch (...) {
    }
}

}} // namespace caspar::newtek
