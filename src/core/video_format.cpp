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

#include "StdAfx.h"

#include "video_format.h"

#include <common/log.h>

#include <boost/algorithm/string.hpp>
#include <boost/range/adaptors.hpp>
#include <utility>

namespace caspar { namespace core {

const std::vector<video_format_desc> format_descs = {
    {video_format::pal, 2, 720, 576, 1024, 576, 50000, 1000, L"PAL", {1920 / 2}},
    {video_format::ntsc,
     2,
     720,
     486,
     720,
     540,
     60000,
     1001,
     L"NTSC",
     {801, 801, 801, 800, 801, 801, 801, 800, 801, 801}},
    {video_format::x576p2500, 1, 720, 576, 1024, 576, 25000, 1000, L"576p2500", {1920}},
    {video_format::x720p2398, 1, 1280, 720, 1280, 720, 24000, 1001, L"720p2398", {2002}},
    {video_format::x720p2400, 1, 1280, 720, 1280, 720, 24000, 1000, L"720p2400", {2000}},
    {video_format::x720p2500, 1, 1280, 720, 1280, 720, 25000, 1000, L"720p2500", {1920}},
    {video_format::x720p5000, 1, 1280, 720, 1280, 720, 50000, 1000, L"720p5000", {960}},
    {video_format::x720p2997, 1, 1280, 720, 1280, 720, 30000, 1001, L"720p2997", {1602, 1601, 1602, 1601, 1602}},
    {video_format::x720p5994, 1, 1280, 720, 1280, 720, 60000, 1001, L"720p5994", {801, 800, 801, 801, 801}},
    {video_format::x720p3000, 1, 1280, 720, 1280, 720, 30000, 1000, L"720p3000", {1600}},
    {video_format::x720p6000, 1, 1280, 720, 1280, 720, 60000, 1000, L"720p6000", {800}},
    {video_format::x1080p2398, 1, 1920, 1080, 1920, 1080, 24000, 1001, L"1080p2398", {2002}},
    {video_format::x1080p2400, 1, 1920, 1080, 1920, 1080, 24000, 1000, L"1080p2400", {2000}},
    {video_format::x1080i5000, 2, 1920, 1080, 1920, 1080, 50000, 1000, L"1080i5000", {1920 / 2}},
    {video_format::x1080i5994,
     2,
     1920,
     1080,
     1920,
     1080,
     60000,
     1001,
     L"1080i5994",
     {801, 801, 801, 800, 801, 801, 801, 800, 801, 801}},
    {video_format::x1080i6000, 2, 1920, 1080, 1920, 1080, 60000, 1000, L"1080i6000", {1600 / 2}},
    {video_format::x1080p2500, 1, 1920, 1080, 1920, 1080, 25000, 1000, L"1080p2500", {1920}},
    {video_format::x1080p2997, 1, 1920, 1080, 1920, 1080, 30000, 1001, L"1080p2997", {1602, 1601, 1602, 1601, 1602}},
    {video_format::x1080p3000, 1, 1920, 1080, 1920, 1080, 30000, 1000, L"1080p3000", {1600}},
    {video_format::x1080p5000, 1, 1920, 1080, 1920, 1080, 50000, 1000, L"1080p5000", {960}},
    {video_format::x1080p5994, 1, 1920, 1080, 1920, 1080, 60000, 1001, L"1080p5994", {801, 800, 801, 801, 801}},
    {video_format::x1080p6000, 1, 1920, 1080, 1920, 1080, 60000, 1000, L"1080p6000", {800}},
    {video_format::x1556p2398, 1, 2048, 1556, 2048, 1556, 24000, 1001, L"1556p2398", {2002}},
    {video_format::x1556p2400, 1, 2048, 1556, 2048, 1556, 24000, 1000, L"1556p2400", {2000}},
    {video_format::x1556p2500, 1, 2048, 1556, 2048, 1556, 25000, 1000, L"1556p2500", {1920}},
    {video_format::x2160p2398, 1, 3840, 2160, 3840, 2160, 24000, 1001, L"2160p2398", {2002}},
    {video_format::x2160p2400, 1, 3840, 2160, 3840, 2160, 24000, 1000, L"2160p2400", {2000}},
    {video_format::x2160p2500, 1, 3840, 2160, 3840, 2160, 25000, 1000, L"2160p2500", {1920}},
    {video_format::x2160p2997, 1, 3840, 2160, 3840, 2160, 30000, 1001, L"2160p2997", {1602, 1601, 1602, 1601, 1602}},
    {video_format::x2160p3000, 1, 3840, 2160, 3840, 2160, 30000, 1000, L"2160p3000", {1600}},
    {video_format::x2160p5000, 1, 3840, 2160, 3840, 2160, 50000, 1000, L"2160p5000", {960}},
    {video_format::x2160p5994, 1, 3840, 2160, 3840, 2160, 60000, 1001, L"2160p5994", {801, 800, 801, 801, 801}},
    {video_format::x2160p6000, 1, 3840, 2160, 3840, 2160, 60000, 1000, L"2160p6000", {800}},
    {video_format::invalid, 1, 0, 0, 0, 0, 1, 1, L"invalid", {1}}};

video_format_desc::video_format_desc(video_format     format,
                                     int              field_count,
                                     int              width,
                                     int              height,
                                     int              square_width,
                                     int              square_height,
                                     int              time_scale,
                                     int              duration,
                                     std::wstring     name,
                                     std::vector<int> audio_cadence)
    : format(format)
    , width(width)
    , height(height)
    , square_width(square_width)
    , square_height(square_height)
    , field_count(field_count)
    , fps(static_cast<double>(time_scale) / static_cast<double>(duration))
    , framerate(time_scale, duration)
    , time_scale(time_scale)
    , duration(duration)
    , size(width * height * 4)
    , name(std::move(name))
    , audio_sample_rate(48000)
    , audio_cadence(std::move(audio_cadence))
{
}

video_format_desc::video_format_desc(video_format format) { *this = format_descs.at(static_cast<int>(format)); }

video_format_desc::video_format_desc(const std::wstring& name)
{
    *this = video_format_desc(video_format::invalid);
    for (auto it = std::begin(format_descs); it != std::end(format_descs) - 1; ++it) {
        if (boost::iequals(it->name, name)) {
            *this = *it;
            break;
        }
    }
}

bool operator==(const video_format_desc& lhs, const video_format_desc& rhs) { return lhs.format == rhs.format; }

bool operator!=(const video_format_desc& lhs, const video_format_desc& rhs) { return !(lhs == rhs); }

std::wostream& operator<<(std::wostream& out, const video_format_desc& format_desc)
{
    out << format_desc.name.c_str();
    return out;
}

std::vector<int> find_audio_cadence(const boost::rational<int>& framerate, bool log_quiet)
{
    static std::map<boost::rational<int>, std::vector<int>> CADENCES_BY_FRAMERATE = [] {
        std::map<boost::rational<int>, std::vector<int>> result;

        for (core::video_format format : enum_constants<core::video_format>()) {
            core::video_format_desc desc(format);
            boost::rational<int>    format_rate(desc.time_scale, desc.duration);

            result.insert(std::make_pair(format_rate, desc.audio_cadence));
        }

        return result;
    }();

    auto exact_match = CADENCES_BY_FRAMERATE.find(framerate);

    if (exact_match != CADENCES_BY_FRAMERATE.end())
        return exact_match->second;

    boost::rational<int> closest_framerate_diff = std::numeric_limits<int>::max();
    boost::rational<int> closest_framerate      = 0;

    for (auto format_framerate : CADENCES_BY_FRAMERATE | boost::adaptors::map_keys) {
        auto diff = boost::abs(framerate - format_framerate);

        if (diff < closest_framerate_diff) {
            closest_framerate_diff = diff;
            closest_framerate      = format_framerate;
        }
    }

    if (log_quiet)
        CASPAR_LOG(debug) << "No exact audio cadence match found for framerate " << to_string(framerate)
                          << "\nClosest match is " << to_string(closest_framerate) << "\nwhich is a "
                          << to_string(closest_framerate_diff) << " difference.";
    else
        CASPAR_LOG(warning) << "No exact audio cadence match found for framerate " << to_string(framerate)
                            << "\nClosest match is " << to_string(closest_framerate) << "\nwhich is a "
                            << to_string(closest_framerate_diff) << " difference.";

    return CADENCES_BY_FRAMERATE[closest_framerate];
}

}} // namespace caspar::core
