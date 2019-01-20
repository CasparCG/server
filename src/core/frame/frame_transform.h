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

#pragma once

#include <common/tweener.h>

#include <core/mixer/image/blend_modes.h>

#include <boost/optional.hpp>

#include <array>

namespace caspar { namespace core {

struct chroma
{
    enum class legacy_type
    {
        none,
        green,
        blue
    };

    bool   enable                    = false;
    bool   show_mask                 = false;
    double target_hue                = 0.0;
    double hue_width                 = 0.0;
    double min_saturation            = 0.0;
    double min_brightness            = 0.0;
    double softness                  = 0.0;
    double spill_suppress            = 0.0;
    double spill_suppress_saturation = 1.0;
};

struct levels final
{
    double min_input  = 0.0;
    double max_input  = 1.0;
    double gamma      = 1.0;
    double min_output = 0.0;
    double max_output = 1.0;
};

struct corners final
{
    std::array<double, 2> ul = {0.0, 0.0};
    std::array<double, 2> ur = {1.0, 0.0};
    std::array<double, 2> lr = {1.0, 1.0};
    std::array<double, 2> ll = {0.0, 1.0};
};

struct rectangle final
{
    std::array<double, 2> ul = {0.0, 0.0};
    std::array<double, 2> lr = {1.0, 1.0};
};

struct image_transform final
{
    double opacity    = 1.0;
    double contrast   = 1.0;
    double brightness = 1.0;
    double saturation = 1.0;

    std::array<double, 2> anchor           = {0.0, 0.0};
    std::array<double, 2> fill_translation = {0.0, 0.0};
    std::array<double, 2> fill_scale       = {1.0, 1.0};
    std::array<double, 2> clip_translation = {0.0, 0.0};
    std::array<double, 2> clip_scale       = {1.0, 1.0};
    double                angle            = 0.0;
    rectangle             crop;
    corners               perspective;
    core::levels          levels;
    core::chroma          chroma;

    bool             is_key      = false;
    bool             invert      = false;
    bool             is_mix      = false;
    core::blend_mode blend_mode  = blend_mode::normal;
    int              layer_depth = 0;

    image_transform& operator*=(const image_transform& other);
    image_transform  operator*(const image_transform& other) const;

    static image_transform tween(double                 time,
                                 const image_transform& source,
                                 const image_transform& dest,
                                 double                 duration,
                                 const tweener&         tween);
};

bool operator==(const image_transform& lhs, const image_transform& rhs);
bool operator!=(const image_transform& lhs, const image_transform& rhs);

struct audio_transform final
{
    double volume = 1.0;

    audio_transform& operator*=(const audio_transform& other);
    audio_transform  operator*(const audio_transform& other) const;

    static audio_transform tween(double                 time,
                                 const audio_transform& source,
                                 const audio_transform& dest,
                                 double                 duration,
                                 const tweener&         tween);
};

bool operator==(const audio_transform& lhs, const audio_transform& rhs);
bool operator!=(const audio_transform& lhs, const audio_transform& rhs);

struct frame_transform final
{
  public:
    frame_transform();

    core::image_transform image_transform;
    core::audio_transform audio_transform;

    frame_transform& operator*=(const frame_transform& other);
    frame_transform  operator*(const frame_transform& other) const;

    static frame_transform tween(double                 time,
                                 const frame_transform& source,
                                 const frame_transform& dest,
                                 double                 duration,
                                 const tweener&         tween);
};

bool operator==(const frame_transform& lhs, const frame_transform& rhs);
bool operator!=(const frame_transform& lhs, const frame_transform& rhs);

class tweened_transform
{
    frame_transform source_;
    frame_transform dest_;
    int             duration_ = 0;
    int             time_     = 0;
    tweener         tweener_;

  public:
    tweened_transform() = default;

    tweened_transform(const frame_transform& source, const frame_transform& dest, int duration, const tweener& tween);

    const frame_transform& dest() const;

    frame_transform fetch();
    void            tick(int num);
};

boost::optional<chroma::legacy_type> get_chroma_mode(const std::wstring& str);

}} // namespace caspar::core
