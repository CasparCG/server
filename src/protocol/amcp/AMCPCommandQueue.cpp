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
 * Author: Nicklas P Andersson
 */

#include "../StdAfx.h"

#include "AMCPCommandQueue.h"

#include <boost/lexical_cast.hpp>
#include <common/except.h>
#include <common/timer.h>

namespace caspar { namespace protocol { namespace amcp {

AMCPCommandQueue::AMCPCommandQueue(const std::wstring&                                  name,
                                   const spl::shared_ptr<std::vector<channel_context>>& channels)
    : executor_(L"AMCPCommandQueue " + name)
    , channels_(channels)
{
}

AMCPCommandQueue::~AMCPCommandQueue() {}

std::future<CommandResult> exec_cmd(std::shared_ptr<AMCPCommand>                         cmd,
                                    const spl::shared_ptr<std::vector<channel_context>>& channels,
                                    bool                                                 reply_without_req_id)
{
    try {
        try {
            caspar::timer timer;

            auto name = cmd->name();
            CASPAR_LOG(debug) << "Executing command: " << name;

            auto res = cmd->Execute(channels).share();
            return std::async(std::launch::async, [res, timer, name]() -> CommandResult {
                auto result = res.get();

                CASPAR_LOG(debug) << "Executed command (" << timer.elapsed() << "s): " << name;

                return CommandResult(0, result);
            });

        } catch (file_not_found&) {
            CASPAR_LOG(error) << " File not found.";
            return CommandResult::create(404, cmd->name() + L" FAILED\r\n");
        } catch (expected_user_error&) {
            return CommandResult::create(403, cmd->name() + L" FAILED\r\n");
        } catch (user_error&) {
            CASPAR_LOG(error) << " Check syntax.";
            return CommandResult::create(403, cmd->name() + L" FAILED\r\n");
        } catch (std::out_of_range&) {
            CASPAR_LOG(error) << L"Missing parameter. Check syntax.";
            return CommandResult::create(402, cmd->name() + L" FAILED\r\n");
        } catch (boost::bad_lexical_cast&) {
            CASPAR_LOG(error) << L"Invalid parameter. Check syntax.";
            return CommandResult::create(403, cmd->name() + L" FAILED\r\n");
        } catch (...) {
            CASPAR_LOG_CURRENT_EXCEPTION();
            CASPAR_LOG(error) << "Failed to execute command: " << cmd->name();
            return CommandResult::create(501, cmd->name() + L" FAILED\r\n");
        }

    } catch (...) {
        CASPAR_LOG_CURRENT_EXCEPTION();
        return CommandResult::create(500, L"INTERNAL ERROR\r\n");
    }
}

std::future<std::vector<CommandResult>> AMCPCommandQueue::AddCommand(std::shared_ptr<AMCPGroupCommand> pCurrentCommand)
{
    if (!pCurrentCommand)
        return {};

    if (executor_.size() > 128) {
        CASPAR_LOG(error) << "AMCP Command Queue Overflow.";
        CASPAR_LOG(error) << "Failed to execute command:" << pCurrentCommand->name();

        std::vector<CommandResult> res = {CommandResult(504, L"QUEUE OVERFLOW\r\n")};
        return make_ready_future(std::move(res));
    }

    return executor_.begin_invoke([=] {
        try {
            return Execute(pCurrentCommand);

            // CASPAR_LOG(trace) << "Ready for a new command";
        } catch (...) {
            CASPAR_LOG_CURRENT_EXCEPTION();
            std::vector<CommandResult> res = {CommandResult(500, L"INTERNAL ERROR\r\n")};
            return res;
        }
    });
} // namespace amcp

std::vector<CommandResult> AMCPCommandQueue::Execute(std::shared_ptr<AMCPGroupCommand> cmd) const
{
    if (cmd->Commands().empty())
        return {};

    // Shortcut for commands which are either not a batch, or don't need to be
    if (cmd->Commands().size() == 1) {
        return {exec_cmd(cmd->Commands().at(0), channels_, true).get()};
    }

    caspar::timer timer;
    CASPAR_LOG(warning) << "Executing batch: " << cmd->name() << L"(" << cmd->Commands().size() << L" commands)";

    spl::shared_ptr<std::vector<channel_context>>     delayed_channels;
    std::vector<std::shared_ptr<core::stage_delayed>> delayed_stages;
    std::vector<std::future<CommandResult>>           results;
    std::vector<std::unique_lock<std::mutex>>         channel_locks;

    try {
        for (auto& ch : *channels_) {
            auto st = std::make_shared<core::stage_delayed>(ch.raw_channel->stage(), ch.raw_channel->index());
            delayed_stages.push_back(st);
            delayed_channels->emplace_back(ch.raw_channel, st, ch.lifecycle_key_);
        }

        // 'execute' aka queue all comamnds
        for (auto& cmd2 : cmd->Commands()) {
            results.push_back(exec_cmd(cmd2, delayed_channels, cmd->HasClient()));
        }

        // lock all the channels needed
        for (auto& st : delayed_stages) {
            if (st->count_queued() == 0) {
                continue;
            }

            channel_locks.push_back(st->get_lock());
        }

        // execute the commands
        for (auto& st : delayed_stages) {
            st->release();
        }
    } catch (...) {
        // Ensure the created executors don't get leaked
        for (auto& st : delayed_stages) {
            st->abort();
        }

        throw;
    }

    // wait for the commands to finish
    for (auto& st : delayed_stages) {
        st->wait();
    }
    channel_locks.clear();

    std::vector<CommandResult> results2;
    results2.reserve(results.size());

    int failed = 0;
    for (auto& f : results) {
        auto res = f.get();

        if (res.status != 0)
            failed++;

        results2.push_back(std::move(res));
    }

    // if (failed > 0)
    //     cmd->SendReply(L"202 COMMIT PARTIAL\r\n");
    // else
    //     cmd->SendReply(L"202 COMMIT OK\r\n");

    CASPAR_LOG(debug) << "Executed batch (" << timer.elapsed() << "s): " << cmd->name();

    return results2;
}

}}} // namespace caspar::protocol::amcp
