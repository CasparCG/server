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
 * Author: Krzysztof Zegzula, zegzulakrzysztof@gmail.com
 */
#pragma once

#include "../interop/Processing.NDI.Lib.h"
#include <string>

namespace caspar { namespace newtek { namespace ndi {
extern NDIlib_framesync_instance_t (*fs_create)(NDIlib_recv_instance_t p_receiver);
extern void (*fs_destroy)(NDIlib_framesync_instance_t p_instance);
extern void (*fs_capture_audio)(NDIlib_framesync_instance_t p_instance,
                                NDIlib_audio_frame_v2_t*    p_audio_data,
                                int                         sample_rate,
                                int                         no_channels,
                                int                         no_samples);

extern void (*fs_free_audio)(NDIlib_framesync_instance_t p_instance, NDIlib_audio_frame_v2_t* p_audio_data);

extern void (*fs_capture_video)(NDIlib_framesync_instance_t p_instance,
                                NDIlib_video_frame_v2_t*    p_video_data,
                                NDIlib_frame_format_type_e  field_type);

extern void (*fs_free_video)(NDIlib_framesync_instance_t p_instance, NDIlib_video_frame_v2_t* p_video_data);

const std::wstring&                    dll_name();
NDIlib_v3*                             load_library();
std::map<std::string, NDIlib_source_t> get_current_sources();
void                                   not_initialized();
void                                   not_installed();

std::vector<int32_t> audio_16_to_32(const short* audio_data, int size);
std::vector<float>   audio_32_to_32f(const int* audio_data, int size);

}}} // namespace caspar::newtek::ndi
