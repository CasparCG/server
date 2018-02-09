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
 * Author: Helge Norberg, helge.norberg@svt.se
 */

#include "image_scroll_producer.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <FreeImage.h>
#include <windows.h>

#include "../util/image_algorithms.h"
#include "../util/image_loader.h"
#include "../util/image_view.h"

#include <core/video_format.h>

#include <core/frame/draw_frame.h>
#include <core/frame/frame.h>
#include <core/frame/frame_factory.h>
#include <core/frame/frame_transform.h>
#include <core/frame/pixel_format.h>
#include <core/monitor/monitor.h>

#include <common/array.h>
#include <common/env.h>
#include <common/except.h>
#include <common/future.h>
#include <common/log.h>
#include <common/os/filesystem.h>
#include <common/param.h>
#include <common/scope_exit.h>
#include <common/tweener.h>

#include <boost/date_time.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/scoped_array.hpp>

#include <algorithm>
#include <array>
#include <cstdint>

namespace caspar { namespace image {

// Like tweened_transform but for speed
class speed_tweener
{
    double  source_   = 0.0;
    double  dest_     = 0.0;
    int     duration_ = 0;
    int     time_     = 0;
    tweener tweener_;

  public:
    speed_tweener() = default;
    speed_tweener(double source, double dest, int duration, const tweener& tween)
        : source_(source)
        , dest_(dest)
        , duration_(duration)
        , time_(0)
        , tweener_(tween)
    {
    }

    double dest() const { return dest_; }

    double fetch() const
    {
        if (time_ == duration_)
            return dest_;

        double delta  = dest_ - source_;
        double result = tweener_(time_, source_, delta, duration_);

        return result;
    }

    double fetch_and_tick()
    {
        time_ = std::min(time_ + 1, duration_);
        return fetch();
    }
};

struct image_scroll_producer : public core::frame_producer_base
{
    core::monitor::state state_;

    const std::wstring            filename_;
    std::vector<core::draw_frame> frames_;
    core::video_format_desc       format_desc_;
    int                           width_;
    int                           height_;

    double                                    delta_ = 0.0;
    speed_tweener                             speed_;
    boost::optional<boost::posix_time::ptime> end_time_;

    int  start_offset_x_ = 0;
    int  start_offset_y_ = 0;
    bool progressive_;

