/*
 * Copyright (c) 2018 Norsk rikskringkasting AS
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
 * Author: Julian Waller, julian@superfly.tv
 */

#include "amcp_command_repository_wrapper.h"

#include <common/future.h>

namespace caspar { namespace protocol { namespace amcp {

void amcp_command_repository_wrapper::register_command(std::wstring              category,
                                                       std::wstring              name,
                                                       amcp_command_impl_func    command,
                                                       int                       min_num_params)
{
    std::weak_ptr<command_context_factory> ctx3 = ctx_;
    auto func = [ctx3, command](const command_context_simple& ctx, const std::vector<channel_context>& channels) {
        auto ctx4 = ctx3.lock();
        if (!ctx4)
            return make_ready_future<std::wstring>(L"");

        auto ctx2 = ctx4->create(ctx, channels);
        return command(ctx2);
    };

    repo_->register_command(category, name, func, min_num_params);
}

void amcp_command_repository_wrapper::register_command(std::wstring              category,
                                                       std::wstring              name,
                                                       amcp_command_impl_func2   command,
                                                       int                       min_num_params)
{
    std::weak_ptr<command_context_factory> ctx3 = ctx_;
    auto func = [ctx3, command](const command_context_simple& ctx, const std::vector<channel_context>& channels) {
        auto ctx4 = ctx3.lock();
        if (!ctx4)
            return make_ready_future<std::wstring>(L"");

        auto ctx2 = ctx4->create(ctx, channels);
        return make_ready_future(command(ctx2));
    };

    repo_->register_command(category, name, func, min_num_params);
}

void amcp_command_repository_wrapper::register_channel_command(std::wstring              category,
                                                               std::wstring              name,
                                                               amcp_command_impl_func    command,
                                                               int                       min_num_params)
{
    std::weak_ptr<command_context_factory> ctx3 = ctx_;
    auto func = [ctx3, command](const command_context_simple& ctx, const std::vector<channel_context>& channels) {
        auto ctx4 = ctx3.lock();
        if (!ctx4)
            return make_ready_future<std::wstring>(L"");

        auto ctx2 = ctx4->create(ctx, channels);
        return command(ctx2);
    };

    repo_->register_channel_command(category, name, func, min_num_params);
}

void amcp_command_repository_wrapper::register_channel_command(std::wstring              category,
                                                               std::wstring              name,
                                                               amcp_command_impl_func2   command,
                                                               int                       min_num_params)
{
    std::weak_ptr<command_context_factory> ctx3 = ctx_;
    auto func = [ctx3, command](const command_context_simple& ctx, const std::vector<channel_context>& channels) {
        auto ctx4 = ctx3.lock();
        if (!ctx4)
            return make_ready_future<std::wstring>(L"");

        auto ctx2 = ctx4->create(ctx, channels);
        return make_ready_future(command(ctx2));
    };

    repo_->register_channel_command(category, name, func, min_num_params);
}

}}} // namespace caspar::protocol::amcp