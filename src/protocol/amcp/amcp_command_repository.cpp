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
 * Author: Helge Norberg, helge.norberg@svt.se
 */

#include "../StdAfx.h"

#include "amcp_command_repository.h"

#include <common/env.h>

#include <boost/property_tree/ptree.hpp>

#include <map>

namespace caspar { namespace protocol { namespace amcp {

AMCPCommand::ptr_type find_command(const std::map<std::wstring, std::pair<amcp_command_func, int>>& commands,
                                   const std::wstring&                                              str,
                                   const command_context&                                           ctx,
                                   std::list<std::wstring>&                                         tokens)
{
    std::wstring subcommand;

    if (!tokens.empty())
        subcommand = boost::to_upper_copy(tokens.front());

    // Start with subcommand syntax like MIXER CLEAR etc
    if (!subcommand.empty()) {
        auto s      = str + L" " + subcommand;
        auto subcmd = commands.find(s);

        if (subcmd != commands.end()) {
            tokens.pop_front();
            return std::make_shared<AMCPCommand>(ctx, subcmd->second.first, subcmd->second.second, s);
        }
    }

    // Resort to ordinary command
    auto s       = str;
    auto command = commands.find(s);

    if (command != commands.end())
        return std::make_shared<AMCPCommand>(ctx, command->second.first, command->second.second, s);

    return nullptr;
}

struct amcp_command_repository::impl
{
    std::vector<channel_context>                         channels;
    spl::shared_ptr<core::cg_producer_registry>          cg_registry;
    spl::shared_ptr<const core::frame_producer_registry> producer_registry;
    spl::shared_ptr<const core::frame_consumer_registry> consumer_registry;
    std::weak_ptr<accelerator::accelerator_device>       ogl_device;
    std::function<void(bool)>                            shutdown_server_now;
    std::string proxy_host = u8(caspar::env::properties().get(L"configuration.amcp.media-server.host", L"127.0.0.1"));
    std::string proxy_port = u8(caspar::env::properties().get(L"configuration.amcp.media-server.port", L"8000"));

    std::map<std::wstring, std::pair<amcp_command_func, int>> commands;
    std::map<std::wstring, std::pair<amcp_command_func, int>> channel_commands;

    impl(const spl::shared_ptr<core::cg_producer_registry>&          cg_registry,
         const spl::shared_ptr<const core::frame_producer_registry>& producer_registry,
         const spl::shared_ptr<const core::frame_consumer_registry>& consumer_registry,
         const std::weak_ptr<accelerator::accelerator_device>&       ogl_device,
         std::function<void(bool)>                                   shutdown_server_now)
        : cg_registry(cg_registry)
        , producer_registry(producer_registry)
        , consumer_registry(consumer_registry)
        , ogl_device(ogl_device)
        , shutdown_server_now(shutdown_server_now)
    {
    }

    void init(const std::vector<spl::shared_ptr<core::video_channel>>& video_channels)
    {
        int index = 0;
        for (const auto& channel : video_channels) {
            std::wstring lifecycle_key = L"lock" + std::to_wstring(index);
            this->channels.push_back(channel_context(channel, lifecycle_key));
            ++index;
        }
    }
};

amcp_command_repository::amcp_command_repository(
    const spl::shared_ptr<core::cg_producer_registry>&          cg_registry,
    const spl::shared_ptr<const core::frame_producer_registry>& producer_registry,
    const spl::shared_ptr<const core::frame_consumer_registry>& consumer_registry,
    const std::weak_ptr<accelerator::accelerator_device>&       ogl_device,
    std::function<void(bool)>                                   shutdown_server_now)
    : impl_(new impl(cg_registry, producer_registry, consumer_registry, ogl_device, shutdown_server_now))
{
}

void amcp_command_repository::init(const std::vector<spl::shared_ptr<core::video_channel>>& channels)
{
    impl_->init(channels);
}

AMCPCommand::ptr_type amcp_command_repository::create_command(const std::wstring&      s,
                                                              IO::ClientInfoPtr        client,
                                                              std::list<std::wstring>& tokens) const
{
    auto& self = *impl_;

    command_context ctx(std::move(client),
                        channel_context(),
                        -1,
                        -1,
                        self.channels,
                        self.cg_registry,
                        self.producer_registry,
                        self.consumer_registry,
                        self.shutdown_server_now,
                        self.proxy_host,
                        self.proxy_port,
                        self.ogl_device);

    auto command = find_command(self.commands, s, ctx, tokens);

    if (command)
        return command;

    return nullptr;
}

const std::vector<channel_context>& amcp_command_repository::channels() const { return impl_->channels; }

AMCPCommand::ptr_type amcp_command_repository::create_channel_command(const std::wstring&      s,
                                                                      IO::ClientInfoPtr        client,
                                                                      unsigned int             channel_index,
                                                                      int                      layer_index,
                                                                      std::list<std::wstring>& tokens) const
{
    auto& self = *impl_;

    auto channel = self.channels.at(channel_index);

    command_context ctx(std::move(client),
                        channel,
                        channel_index,
                        layer_index,
                        self.channels,
                        self.cg_registry,
                        self.producer_registry,
                        self.consumer_registry,
                        self.shutdown_server_now,
                        self.proxy_host,
                        self.proxy_port,
                        self.ogl_device);

    auto command = find_command(self.channel_commands, s, ctx, tokens);

    if (command)
        return command;

    return nullptr;
}

void amcp_command_repository::register_command(std::wstring      category,
                                               std::wstring      name,
                                               amcp_command_func command,
                                               int               min_num_params)
{
    auto& self = *impl_;
    self.commands.insert(std::make_pair(std::move(name), std::make_pair(std::move(command), min_num_params)));
}

void amcp_command_repository::register_channel_command(std::wstring      category,
                                                       std::wstring      name,
                                                       amcp_command_func command,
                                                       int               min_num_params)
{
    auto& self = *impl_;
    self.channel_commands.insert(std::make_pair(std::move(name), std::make_pair(std::move(command), min_num_params)));
}

}}} // namespace caspar::protocol::amcp
