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

#include <boost/property_tree/ptree_fwd.hpp>

#include <functional>
#include <future>
#include <string>
#include <vector>

namespace caspar { namespace core {

class frame_consumer
{
    frame_consumer(const frame_consumer&);
    frame_consumer& operator=(const frame_consumer&);

  public:
    static const spl::shared_ptr<frame_consumer>& empty();

    frame_consumer() {}
    virtual ~frame_consumer() {}

    virtual std::future<bool> send(const_frame frame)                                             = 0;
    virtual void              initialize(const video_format_desc& format_desc, int channel_index) = 0;

    virtual core::monitor::state state() const
    {
        static const monitor::state empty;
        return empty;
    }

    virtual std::wstring print() const = 0;
    virtual std::wstring name() const  = 0;
    virtual bool         has_synchronization_clock() const { return false; }
    virtual int          index() const = 0;
};

using consumer_factory_t =
    std::function<spl::shared_ptr<frame_consumer>(const std::vector<std::wstring>&,
                                                  std::vector<spl::shared_ptr<video_channel>> channels)>;
using preconfigured_consumer_factory_t =
    std::function<spl::shared_ptr<frame_consumer>(const boost::property_tree::wptree&         element,
                                                  std::vector<spl::shared_ptr<video_channel>> channels)>;

class frame_consumer_registry
{
  public:
    frame_consumer_registry();
    void register_consumer_factory(const std::wstring& name, const consumer_factory_t& factory);
    void register_preconfigured_consumer_factory(const std::wstring&                     element_name,
                                                 const preconfigured_consumer_factory_t& factory);
    spl::shared_ptr<frame_consumer> create_consumer(const std::vector<std::wstring>&            params,
                                                    std::vector<spl::shared_ptr<video_channel>> channels) const;
    spl::shared_ptr<frame_consumer> create_consumer(const std::wstring&                         element_name,
                                                    const boost::property_tree::wptree&         element,
                                                    std::vector<spl::shared_ptr<video_channel>> channels) const;

  private:
    struct impl;
    spl::shared_ptr<impl> impl_;
    frame_consumer_registry(const frame_consumer_registry&) = delete;
    frame_consumer_registry& operator=(const frame_consumer_registry&) = delete;
};

void destroy_consumers_synchronously();

}} // namespace caspar::core
