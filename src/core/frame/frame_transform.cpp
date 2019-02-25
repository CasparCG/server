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
#include "frame_transform.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/range/algorithm/equal.hpp>

#include <cmath>

namespace caspar { namespace core {

// image_transform

template <typename Rect>
void transform_rect(Rect& self, const Rect& other)
{
    self.ul[0] += other.ul[0];
    self.ul[1] += other.ul[1];
    self.lr[0] *= other.lr[0];
    self.lr[1] *= other.lr[1];
}

void transform_corners(corners& self, const corners& other)
{
    transform_rect(self, other);

    self.ur[0] *= other.ur[0];
    self.ur[1] += other.ur[1];
    self.ll[0] += other.ll[0];
    self.ll[1] *= other.ll[1];

    // TODO (fix) figure out the math to compose perspective transforms correctly.
}

image_transform& image_transform::operator*=(const image_transform& other)
{
    opacity *= other.opacity;
    brightness *= other.brightness;
    contrast *= other.contrast;
    saturation *= other.saturation;

    // TODO (fix)
    auto aspect_ratio = 1.0;

    std::array<double, 2> rotated;

    auto orig_x = other.fill_translation[0];
    auto orig_y = other.fill_translation[1] / aspect_ratio;
    rotated[0]  = orig_x * std::cos(angle) - orig_y * std::sin(angle);
    rotated[1]  = orig_x * std::sin(angle) + orig_y * std::cos(angle);
    rotated[1] *= aspect_ratio;

    anchor[0] += other.anchor[0] * fill_scale[0];
    anchor[1] += other.anchor[1] * fill_scale[1];
    fill_translation[0] += rotated[0] * fill_scale[0];
    fill_translation[1] += rotated[1] * fill_scale[1];
    fill_scale[0] *= other.fill_scale[0];
    fill_scale[1] *= other.fill_scale[1];
    clip_translation[0] += other.clip_translation[0] * clip_scale[0];
    clip_translation[1] += other.clip_translation[1] * clip_scale[1];
    clip_scale[0] *= other.clip_scale[0];
    clip_scale[1] *= other.clip_scale[1];
    angle += other.angle;

    transform_rect(crop, other.crop);
    transform_corners(perspective, other.perspective);

    levels.min_input  = std::max(levels.min_input, other.levels.min_input);
    levels.max_input  = std::min(levels.max_input, other.levels.max_input);
    levels.min_output = std::max(levels.min_output, other.levels.min_output);
    levels.max_output = std::min(levels.max_output, other.levels.max_output);
    levels.gamma *= other.levels.gamma;
    chroma.enable |= other.chroma.enable;
    chroma.show_mask |= other.chroma.show_mask;
    chroma.target_hue     = std::max(other.chroma.target_hue, chroma.target_hue);
    chroma.min_saturation = std::max(other.chroma.min_saturation, chroma.min_saturation);
    chroma.min_brightness = std::max(other.chroma.min_brightness, chroma.min_brightness);
    chroma.hue_width      = std::max(other.chroma.hue_width, chroma.hue_width);
    chroma.softness       = std::max(other.chroma.softness, chroma.softness);
    chroma.spill_suppress = std::max(other.chroma.spill_suppress, chroma.spill_suppress);
    chroma.spill_suppress_saturation =
        std::min(other.chroma.spill_suppress_saturation, chroma.spill_suppress_saturation);
    is_key |= other.is_key;
    invert |= other.invert;
    is_mix |= other.is_mix;
    blend_mode = std::max(blend_mode, other.blend_mode);
    layer_depth += other.layer_depth;

    return *this;
}

image_transform image_transform::operator*(const image_transform& other) const
{
    return image_transform(*this) *= other;
}

double do_tween(double time, double source, double dest, double duration, const tweener& tween)
{
    return tween(time, source, dest - source, duration);
};

template <typename Rect>
void do_tween_rectangle(const Rect&    source,
                        const Rect&    dest,
                        Rect&          out,
                        double         time,
                        double         duration,
                        const tweener& tweener)
{
    out.ul[0] = do_tween(time, source.ul[0], dest.ul[0], duration, tweener);
    out.ul[1] = do_tween(time, source.ul[1], dest.ul[1], duration, tweener);
    out.lr[0] = do_tween(time, source.lr[0], dest.lr[0], duration, tweener);
    out.lr[1] = do_tween(time, source.lr[1], dest.lr[1], duration, tweener);
}

void do_tween_corners(const corners& source,
                      const corners& dest,
                      corners&       out,
                      double         time,
                      double         duration,
                      const tweener& tweener)
{
    do_tween_rectangle(source, dest, out, time, duration, tweener);

    out.ur[0] = do_tween(time, source.ur[0], dest.ur[0], duration, tweener);
    out.ur[1] = do_tween(time, source.ur[1], dest.ur[1], duration, tweener);
    out.ll[0] = do_tween(time, source.ll[0], dest.ll[0], duration, tweener);
    out.ll[1] = do_tween(time, source.ll[1], dest.ll[1], duration, tweener);
};

image_transform image_transform::tween(double                 time,
                                       const image_transform& source,
                                       const image_transform& dest,
                                       double                 duration,
                                       const tweener&         tween)
{
    image_transform result;

    result.brightness          = do_tween(time, source.brightness, dest.brightness, duration, tween);
    result.contrast            = do_tween(time, source.contrast, dest.contrast, duration, tween);
    result.saturation          = do_tween(time, source.saturation, dest.saturation, duration, tween);
    result.opacity             = do_tween(time, source.opacity, dest.opacity, duration, tween);
    result.anchor[0]           = do_tween(time, source.anchor[0], dest.anchor[0], duration, tween);
    result.anchor[1]           = do_tween(time, source.anchor[1], dest.anchor[1], duration, tween);
    result.fill_translation[0] = do_tween(time, source.fill_translation[0], dest.fill_translation[0], duration, tween);
    result.fill_translation[1] = do_tween(time, source.fill_translation[1], dest.fill_translation[1], duration, tween);
    result.fill_scale[0]       = do_tween(time, source.fill_scale[0], dest.fill_scale[0], duration, tween);
    result.fill_scale[1]       = do_tween(time, source.fill_scale[1], dest.fill_scale[1], duration, tween);
    result.clip_translation[0] = do_tween(time, source.clip_translation[0], dest.clip_translation[0], duration, tween);
    result.clip_translation[1] = do_tween(time, source.clip_translation[1], dest.clip_translation[1], duration, tween);
    result.clip_scale[0]       = do_tween(time, source.clip_scale[0], dest.clip_scale[0], duration, tween);
    result.clip_scale[1]       = do_tween(time, source.clip_scale[1], dest.clip_scale[1], duration, tween);
    result.angle               = do_tween(time, source.angle, dest.angle, duration, tween);
    result.levels.max_input    = do_tween(time, source.levels.max_input, dest.levels.max_input, duration, tween);
    result.levels.min_input    = do_tween(time, source.levels.min_input, dest.levels.min_input, duration, tween);
    result.levels.max_output   = do_tween(time, source.levels.max_output, dest.levels.max_output, duration, tween);
    result.levels.min_output   = do_tween(time, source.levels.min_output, dest.levels.min_output, duration, tween);
    result.levels.gamma        = do_tween(time, source.levels.gamma, dest.levels.gamma, duration, tween);
    result.chroma.target_hue   = do_tween(time, source.chroma.target_hue, dest.chroma.target_hue, duration, tween);
    result.chroma.hue_width    = do_tween(time, source.chroma.hue_width, dest.chroma.hue_width, duration, tween);
    result.chroma.min_saturation =
        do_tween(time, source.chroma.min_saturation, dest.chroma.min_saturation, duration, tween);
    result.chroma.min_brightness =
        do_tween(time, source.chroma.min_brightness, dest.chroma.min_brightness, duration, tween);
    result.chroma.softness = do_tween(time, source.chroma.softness, dest.chroma.softness, duration, tween);
    result.chroma.spill_suppress =
        do_tween(time, source.chroma.spill_suppress, dest.chroma.spill_suppress, duration, tween);
    result.chroma.spill_suppress_saturation =
        do_tween(time, source.chroma.spill_suppress_saturation, dest.chroma.spill_suppress_saturation, duration, tween);
    result.chroma.enable    = dest.chroma.enable;
    result.chroma.show_mask = dest.chroma.show_mask;
    result.is_key           = source.is_key || dest.is_key;
    result.invert           = source.invert || dest.invert;
    result.is_mix           = source.is_mix || dest.is_mix;
    result.blend_mode       = std::max(source.blend_mode, dest.blend_mode);
    result.layer_depth      = dest.layer_depth;

    do_tween_rectangle(source.crop, dest.crop, result.crop, time, duration, tween);
    do_tween_corners(source.perspective, dest.perspective, result.perspective, time, duration, tween);

    return result;
}

bool eq(double lhs, double rhs) { return std::abs(lhs - rhs) < 5e-8; };

bool operator==(const corners& lhs, const corners& rhs)
{
    return boost::range::equal(lhs.ul, rhs.ul, eq) && boost::range::equal(lhs.ur, rhs.ur, eq) &&
           boost::range::equal(lhs.lr, rhs.lr, eq) && boost::range::equal(lhs.ll, rhs.ll, eq);
}

bool operator==(const rectangle& lhs, const rectangle& rhs)
{
    return boost::range::equal(lhs.ul, rhs.ul, eq) && boost::range::equal(lhs.lr, rhs.lr, eq);
}

bool operator==(const image_transform& lhs, const image_transform& rhs)
{
    return eq(lhs.opacity, rhs.opacity) && eq(lhs.contrast, rhs.contrast) && eq(lhs.brightness, rhs.brightness) &&
           eq(lhs.saturation, rhs.saturation) && boost::range::equal(lhs.anchor, rhs.anchor, eq) &&
           boost::range::equal(lhs.fill_translation, rhs.fill_translation, eq) &&
           boost::range::equal(lhs.fill_scale, rhs.fill_scale, eq) &&
           boost::range::equal(lhs.clip_translation, rhs.clip_translation, eq) &&
           boost::range::equal(lhs.clip_scale, rhs.clip_scale, eq) && eq(lhs.angle, rhs.angle) &&
           lhs.is_key == rhs.is_key && lhs.invert == rhs.invert && lhs.is_mix == rhs.is_mix &&
           lhs.blend_mode == rhs.blend_mode && lhs.layer_depth == rhs.layer_depth &&
           lhs.chroma.enable == rhs.chroma.enable && lhs.chroma.show_mask == rhs.chroma.show_mask &&
           eq(lhs.chroma.target_hue, rhs.chroma.target_hue) && eq(lhs.chroma.hue_width, rhs.chroma.hue_width) &&
           eq(lhs.chroma.min_saturation, rhs.chroma.min_saturation) &&
           eq(lhs.chroma.min_brightness, rhs.chroma.min_brightness) && eq(lhs.chroma.softness, rhs.chroma.softness) &&
           eq(lhs.chroma.spill_suppress, rhs.chroma.spill_suppress) &&
           eq(lhs.chroma.spill_suppress_saturation, rhs.chroma.spill_suppress_saturation) && lhs.crop == rhs.crop &&
           lhs.perspective == rhs.perspective;
}

bool operator!=(const image_transform& lhs, const image_transform& rhs) { return !(lhs == rhs); }

// audio_transform

audio_transform& audio_transform::operator*=(const audio_transform& other)
{
    volume *= other.volume;
    return *this;
}

audio_transform audio_transform::operator*(const audio_transform& other) const
{
    return audio_transform(*this) *= other;
}

audio_transform audio_transform::tween(double                 time,
                                       const audio_transform& source,
                                       const audio_transform& dest,
                                       double                 duration,
                                       const tweener&         tween)
{
    audio_transform result;
    result.volume = do_tween(time, source.volume, dest.volume, duration, tween);

    return result;
}

bool operator==(const audio_transform& lhs, const audio_transform& rhs) { return eq(lhs.volume, rhs.volume); }

bool operator!=(const audio_transform& lhs, const audio_transform& rhs) { return !(lhs == rhs); }

// frame_transform
frame_transform::frame_transform() {}

frame_transform& frame_transform::operator*=(const frame_transform& other)
{
    image_transform *= other.image_transform;
    audio_transform *= other.audio_transform;
    return *this;
}

frame_transform frame_transform::operator*(const frame_transform& other) const
{
    return frame_transform(*this) *= other;
}

frame_transform frame_transform::tween(double                 time,
                                       const frame_transform& source,
                                       const frame_transform& dest,
                                       double                 duration,
                                       const tweener&         tween)
{
    frame_transform result;
    result.image_transform =
        image_transform::tween(time, source.image_transform, dest.image_transform, duration, tween);
    result.audio_transform =
        audio_transform::tween(time, source.audio_transform, dest.audio_transform, duration, tween);
    return result;
}

bool operator==(const frame_transform& lhs, const frame_transform& rhs)
{
    return lhs.image_transform == rhs.image_transform && lhs.audio_transform == rhs.audio_transform;
}

bool operator!=(const frame_transform& lhs, const frame_transform& rhs) { return !(lhs == rhs); }

tweened_transform::tweened_transform(const frame_transform& source,
                                     const frame_transform& dest,
                                     int                    duration,
                                     const tweener&         tween)
    : source_(source)
    , dest_(dest)
    , duration_(duration)
    , tweener_(tween)
{
}

const frame_transform& tweened_transform::dest() const { return dest_; }

frame_transform tweened_transform::fetch()
{
    return time_ == duration_
               ? dest_
               : frame_transform::tween(
                     static_cast<double>(time_), source_, dest_, static_cast<double>(duration_), tweener_);
}

void tweened_transform::tick(int num) { time_ = std::min(time_ + num, duration_); }

boost::optional<chroma::legacy_type> get_chroma_mode(const std::wstring& str)
{
    if (boost::iequals(str, L"none")) {
        return chroma::legacy_type::none;
    }
    if (boost::iequals(str, L"green")) {
        return chroma::legacy_type::green;
    }
    if (boost::iequals(str, L"blue")) {
        return chroma::legacy_type::blue;
    } else {
        return boost::none;
    }
}

}} // namespace caspar::core
