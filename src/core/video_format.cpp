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

struct video_format_repository::impl
{
  private:
    std::map<std::wstring, video_format_desc> formats_;

  public:
    impl()
        : formats_()
    {
        const std::vector<video_format_desc> default_formats = {
            {video_format::pal, 2, 720, 576, 1024, 576, 25000, 1000, L"PAL", {1920 / 2}},
            {video_format::ntsc,
             2,
             720,
             486,
             720,
             540,
             30000,
             1001,
             L"NTSC",
             {801, 801, 801, 800, 801, 801, 801, 800, 801, 801}},
            {video_format::x576p2500, 1, 720, 576, 1024, 576, 25000, 1000, L"576p2500", {1920}},
            {video_format::x720p2398, 1, 1280, 720, 1280, 720, 24000, 1001, L"720p2398", {2002}},
            {video_format::x720p2400, 1, 1280, 720, 1280, 720, 24000, 1000, L"720p2400", {2000}},
            {video_format::x720p2500, 1, 1280, 720, 1280, 720, 25000, 1000, L"720p2500", {1920}},
            {video_format::x720p2997,
             1,
             1280,
             720,
             1280,
             720,
             30000,
             1001,
             L"720p2997",
             {1602, 1601, 1602, 1601, 1602}},
            {video_format::x720p3000, 1, 1280, 720, 1280, 720, 30000, 1000, L"720p3000", {1600}},
            {video_format::x720p5000, 1, 1280, 720, 1280, 720, 50000, 1000, L"720p5000", {960}},
            {video_format::x720p5994, 1, 1280, 720, 1280, 720, 60000, 1001, L"720p5994", {801, 800, 801, 801, 801}},
            {video_format::x720p6000, 1, 1280, 720, 1280, 720, 60000, 1000, L"720p6000", {800}},
            {video_format::x1080i5000, 2, 1920, 1080, 1920, 1080, 25000, 1000, L"1080i5000", {1920 / 2}},
            {video_format::x1080i5994,
             2,
             1920,
             1080,
             1920,
             1080,
             30000,
             1001,
             L"1080i5994",
             {801, 801, 801, 800, 801, 801, 801, 800, 801, 801}},
            {video_format::x1080i6000, 2, 1920, 1080, 1920, 1080, 30000, 1000, L"1080i6000", {1600 / 2}},
            {video_format::x1080p2398, 1, 1920, 1080, 1920, 1080, 24000, 1001, L"1080p2398", {2002}},
            {video_format::x1080p2400, 1, 1920, 1080, 1920, 1080, 24000, 1000, L"1080p2400", {2000}},
            {video_format::x1080p2500, 1, 1920, 1080, 1920, 1080, 25000, 1000, L"1080p2500", {1920}},
            {video_format::x1080p2997,
             1,
             1920,
             1080,
             1920,
             1080,
             30000,
             1001,
             L"1080p2997",
             {1602, 1601, 1602, 1601, 1602}},
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
            {video_format::x2160p2997,
             1,
             3840,
             2160,
             3840,
             2160,
             30000,
             1001,
             L"2160p2997",
             {1602, 1601, 1602, 1601, 1602}},
            {video_format::x2160p3000, 1, 3840, 2160, 3840, 2160, 30000, 1000, L"2160p3000", {1600}},
            {video_format::x2160p5000, 1, 3840, 2160, 3840, 2160, 50000, 1000, L"2160p5000", {960}},
            {video_format::x2160p5994, 1, 3840, 2160, 3840, 2160, 60000, 1001, L"2160p5994", {801, 800, 801, 801, 801}},
            {video_format::x2160p6000, 1, 3840, 2160, 3840, 2160, 60000, 1000, L"2160p6000", {800}},
        };

        for (auto& f : default_formats)
            formats_.insert({(boost::to_lower_copy(f.name)), f});
    }

    video_format_desc find(const std::wstring& name) const
    {
        const std::wstring lower = boost::to_lower_copy(name);

        const auto res = formats_.find(lower);
        if (res != formats_.end())
            return res->second;

        return invalid();
    }

    video_format_desc find_format(const video_format& id) const
    {
        for (auto& f : formats_) {
            if (f.second.format == id)
                return f.second;
        }

        return invalid();
    }

    void store(const video_format_desc& format)
    {
        const std::wstring lower = boost::to_lower_copy(format.name);
        formats_.insert({lower, format});
    }

    std::size_t get_max_video_format_size() const
    {
        size_t max = 0;
        for (auto& f : formats_) {
            if (f.second.size > max)
                max = f.second.size;
        }

        return max;
    }
};

video_format_repository::video_format_repository()
    : impl_(new impl())
{
}
video_format_desc video_format_repository::invalid()
{
    return video_format_desc(video_format::invalid, 1, 0, 0, 0, 0, 1, 1, L"invalid", {1});
};
video_format_desc video_format_repository::find(const std::wstring& name) const { return impl_->find(name); }
video_format_desc video_format_repository::find_format(const video_format& format) const
{
    return impl_->find_format(format);
}
void             video_format_repository::store(const video_format_desc& format) { impl_->store(format); }

std::size_t video_format_repository::get_max_video_format_size() const { return impl_->get_max_video_format_size(); }

video_format_desc::video_format_desc(const video_format format,
                                     const int          field_count,
                                     const int          width,
                                     const int          height,
                                     const int          square_width,
                                     const int          square_height,
                                     const int          time_scale,
                                     const int          duration,
                                     const std::wstring name,
                                     const std::vector<int> audio_cadence)
    : format(format)
    , width(width)
    , height(height)
    , square_width(square_width)
    , square_height(square_height)
    , field_count(field_count)
    , hz(static_cast<double>(time_scale) / static_cast<double>(duration))
    , fps(hz * field_count)
    , framerate(time_scale, duration)
    , time_scale(time_scale)
    , duration(duration)
    , size(width * height * 4)
    , name(std::move(name))
    , audio_sample_rate(48000)
    , audio_cadence(std::move(audio_cadence))
{
}

video_format_desc::video_format_desc()
    : format(video_format::invalid)
{
    *this = video_format_repository::invalid();
}

bool operator==(const video_format_desc& lhs, const video_format_desc& rhs)
{
    if (lhs.format == video_format::custom || rhs.format == video_format::custom) {
        if (lhs.format != rhs.format) {
            // If one is custom, and the other isnt, then they dont match
            return false;
        }

        // TODO - expand on this
        if (lhs.width != rhs.width || lhs.height != rhs.height || lhs.framerate != rhs.framerate) {
            return false;
        }

        return true;
    } else {
        // If neither are custom, look just at the format
        return lhs.format == rhs.format;
    }
}

bool operator!=(const video_format_desc& lhs, const video_format_desc& rhs) { return !(lhs == rhs); }

std::wostream& operator<<(std::wostream& out, const video_format_desc& format_desc)
{
    out << format_desc.name.c_str();
    return out;
}

}} // namespace caspar::core
