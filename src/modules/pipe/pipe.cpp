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
 * Author: Philip Starkey, https://github.com/philipstarkey
 * Author: Robert Nagy, ronag89@gmail.com
 */

#include "pipe.h"

#include "consumer/pipe_consumer.h"

//#include <core/consumer/frame_consumer.h>

namespace caspar { namespace pipe {

void init(core::module_dependencies dependencies)
{
    dependencies.consumer_registry->register_consumer_factory(L"Pipe Consumer", create_consumer);
    dependencies.consumer_registry->register_preconfigured_consumer_factory(L"pipe", create_preconfigured_consumer);
}

}} // namespace caspar::pipe
