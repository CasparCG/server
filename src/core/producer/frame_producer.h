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

#include "../fwd.h"
#include "../monitor/monitor.h"

#include <common/except.h>
#include <common/memory.h>

#include <core/frame/draw_frame.h>
#include <core/video_format.h>

#include <boost/optional.hpp>

#include <cstdint>
#include <functional>
#include <future>
#include <string>
#include <type_traits>
#include <vector>

namespace caspar { namespace core {

class frame_producer
{
    frame_producer(const frame_producer&);
    frame_producer& operator=(const frame_producer&);

    uint32_t         frame_number_ = 0;
    core::draw_frame last_frame_;
    core::draw_frame first_frame_;

  public:
    static const spl::shared_ptr<frame_producer>& empty();

    frame_producer(core::draw_frame frame)
        : last_frame_(std::move(frame))
    {
    }
    frame_producer() {}
    virtual ~frame_producer() {}

    draw_frame receive(const video_field field, int nb_samples)
    {
        if (frame_number_ == 0 && first_frame_) {
            frame_number_ += 1;
            return first_frame_;
        }

        auto frame = receive_impl(field, nb_samples);

        if (frame) {
            frame_number_ += 1;
            last_frame_ = frame;

            if (!first_frame_) {
                first_frame_ = frame;
            }
        }

        return frame;
    }

    virtual draw_frame receive_impl(const video_field field, int nb_samples) { return core::draw_frame{}; }

    virtual std::future<std::wstring> call(const std::vector<std::wstring>& params)
    {
        CASPAR_THROW_EXCEPTION(not_implemented());
    }

    virtual core::monitor::state state() const
    {
        static const monitor::state empty;
        return empty;
    }
    virtual std::wstring print() const { return L"frame_producer"; }
    virtual std::wstring name() const { return L"frame_producer"; }
    virtual uint32_t     frame_number() const { return frame_number_; }
    virtual uint32_t     nb_frames() const { return std::numeric_limits<uint32_t>::max(); }
    virtual draw_frame   last_frame(const video_field field)
    {
        if (!last_frame_) {
            last_frame_ = receive_impl(field, 0);
        }
        return core::draw_frame::still(last_frame_);
    }
    virtual draw_frame first_frame(const video_field field)
    {
        if (!first_frame_) {
            first_frame_ = receive_impl(field, 0);
        }
        return core::draw_frame::still(first_frame_);
    }
    virtual void                            leading_producer(const spl::shared_ptr<frame_producer>&) {}
    virtual spl::shared_ptr<frame_producer> following_producer() const { return core::frame_producer::empty(); }
    virtual boost::optional<int64_t>        auto_play_delta() const { return boost::none; }
};

class const_producer : public core::frame_producer
{
    core::draw_frame frame1_;
    core::draw_frame frame2_;

  public:
    const_producer(core::draw_frame frame1, core::draw_frame frame2)
        : frame1_(frame1)
        , frame2_(frame2)
    {
    }

    // frame_producer

    core::draw_frame last_frame(const core::video_field field) override
    {
        if (field == core::video_field::b)
            return frame2_;
        else
            return frame1_;
    }

    core::draw_frame first_frame(const core::video_field field) override { return last_frame(field); }
};

class frame_producer_registry;

struct frame_producer_dependencies
{
    spl::shared_ptr<core::frame_factory>           frame_factory;
    std::vector<spl::shared_ptr<video_channel>>    channels;
    video_format_repository                        format_repository;
    video_format_desc                              format_desc;
    spl::shared_ptr<const frame_producer_registry> producer_registry;
    spl::shared_ptr<const cg_producer_registry>    cg_registry;

    frame_producer_dependencies(const spl::shared_ptr<core::frame_factory>&           frame_factory,
                                const std::vector<spl::shared_ptr<video_channel>>&    channels,
                                const video_format_repository&                        format_repository,
                                const video_format_desc&                              format_desc,
                                const spl::shared_ptr<const frame_producer_registry>& producer_registry,
                                const spl::shared_ptr<const cg_producer_registry>&    cg_registry);
};

using producer_factory_t = std::function<spl::shared_ptr<core::frame_producer>(const frame_producer_dependencies&,
                                                                               const std::vector<std::wstring>&)>;

class frame_producer_registry
{
  public:
    frame_producer_registry();
    void register_producer_factory(std::wstring name, const producer_factory_t& factoryr); // Not thread-safe.
    spl::shared_ptr<core::frame_producer> create_producer(const frame_producer_dependencies&,
                                                          const std::vector<std::wstring>& params) const;
    spl::shared_ptr<core::frame_producer> create_producer(const frame_producer_dependencies&,
                                                          const std::wstring& params) const;

  private:
    struct impl;
    spl::shared_ptr<impl> impl_;

    frame_producer_registry(const frame_producer_registry&)            = delete;
    frame_producer_registry& operator=(const frame_producer_registry&) = delete;
};

spl::shared_ptr<core::frame_producer> create_destroy_proxy(spl::shared_ptr<core::frame_producer> producer);
void                                  destroy_producers_synchronously();

}} // namespace caspar::core
