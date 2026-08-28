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
#pragma once

namespace caspar::accelerator {
class accelerator;
}
namespace caspar::accelerator::ogl {
class device;
}

namespace caspar::core {
class stage;
class mixer;
class output;
class image_mixer;
struct video_format_desc;
class frame_factory;
struct frame_producer_and_attrs;
class frame_producer;
class frame_consumer;
class draw_frame;
class mutable_frame;
class const_frame;
class video_channel;
struct pixel_format_desc;
struct frame_transform;
struct frame_producer_dependencies;
struct module_dependencies;
class cg_producer_registry;
class frame_producer_registry;
class frame_consumer_registry;
class video_format_repository;
struct channel_info;
} // namespace caspar::core
