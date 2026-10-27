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
 * Author: Ole Kristensen, Den Frie Vilje ApS, ole@kristensen.name
 */

#include "ultralight.h"

#include "producer/ultralight_cg_proxy.h"
#include "producer/ultralight_producer.h"

#include <common/log.h>

namespace caspar { namespace ultralight {

void init(const core::module_dependencies& dependencies)
{
    // Register the Ultralight producer factory.
    // Templates with .html extension will be handled by Ultralight when this module is enabled.
    dependencies.producer_registry->register_producer_factory(
        L"Ultralight HTML Producer", create_ul_producer);

    // Register as a CG producer so CG ADD/PLAY/STOP/etc. commands work.
    dependencies.cg_registry->register_cg_producer(
        L"ultralight",
        {L".html"},
        [](const spl::shared_ptr<core::frame_producer>& producer) {
            return spl::make_shared<ultralight_cg_proxy>(producer);
        },
        [](const core::frame_producer_dependencies& dependencies, const std::wstring& filename) {
            return create_ul_cg_producer(dependencies, {filename});
        },
        false); // reusable_producer_instance = false

    CASPAR_LOG(info) << L"Ultralight HTML module initialized (synchronous renderer).";
}

void uninit()
{
    CASPAR_LOG(info) << L"Ultralight HTML module uninitialized.";
}

}} // namespace caspar::ultralight
