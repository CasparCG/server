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

#include "AMCPCommand.h"
#include "AMCPCommandQueue.h"
#include "AMCPProtocolStrategy.h"
#include "amcp_command_repository.h"
#include "amcp_shared.h"

#include <algorithm>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/lexical_cast.hpp>

#if defined(_MSC_VER)
#pragma warning(push, 1) // TODO: Legacy code, just disable warnings
#endif

namespace caspar { namespace protocol { namespace amcp {

using IO::ClientInfoPtr;

template <typename Out, typename In>
bool try_lexical_cast(const In& input, Out& result)
{
    Out  saved   = result;
    bool success = boost::conversion::detail::try_lexical_convert(input, result);

    if (!success)
        result = saved; // Needed because of how try_lexical_convert is implemented.

    return success;
}

struct AMCPProtocolStrategy::impl
{
  private:
    std::vector<AMCPCommandQueue::ptr_type>  commandQueues_;
    spl::shared_ptr<amcp_command_repository> repo_;

  public:
    impl(const std::wstring& name, const spl::shared_ptr<amcp_command_repository>& repo)
        : repo_(repo)
    {
        commandQueues_.push_back(spl::make_shared<AMCPCommandQueue>(L"General Queue for " + name));

        for (int i = 0; i < repo_->channels().size(); ++i) {
            commandQueues_.push_back(
                spl::make_shared<AMCPCommandQueue>(L"Channel " + std::to_wstring(i + 1) + L" for " + name));
        }
    }

    ~impl() {}

    enum class error_state
    {
        no_error = 0,
        command_error,
        channel_error,
        parameters_error,
        unknown_error,
        access_error
    };

    struct command_interpreter_result
    {
        std::shared_ptr<caspar::IO::lock_container> lock;
        std::wstring                                request_id;
        std::wstring                                command_name;
        AMCPCommand::ptr_type                       command;
        error_state                                 error = error_state::no_error;
        std::shared_ptr<AMCPCommandQueue>           queue;
    };

    // The paser method expects message to be complete messages with the delimiter stripped away.
    // Thesefore the AMCPProtocolStrategy should be decorated with a delimiter_based_chunking_strategy
    void Parse(const std::wstring& message, ClientInfoPtr client)
    {
        std::list<std::wstring> tokens;
        tokenize(message, tokens);

        if (!tokens.empty() && boost::iequals(tokens.front(), L"PING")) {
            tokens.pop_front();
            std::wstringstream answer;
            answer << L"PONG";

            for (auto t : tokens)
                answer << L" " << t;

            answer << "\r\n";
            client->send(answer.str(), true);
            return;
        }

        CASPAR_LOG(info) << L"Received message from " << client->address() << ": " << message << L"\\r\\n";

        command_interpreter_result result;
        if (interpret_command_string(tokens, result, client)) {
            if (result.lock && !result.lock->check_access(client))
                result.error = error_state::access_error;
            else
                result.queue->AddCommand(result.command);
        }

        if (result.error != error_state::no_error) {
            std::wstringstream answer;

            if (!result.request_id.empty())
                answer << L"RES " << result.request_id << L" ";

            switch (result.error) {
                case error_state::command_error:
                    answer << L"400 ERROR\r\n" << message << "\r\n";
                    break;
                case error_state::channel_error:
                    answer << L"401 " << result.command_name << " ERROR\r\n";
                    break;
                case error_state::parameters_error:
                    answer << L"402 " << result.command_name << " ERROR\r\n";
                    break;
                case error_state::access_error:
                    answer << L"503 " << result.command_name << " FAILED\r\n";
                    break;
                case error_state::unknown_error:
                    answer << L"500 FAILED\r\n";
                    break;
                default:
                    CASPAR_THROW_EXCEPTION(programming_error()
                                           << msg_info(L"Unhandled error_state enum constant " +
                                                       std::to_wstring(static_cast<int>(result.error))));
            }
            client->send(answer.str());
        }
    }

