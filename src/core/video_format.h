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

#include <cstddef>
#include <string>
#include <vector>

#include <common/memory.h>

#include <boost/rational.hpp>

namespace caspar { namespace core {

enum class video_field
{
    progressive,
    a,
    b,
};

enum class video_format
{
    pal,
    ntsc,
    x576p2500,
    x720p2398,
    x720p2400,
    x720p2500,
    x720p2997,
    x720p3000,
    x720p5000,
    x720p5994,
    x720p6000,
    x1080i5000,
    x1080i5994,
    x1080i6000,
    x1080p2398,
    x1080p2400,
    x1080p2500,
    x1080p2997,
    x1080p3000,
    x1080p5000,
    x1080p5994,
    x1080p6000,
    x1556p2398,
    x1556p2400,
    x1556p2500,
    x2160p2398,
    x2160p2400,
    x2160p2500,
    x2160p2997,
    x2160p3000,
    x2160p5000,
    x2160p5994,
    x2160p6000,
    invalid,
    custom,
    count
};

struct video_format_desc final
{
    video_format format{video_format::invalid};

    int                  width;
    int                  height;
    int                  square_width;
    int                  square_height;
    int                  field_count;
    double               hz;  // actual tickrate of the channel, e.g. i50 = 25 hz, p50 = 50 hz
    double               fps; // actual fieldrate, e.g. i50 = 50 fps, p50 = 50 fps
    boost::rational<int> framerate;
    int                  time_scale;
    int                  duration;
    std::size_t          size; // frame size in bytes
    std::wstring         name; // name of output format

    int              audio_channels = 8;
    int              audio_sample_rate;
    std::vector<int> audio_cadence; // rotating optimal number of samples per frame

    video_format_desc(video_format     format,
                      int              field_count,
                      int              width,
                      int              height,
                      int              square_width,
                      int              square_height,
                      int              time_scale,
                      int              duration,
                      std::wstring     name,
                      std::vector<int> audio_cadence);

    video_format_desc();
};

bool operator==(const video_format_desc& rhs, const video_format_desc& lhs);
bool operator!=(const video_format_desc& rhs, const video_format_desc& lhs);

std::wostream& operator<<(std::wostream& out, const video_format_desc& format_desc);

class video_format_repository
{
  public:
    explicit video_format_repository();

    video_format_desc find(const std::wstring& name) const;
    video_format_desc find_format(const video_format& format) const;
    void              store(const video_format_desc& format);

    std::size_t      get_max_video_format_size() const;

    static video_format_desc invalid();

  private:
    struct impl;
    spl::shared_ptr<impl> impl_;
};

}} // namespace caspar::core
