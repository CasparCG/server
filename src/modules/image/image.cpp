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

#include "image.h"

#include "consumer/image_consumer.h"
#include "producer/image_producer.h"
#include "producer/image_scroll_producer.h"

#include <common/utf.h>

namespace caspar { namespace image {

void init(const core::module_dependencies& dependencies)
{
    dependencies.producer_registry->register_producer_factory(L"Image Scroll Producer", create_scroll_producer);
    dependencies.producer_registry->register_producer_factory(L"Image Producer", create_producer);
    dependencies.consumer_registry->register_consumer_factory(L"Image Consumer", create_consumer);
}

}} // namespace caspar::image
