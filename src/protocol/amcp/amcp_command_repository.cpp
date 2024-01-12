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

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <map>

namespace caspar { namespace protocol { namespace amcp {

AMCPCommand::ptr_type make_cmd(amcp_command_func        func,
                               const std::wstring&      name,
                               const std::wstring&      id,
                               IO::ClientInfoPtr        client,
                               unsigned int             channel_index,
                               int                      layer_index,
                               std::list<std::wstring>& tokens)
{
    const std::vector<std::wstring> parameters(tokens.begin(), tokens.end());
    const command_context_simple    ctx(std::move(client), channel_index, layer_index, std::move(parameters));

    return std::make_shared<AMCPCommand>(ctx, func, name, id);
}

AMCPCommand::ptr_type find_command(const std::map<std::wstring, std::pair<amcp_command_func, int>>& commands,
                                   const std::wstring&                                              name,
                                   const std::wstring&                                              request_id,
                                   IO::ClientInfoPtr                                                client,
                                   int                                                              channel_index,
                                   int                                                              layer_index,
                                   std::list<std::wstring>&                                         tokens)
{
    std::wstring subcommand;

    if (!tokens.empty())
        subcommand = boost::to_upper_copy(tokens.front());

    // Start with subcommand syntax like MIXER CLEAR etc
    if (!subcommand.empty()) {
        const auto fullname = name + L" " + subcommand;
        const auto subcmd   = commands.find(fullname);

        if (subcmd != commands.end()) {
            tokens.pop_front();

            if (tokens.size() >= subcmd->second.second) {
                return make_cmd(
                    subcmd->second.first, fullname, request_id, std::move(client), channel_index, layer_index, tokens);
            }
        }
    }

    // Resort to ordinary command
    const auto command = commands.find(name);

    if (command != commands.end() && tokens.size() >= command->second.second) {
        return make_cmd(command->second.first, name, request_id, std::move(client), channel_index, layer_index, tokens);
    }

    return nullptr;
}

struct amcp_command_repository::impl
{
    const spl::shared_ptr<std::vector<channel_context>> channels_;

    std::map<std::wstring, std::pair<amcp_command_func, int>> commands{};
    std::map<std::wstring, std::pair<amcp_command_func, int>> channel_commands{};

    impl(const spl::shared_ptr<std::vector<channel_context>>& channels)
        : channels_(channels)
    {
    }

    AMCPCommand::ptr_type create_command(const std::wstring&      name,
                                         const std::wstring&      id,
                                         IO::ClientInfoPtr        client,
                                         std::list<std::wstring>& tokens) const
    {
        auto command = find_command(commands, name, id, std::move(client), -1, -1, tokens);

        if (command)
            return command;

        return nullptr;
    }

    AMCPCommand::ptr_type create_channel_command(const std::wstring&      name,
                                                 const std::wstring&      id,
                                                 IO::ClientInfoPtr        client,
                                                 unsigned int             channel_index,
                                                 int                      layer_index,
                                                 std::list<std::wstring>& tokens) const
    {
        if (channels_->size() <= channel_index)
            return nullptr;

        auto command = find_command(channel_commands, name, id, std::move(client), channel_index, layer_index, tokens);

        if (command)
            return command;

        return nullptr;
    }

    std::shared_ptr<AMCPCommand> parse_command(IO::ClientInfoPtr       client,
                                               const std::wstring&     command_name,
                                               int                     channel_index,
                                               int                     layer_index,
                                               std::list<std::wstring> tokens,
                                               const std::wstring&     request_id) const
    {
        // Create command instance
        if (channel_index >= 0) {
            return create_channel_command(command_name, request_id, client, channel_index, layer_index, tokens);

        } else {
            // Create global instance
            return create_command(command_name, request_id, client, tokens);
        }
    }
};

amcp_command_repository::amcp_command_repository(const spl::shared_ptr<std::vector<channel_context>>& channels)
    : impl_(new impl(channels))
{
}

const spl::shared_ptr<std::vector<channel_context>>& amcp_command_repository::channels() const
{
    return impl_->channels_;
}

std::shared_ptr<AMCPCommand> amcp_command_repository::parse_command(IO::ClientInfoPtr       client,
                                                                    const std::wstring&     command_name,
                                                                    int                     channel_index,
                                                                    int                     layer_index,
                                                                    std::list<std::wstring> tokens,
                                                                    const std::wstring&     request_id) const
{
    return impl_->parse_command(client, command_name, channel_index, layer_index, tokens, request_id);
}

void amcp_command_repository::register_command(std::wstring      category,
                                               std::wstring      name,
                                               amcp_command_func command,
                                               int               min_num_params)
{
    impl_->commands.insert(std::make_pair(std::move(name), std::make_pair(std::move(command), min_num_params)));
}

void amcp_command_repository::register_channel_command(std::wstring      category,
                                                       std::wstring      name,
                                                       amcp_command_func command,
                                                       int               min_num_params)
{
    impl_->channel_commands.insert(std::make_pair(std::move(name), std::make_pair(std::move(command), min_num_params)));
}

}}} // namespace caspar::protocol::amcp