  private:
    bool
    interpret_command_string(std::list<std::wstring> tokens, command_interpreter_result& result, ClientInfoPtr client)
    {
        try {
            // Discard GetSwitch
            if (!tokens.empty() && tokens.front().at(0) == L'/')
                tokens.pop_front();

            if (!tokens.empty() && boost::iequals(tokens.front(), L"REQ")) {
                tokens.pop_front();

                if (tokens.empty()) {
                    result.error = error_state::parameters_error;
                    return false;
                }

                result.request_id = tokens.front();
                tokens.pop_front();
            }

            // Fail if no more tokens.
            if (tokens.empty()) {
                result.error = error_state::command_error;
                return false;
            }

            // Consume command name
            result.command_name = boost::to_upper_copy(tokens.front());
            tokens.pop_front();

            // Determine whether the next parameter is a channel spec or not
            int          channel_index = -1;
            int          layer_index   = -1;
            std::wstring channel_spec;

            if (!tokens.empty()) {
                channel_spec                            = tokens.front();
                std::wstring              channelid_str = boost::trim_copy(channel_spec);
                std::vector<std::wstring> split;
                boost::split(split, channelid_str, boost::is_any_of("-"));

                // Use non_throwing lexical cast to not hit exception break point all the time.
                if (try_lexical_cast(split[0], channel_index)) {
                    --channel_index;

                    if (split.size() > 1)
                        try_lexical_cast(split[1], layer_index);

                    // Consume channel-spec
                    tokens.pop_front();
                }
            }

            bool is_channel_command = channel_index != -1;

            // Create command instance
            if (is_channel_command) {
                result.command =
                    repo_->create_channel_command(result.command_name, client, channel_index, layer_index, tokens);

                if (result.command) {
                    result.lock  = repo_->channels().at(channel_index).lock;
                    result.queue = commandQueues_.at(channel_index + 1);
                } else // Might be a non channel command, although the first argument is numeric
                {
                    // Restore backed up channel spec string.
                    tokens.push_front(channel_spec);
                    result.command = repo_->create_command(result.command_name, client, tokens);

                    if (result.command)
                        result.queue = commandQueues_.at(0);
                }
            } else {
                result.command = repo_->create_command(result.command_name, client, tokens);

                if (result.command)
                    result.queue = commandQueues_.at(0);
            }

            if (!result.command)
                result.error = error_state::command_error;
            else {
                std::vector<std::wstring> parameters(tokens.begin(), tokens.end());

                result.command->parameters() = std::move(parameters);

                if (result.command->parameters().size() < result.command->minimum_parameters())
                    result.error = error_state::parameters_error;
            }

            if (result.command)
                result.command->set_request_id(result.request_id);
        } catch (std::out_of_range&) {
            CASPAR_LOG(error) << "Invalid channel specified.";
            result.error = error_state::channel_error;
        } catch (...) {
            CASPAR_LOG_CURRENT_EXCEPTION();
            result.error = error_state::unknown_error;
        }

        return result.error == error_state::no_error;
    }

    template <typename C>
    std::size_t tokenize(const std::wstring& message, C& pTokenVector)
    {
        // split on whitespace but keep strings within quotationmarks
        // treat \ as the start of an escape-sequence: the following char will indicate what to actually put in the
        // string

        std::wstring currentToken;

        bool inQuote        = false;
        bool getSpecialCode = false;

        for (unsigned int charIndex = 0; charIndex < message.size(); ++charIndex) {
            if (getSpecialCode) {
                // insert code-handling here
                switch (message[charIndex]) {
                    case L'\\':
                        currentToken += L"\\";
                        break;
                    case L'\"':
                        currentToken += L"\"";
                        break;
                    case L'n':
                        currentToken += L"\n";
                        break;
                    default:
                        break;
                }
                getSpecialCode = false;
                continue;
            }

            if (message[charIndex] == L'\\') {
                getSpecialCode = true;
                continue;
            }

            if (message[charIndex] == L' ' && inQuote == false) {
                if (!currentToken.empty()) {
                    pTokenVector.push_back(currentToken);
                    currentToken.clear();
                }
                continue;
            }

            if (message[charIndex] == L'\"') {
                inQuote = !inQuote;

                if (!currentToken.empty() || !inQuote) {
                    pTokenVector.push_back(currentToken);
                    currentToken.clear();
                }
                continue;
            }

            currentToken += message[charIndex];
        }

        if (!currentToken.empty()) {
            pTokenVector.push_back(currentToken);
            currentToken.clear();
        }

        return pTokenVector.size();
    }
};

AMCPProtocolStrategy::AMCPProtocolStrategy(const std::wstring&                             name,
                                           const spl::shared_ptr<amcp_command_repository>& repo)
    : impl_(spl::make_unique<impl>(name, repo))
{
}
AMCPProtocolStrategy::~AMCPProtocolStrategy() {}
void AMCPProtocolStrategy::Parse(const std::wstring& msg, IO::ClientInfoPtr pClientInfo)
{
    impl_->Parse(msg, pClientInfo);
}

}}} // namespace caspar::protocol::amcp
