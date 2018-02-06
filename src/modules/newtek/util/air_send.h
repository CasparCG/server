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
 * Author: Helge Norberg, helge.norberg@svt.se
 */
#pragma once

#include <string>

namespace caspar { namespace newtek { namespace airsend {

const std::wstring& dll_name();
bool                is_available();

extern void* (*create)(const int   width,
                       const int   height,
                       const int   timescale,
                       const int   duration,
                       const bool  progressive,
                       const float aspect_ratio,
                       const bool  audio_enabled,
                       const int   num_channels,
                       const int   sample_rate);
extern void (*destroy)(void* instance);
extern bool (*add_audio)(void* instance, const short* samples, const int num_samples);
extern bool (*add_frame_bgra)(void* instance, const unsigned char* data);

}}} // namespace caspar::newtek::airsend
