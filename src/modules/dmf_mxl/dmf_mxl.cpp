/*
 * Copyright (c) 2026 Sveriges Television AB <info@casparcg.com>
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
 * Author: Mint de Wit mint@araminta.dev
 */

#include "dmf_mxl.h"

#include "consumer/dmf_mxl_consumer.h"
#include "producer/dmf_mxl_producer.h"

#define WIN32_LEAN_AND_MEAN

#include <core/consumer/frame_consumer.h>

#include <common/utf.h>

namespace caspar { namespace dmf_mxl {

void init(const core::module_dependencies& dependencies)
{
    dependencies.producer_registry->register_producer_factory(L"MXL Producer", create_mxl_producer);

    dependencies.consumer_registry->register_consumer_factory(L"MXL Consumer", create_mxl_consumer);
    dependencies.consumer_registry->register_preconfigured_consumer_factory(L"mxl", create_preconfigured_mxl_consumer);
}

}} // namespace caspar::dmf_mxl
