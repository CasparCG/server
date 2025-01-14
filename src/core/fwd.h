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

namespace caspar::accelerator { class accelerator; }
namespace caspar::accelerator::ogl { class device; }

namespace caspar::core { class stage; }
namespace caspar::core { class mixer; }
namespace caspar::core { class output; }
namespace caspar::core { class image_mixer; }
namespace caspar::core { struct video_format_desc; }
namespace caspar::core { class frame_factory; }
namespace caspar::core { class frame_producer; }
namespace caspar::core { class frame_consumer; }
namespace caspar::core { class draw_frame; }
namespace caspar::core { class mutable_frame; }
namespace caspar::core { class const_frame; }
namespace caspar::core { class video_channel; }
namespace caspar::core { struct pixel_format_desc; }
namespace caspar::core { class cg_producer_registry; }
namespace caspar::core { struct frame_transform; }
namespace caspar::core { struct frame_producer_dependencies; }
namespace caspar::core { struct module_dependencies; }
namespace caspar::core { class frame_producer_registry; }
namespace caspar::core { class video_format_repository; }
