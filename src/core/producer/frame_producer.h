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

#include <common/memory.h>

#include <core/interaction/interaction_sink.h>
#include <core/video_format.h>

#include <cstdint>
#include <functional>
#include <future>
#include <string>
#include <type_traits>
#include <vector>

namespace caspar { namespace core {

class frame_producer : public interaction_sink
{
    frame_producer(const frame_producer&);
    frame_producer& operator=(const frame_producer&);

  public:
    static const spl::shared_ptr<frame_producer>& empty();

    frame_producer() {}
    virtual ~frame_producer() {}

    virtual draw_frame                receive()                                     = 0;
    virtual std::future<std::wstring> call(const std::vector<std::wstring>& params) = 0;

    virtual monitor::subject& monitor_output() = 0;

    virtual void on_interaction(const interaction_event::ptr& event) override {}
    virtual bool collides(double x, double y) const override { return false; }

    virtual void         paused(bool value)   = 0;
    virtual std::wstring print() const        = 0;
    virtual std::wstring name() const         = 0;
    virtual uint32_t     nb_frames() const    = 0;
    virtual uint32_t     frame_number() const = 0;
    virtual draw_frame   last_frame()         = 0;
    virtual void         leading_producer(const spl::shared_ptr<frame_producer>&) {}
    virtual spl::shared_ptr<frame_producer> following_producer() const
    {
        return core::frame_producer::empty();
    }
};

class frame_producer_base : public frame_producer
{
  public:
    frame_producer_base();
    virtual ~frame_producer_base() {}

    virtual std::future<std::wstring> call(const std::vector<std::wstring>& params) override;

    void               paused(bool value) override;
    uint32_t           nb_frames() const override;
    uint32_t           frame_number() const override;
    virtual draw_frame last_frame() override;

  private:
    virtual draw_frame receive() override;
    virtual draw_frame receive_impl() = 0;

    struct impl;
    friend struct impl;
    std::shared_ptr<impl> impl_;
};

class frame_producer_registry;

struct frame_producer_dependencies
{
    spl::shared_ptr<core::frame_factory>           frame_factory;
    std::vector<spl::shared_ptr<video_channel>>    channels;
    video_format_desc                              format_desc;
    spl::shared_ptr<const frame_producer_registry> producer_registry;
    spl::shared_ptr<const cg_producer_registry>    cg_registry;

    frame_producer_dependencies(const spl::shared_ptr<core::frame_factory>&          frame_factory,
                                const std::vector<spl::shared_ptr<video_channel>>&   channels,
                                const video_format_desc&                             format_desc,
                                const spl::shared_ptr<const frame_producer_registry> producer_registry,
                                const spl::shared_ptr<const cg_producer_registry>    cg_registry);
};

typedef std::function<spl::shared_ptr<core::frame_producer>(const frame_producer_dependencies&,
                                                            const std::vector<std::wstring>&)>
    producer_factory_t;

class frame_producer_registry : boost::noncopyable
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
};

spl::shared_ptr<core::frame_producer> create_destroy_proxy(spl::shared_ptr<core::frame_producer> producer);
void                                  destroy_producers_synchronously();

}} // namespace caspar::core