    explicit image_scroll_producer(const spl::shared_ptr<core::frame_factory>& frame_factory,
                                   const core::video_format_desc&              format_desc,
                                   const std::wstring&                         filename,
                                   double                                      s,
                                   double                                      duration,
                                   boost::optional<boost::posix_time::ptime>   end_time,
                                   int                                         motion_blur_px         = 0,
                                   bool                                        premultiply_with_alpha = false,
                                   bool                                        progressive            = false)
        : filename_(filename)
        , format_desc_(format_desc)
        , end_time_(std::move(end_time))
        , progressive_(progressive)
    {
        double speed = s;

        if (end_time_)
            speed = -1.0;

        auto bitmap = load_image(filename_);
        FreeImage_FlipVertical(bitmap.get());

        width_  = FreeImage_GetWidth(bitmap.get());
        height_ = FreeImage_GetHeight(bitmap.get());

        bool vertical   = width_ == format_desc_.width;
        bool horizontal = height_ == format_desc_.height;

        if (!vertical && !horizontal)
            CASPAR_THROW_EXCEPTION(caspar::user_error()
                                   << msg_info("Neither width nor height matched the video resolution"));

        if (duration != 0.0)
            speed = speed_from_duration(duration);

        if (vertical) {
            if (speed < 0.0) {
                start_offset_y_ = height_ + format_desc_.height;
            }
        } else {
            if (speed > 0.0)
                start_offset_x_ = format_desc_.width - (width_ % format_desc_.width);
            else
                start_offset_x_ = format_desc_.width - (width_ % format_desc_.width) + width_ + format_desc_.width;
        }

        speed_ = speed_tweener(speed, speed, 0, tweener(L"linear"));

        auto                   bytes = FreeImage_GetBits(bitmap.get());
        auto                   count = width_ * height_ * 4;
        image_view<bgra_pixel> original_view(bytes, width_, height_);

        if (premultiply_with_alpha)
            premultiply(original_view);

        boost::scoped_array<uint8_t> blurred_copy;

        if (motion_blur_px > 0) {
            double angle = 3.14159265 / 2; // Up

            if (horizontal && speed < 0)
                angle *= 2; // Left
            else if (vertical && speed > 0)
                angle *= 3; // Down
            else if (horizontal && speed > 0)
                angle = 0.0; // Right

            blurred_copy.reset(new uint8_t[count]);
            image_view<bgra_pixel> blurred_view(blurred_copy.get(), width_, height_);
            caspar::tweener        blur_tweener(L"easeInQuad");
            blur(original_view, blurred_view, angle, motion_blur_px, blur_tweener);
            bytes = blurred_copy.get();
            bitmap.reset();
        }

        if (vertical) {
            int n = 1;

            while (count > 0) {
                core::pixel_format_desc desc = core::pixel_format::bgra;
                desc.planes.push_back(core::pixel_format_desc::plane(width_, format_desc_.height, 4));
                auto frame = frame_factory->create_frame(this, desc);

                if (count >= frame.image_data(0).size()) {
                    std::copy_n(bytes + count - frame.image_data(0).size(),
                                frame.image_data(0).size(),
                                frame.image_data(0).begin());
                    count -= static_cast<int>(frame.image_data(0).size());
                } else {
                    memset(frame.image_data(0).begin(), 0, frame.image_data(0).size());
                    std::copy_n(bytes, count, frame.image_data(0).begin() + format_desc_.size - count);
                    count = 0;
                }

                core::draw_frame draw_frame(std::move(frame));

                // Set the relative position to the other image fragments
                draw_frame.transform().image_transform.fill_translation[1] = -n++;

                frames_.push_back(draw_frame);
            }
        } else if (horizontal) {
            int i = 0;
            while (count > 0) {
                core::pixel_format_desc desc = core::pixel_format::bgra;
                desc.planes.push_back(core::pixel_format_desc::plane(format_desc_.width, height_, 4));
                auto frame = frame_factory->create_frame(this, desc);
                if (count >= frame.image_data(0).size()) {
                    for (int y = 0; y < height_; ++y)
                        std::copy_n(bytes + i * format_desc_.width * 4 + y * width_ * 4,
                                    format_desc_.width * 4,
                                    frame.image_data(0).begin() + y * format_desc_.width * 4);

                    ++i;
                    count -= static_cast<int>(frame.image_data(0).size());
                } else {
                    memset(frame.image_data(0).begin(), 0, frame.image_data(0).size());
                    auto width2 = width_ % format_desc_.width;
                    for (int y = 0; y < height_; ++y)
                        std::copy_n(bytes + i * format_desc_.width * 4 + y * width_ * 4,
                                    width2 * 4,
                                    frame.image_data(0).begin() + y * format_desc_.width * 4);

                    count = 0;
                }

                frames_.push_back(core::draw_frame(std::move(frame)));
            }

            std::reverse(frames_.begin(), frames_.end());

            // Set the relative positions of the image fragments.
            for (size_t n = 0; n < frames_.size(); ++n) {
                double translation                                         = -(static_cast<double>(n) + 1.0);
                frames_[n].transform().image_transform.fill_translation[0] = translation;
            }
        }

        CASPAR_LOG(info) << print() << L" Initialized";
    }

    double get_total_num_pixels() const
    {
        bool vertical = width_ == format_desc_.width;

        if (vertical)
            return height_ + format_desc_.height;
        else
            return width_ + format_desc_.width;
    }

    double speed_from_duration(double duration_seconds) const
    {
        return get_total_num_pixels() /
               (duration_seconds * format_desc_.fps * static_cast<double>(format_desc_.field_count));
    }

    std::future<std::wstring> call(const std::vector<std::wstring>& params) override
    {
        auto cmd = params.at(0);

        if (boost::iequals(cmd, L"SPEED")) {
            if (params.size() == 1)
                return make_ready_future(boost::lexical_cast<std::wstring>(-speed_.fetch()));

            auto         val      = boost::lexical_cast<double>(params.at(1));
            int          duration = params.size() > 2 ? boost::lexical_cast<int>(params.at(2)) : 0;
            std::wstring tween    = params.size() > 3 ? params.at(3) : L"linear";
            speed_                = speed_tweener(speed_.fetch(), -val, duration, tween);
        }

        return make_ready_future<std::wstring>(L"");
    }

    std::vector<core::draw_frame> get_visible()
    {
        std::vector<core::draw_frame> result;
        result.reserve(frames_.size());

        for (auto& frame : frames_) {
            auto& fill_translation = frame.transform().image_transform.fill_translation;

            if (width_ == format_desc_.width) {
                auto motion_offset_in_screens =
                    (static_cast<double>(start_offset_y_) + delta_) / static_cast<double>(format_desc_.height);
                auto vertical_offset = fill_translation[1] + motion_offset_in_screens;

                if (vertical_offset < -1.0 || vertical_offset > 1.0) {
                    continue;
                }
            } else {
                auto motion_offset_in_screens =
                    (static_cast<double>(start_offset_x_) + delta_) / static_cast<double>(format_desc_.width);
                auto horizontal_offset = fill_translation[0] + motion_offset_in_screens;

                if (horizontal_offset < -1.0 || horizontal_offset > 1.0) {
                    continue;
                }
            }

            result.push_back(frame);
        }

        return std::move(result);
    }

