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

#include <core/producer/transition/sting_producer.h>
#include <core/producer/transition/transition_producer.h>

#include <functional>
#include <memory>

#include <core/monitor/monitor.h>
#include <core/video_format.h>

#include <boost/property_tree/ptree_fwd.hpp>

namespace caspar {

namespace core {
class cg_proxy;
}

class channel_state_emitter
{
  public:
    virtual void send_state(int channel_index, const core::monitor::state& state) = 0;
};

class server final
{
  public:
    explicit server();
    void                                                                start();
    spl::shared_ptr<std::vector<spl::shared_ptr<core::video_channel>>>& get_channels() const;

    spl::shared_ptr<core::cg_proxy> get_cg_proxy(int channel_index, int layer_index);

    int  add_consumer_from_xml(int channel_index, const boost::property_tree::wptree& config);
    int  add_consumer_from_tokens(const std::shared_ptr<core::video_channel>& channel,
                                  int                                         layer_index,
                                  std::vector<std::wstring>                   amcp_params);
    bool remove_consumer_by_port(int channel_index, int layer_index);
    bool remove_consumer_by_params(int channel_index, std::vector<std::wstring> amcp_params);

    int  add_channel(std::wstring format_desc_str, std::weak_ptr<channel_state_emitter> weak_osc_sender);
    void add_video_format_desc(std::wstring id, core::video_format_desc format);
    core::video_format_desc get_video_format_desc(std::wstring id);

    spl::shared_ptr<core::frame_producer> create_producer(const std::shared_ptr<core::video_channel>& channel,
                                                          int                                         layer_index,
                                                          std::vector<std::wstring>                   amcp_params);
    spl::shared_ptr<core::frame_producer>
    create_sting_transition(const std::shared_ptr<core::video_channel>&  channel,
                            int                                          layer_index,
                            const spl::shared_ptr<core::frame_producer>& destination,
                            core::sting_info&                            info);
    spl::shared_ptr<core::frame_producer>
    create_basic_transition(const std::shared_ptr<core::video_channel>&  channel,
                            int                                          layer_index,
                            const spl::shared_ptr<core::frame_producer>& destination,
                            core::transition_info&                       info);

  private:
    struct impl;
    std::shared_ptr<impl> impl_;

    server(const server&)            = delete;
    server& operator=(const server&) = delete;
};

} // namespace caspar
