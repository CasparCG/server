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

std::future<std::wstring> exec_cmd(std::shared_ptr<AMCPCommand>                         cmd,
                                   const spl::shared_ptr<std::vector<channel_context>>& channels)
{
    try {
        try {
            caspar::timer timer;

            auto name = cmd->name();
            CASPAR_LOG(debug) << "Executing command: " << name;

            auto res = cmd->Execute(channels).share();
            return std::async(std::launch::async, [res, cmd, timer, name]() -> std::wstring {
                cmd->SetResult(res.get());

                CASPAR_LOG(debug) << "Executed command (" << timer.elapsed() << "s): " << name;

                return L"";
            });

        } catch (file_not_found&) {
            CASPAR_LOG(error) << " File not found.";
            return make_ready_future(L"404" + cmd->name() + L" FAILED\r\n");
        } catch (expected_user_error&) {
            return make_ready_future(L"403" + cmd->name() + L" FAILED\r\n");
        } catch (user_error&) {
            CASPAR_LOG(error) << " Check syntax.";
            return make_ready_future(L"403" + cmd->name() + L" FAILED\r\n");
        } catch (std::out_of_range&) {
            CASPAR_LOG(error) << L"Missing parameter. Check syntax.";
            return make_ready_future(L"402" + cmd->name() + L" FAILED\r\n");
        } catch (boost::bad_lexical_cast&) {
            CASPAR_LOG(error) << L"Invalid parameter. Check syntax.";
            return make_ready_future(L"403" + cmd->name() + L" FAILED\r\n");
        } catch (...) {
            CASPAR_LOG_CURRENT_EXCEPTION();
            CASPAR_LOG(error) << "Failed to execute command: " << cmd->name();
            return make_ready_future(L"501" + cmd->name() + L" FAILED\r\n");
        }

    } catch (...) {
        CASPAR_LOG_CURRENT_EXCEPTION();
        return make_ready_future<std::wstring>(L"500 INTERNAL ERROR\r\n");
    }
}

std::future<std::wstring> AMCPCommandQueue::QueueCommandBatch(std::vector<std::shared_ptr<AMCPCommand>> cmds)
{
    if (cmds.empty())
        return make_ready_future<std::wstring>(L"");

    if (executor_.size() > 128) {
        CASPAR_LOG(error) << "AMCP Command Queue Overflow.";
        CASPAR_LOG(error) << "Failed to execute commands";

        return make_ready_future<std::wstring>(L"504 QUEUE OVERFLOW\r\n");
    }

    return executor_.begin_invoke([=]() -> std::wstring {
        try {
            return Execute(cmds);

            // CASPAR_LOG(trace) << "Ready for a new command";
            // return L"";
        } catch (...) {
            CASPAR_LOG_CURRENT_EXCEPTION();
            return L"500 INTERNAL ERROR\r\n";
        }
    });
} // namespace amcp

std::wstring AMCPCommandQueue::Execute(std::vector<std::shared_ptr<AMCPCommand>> cmds) const
{
    if (cmds.empty())
        return L"";

    // Shortcut for commands which are either not a batch, or don't need to be
    if (cmds.size() == 1) {
        return exec_cmd(cmds.at(0), channels_).get();
    }

    caspar::timer timer;
    CASPAR_LOG(warning) << "Executing batch: " << L"(" << cmds.size() << L" commands)";

    spl::shared_ptr<std::vector<channel_context>>     delayed_channels;
    std::vector<std::shared_ptr<core::stage_delayed>> delayed_stages;
    std::vector<std::future<std::wstring>>            results;
    std::vector<std::unique_lock<std::mutex>>         channel_locks;

    try {
        for (auto& ch : *channels_) {
            auto st = std::make_shared<core::stage_delayed>(ch.raw_channel->stage(), ch.raw_channel->index());
            delayed_stages.push_back(st);
            delayed_channels->emplace_back(ch.raw_channel, st, ch.lifecycle_key_);
        }

        // 'execute' aka queue all comamnds
        for (auto& cmd2 : cmds) {
            results.push_back(exec_cmd(cmd2, delayed_channels));
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

    int failed = 0;
    for (auto& f : results) {
        if (!f.get().empty())
            failed++;
    }

    CASPAR_LOG(debug) << "Executed batch (" << timer.elapsed() << "s)";

    if (failed > 0)
        return L"202 COMMIT PARTIAL\r\n";
    else
        return L"202 COMMIT OK\r\n";
}

}}} // namespace caspar::protocol::amcp