    // frame_producer
    core::draw_frame render_frame(bool allow_eof)
    {
        if (frames_.empty())
            return core::draw_frame{};

        core::draw_frame result(get_visible());
        auto&            fill_translation = result.transform().image_transform.fill_translation;

        if (width_ == format_desc_.width) {
            if (static_cast<size_t>(std::abs(delta_)) >= height_ + format_desc_.height && allow_eof)
                return core::draw_frame{};

            fill_translation[1] = static_cast<double>(start_offset_y_) / static_cast<double>(format_desc_.height) +
                                  delta_ / static_cast<double>(format_desc_.height);
        } else {
            if (static_cast<size_t>(std::abs(delta_)) >= width_ + format_desc_.width && allow_eof)
                return core::draw_frame{};

            fill_translation[0] = static_cast<double>(start_offset_x_) / static_cast<double>(format_desc_.width) +
                                  (delta_) / static_cast<double>(format_desc_.width);
        }

        return result;
    }

    core::draw_frame render_frame(bool allow_eof, bool advance_delta)
    {
        auto result = render_frame(allow_eof);

        if (advance_delta) {
            advance();
        }

        return result;
    }

    void advance()
    {
        if (end_time_) {
            boost::posix_time::ptime now(boost::posix_time::second_clock::local_time());

            auto diff    = *end_time_ - now;
            auto seconds = diff.total_seconds();

            set_speed(-speed_from_duration(static_cast<double>(seconds)));
            end_time_ = boost::none;
        } else
            delta_ += speed_.fetch_and_tick();
    }

    void set_speed(double speed) { speed_ = speed_tweener(speed, speed, 0, tweener(L"linear")); }

    core::draw_frame receive_impl() override
    {
        CASPAR_SCOPE_EXIT
        {
            state_["file/path"] = filename_;
            state_["delta"]     = delta_;
            state_["speed"]     = speed_.fetch();
        };

        core::draw_frame result;

        if (format_desc_.field_mode == core::field_mode::progressive || progressive_) {
            result = render_frame(true, true);
        } else {
            auto field1 = render_frame(true, true);
            auto field2 = render_frame(true, false);

            if (field1 && !field2) {
                field2 = render_frame(false, true);
            } else {
                advance();
            }

            result = core::draw_frame::interlace(field1, field2, format_desc_.field_mode);
        }

        return result;
    }

    std::wstring print() const override { return L"image_scroll_producer[" + filename_ + L"]"; }

    std::wstring name() const override { return L"image-scroll"; }

    uint32_t nb_frames() const override
    {
        if (width_ == format_desc_.width) {
            auto length = (height_ + format_desc_.height * 2);
            return static_cast<uint32_t>(length / std::abs(speed_.fetch())); // + length % std::abs(delta_));
        } else {
            auto length = (width_ + format_desc_.width * 2);
            return static_cast<uint32_t>(length / std::abs(speed_.fetch())); // + length % std::abs(delta_));
        }
    }

    const core::monitor::state& state() { return state_; }
};

spl::shared_ptr<core::frame_producer> create_scroll_producer(const core::frame_producer_dependencies& dependencies,
                                                             const std::vector<std::wstring>&         params)
{
    std::wstring filename = env::media_folder() + params.at(0);

    auto ext =
        std::find_if(supported_extensions().begin(), supported_extensions().end(), [&](const std::wstring& ex) -> bool {
            auto file =
                caspar::find_case_insensitive(boost::filesystem::path(filename).replace_extension(ex).wstring());

            return static_cast<bool>(file);
        });

    if (ext == supported_extensions().end())
        return core::frame_producer::empty();

    double                                    duration = 0.0;
    double                                    speed    = get_param(L"SPEED", params, 0.0);
    boost::optional<boost::posix_time::ptime> end_time;

    if (speed == 0)
        duration = get_param(L"DURATION", params, 0.0);

    if (duration == 0) {
        auto end_time_str = get_param(L"END_TIME", params);

        if (!end_time_str.empty()) {
            end_time = boost::posix_time::time_from_string(u8(end_time_str));
        }
    }

    if (speed == 0 && duration == 0 && !end_time)
        return core::frame_producer::empty();

    int motion_blur_px = get_param(L"BLUR", params, 0);

    bool premultiply_with_alpha = contains_param(L"PREMULTIPLY", params);
    bool progressive            = contains_param(L"PROGRESSIVE", params);

    return core::create_destroy_proxy(
        spl::make_shared<image_scroll_producer>(dependencies.frame_factory,
                                                dependencies.format_desc,
                                                *caspar::find_case_insensitive(filename + *ext),
                                                -speed,
                                                -duration,
                                                end_time,
                                                motion_blur_px,
                                                premultiply_with_alpha,
                                                progressive));
}

}} // namespace caspar::image
