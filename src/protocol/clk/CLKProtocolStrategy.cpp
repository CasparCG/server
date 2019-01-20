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

#include "CLKProtocolStrategy.h"
#include "clk_commands.h"

#include <boost/algorithm/string/case_conv.hpp>
#include <sstream>
#include <string>
#include <vector>

namespace caspar { namespace protocol { namespace CLK {

class CLKProtocolStrategy : public IO::protocol_strategy<wchar_t>
{
    enum class ParserState
    {
        ExpectingNewCommand,
        ExpectingCommand,
        ExpectingParameter
    };

    ParserState                         currentState_ = ParserState::ExpectingNewCommand;
    std::wstringstream                  currentCommandString_;
    std::wstring                        command_name_;
    std::vector<std::wstring>           parameters_;
    clk_command_processor&              command_processor_;
    IO::client_connection<wchar_t>::ptr client_connection_;

  public:
    CLKProtocolStrategy(const IO::client_connection<wchar_t>::ptr& client_connection,
                        clk_command_processor&                     command_processor)
        : command_processor_(command_processor)
        , client_connection_(client_connection)
    {
    }

    void parse(const std::basic_string<wchar_t>& data) override
    {
        for (int index = 0; index < data.length(); ++index) {
            wchar_t currentByte = data[index];

            if (currentByte < 32)
                currentCommandString_ << L"<" << static_cast<int>(currentByte) << L">";
            else
                currentCommandString_ << currentByte;

            if (currentByte != 0) {
                switch (currentState_) {
                    case ParserState::ExpectingNewCommand:
                        if (currentByte == 1)
                            currentState_ = ParserState::ExpectingCommand;
                        // just throw anything else away
                        break;
                    case ParserState::ExpectingCommand:
                        if (currentByte == 2)
                            currentState_ = ParserState::ExpectingParameter;
                        else
                            command_name_ += currentByte;
                        break;
                    case ParserState::ExpectingParameter:
                        // allocate new parameter
                        if (parameters_.size() == 0 || currentByte == 2)
                            parameters_.push_back(std::wstring());

                        // add the character to end end of the last parameter
                        if (currentByte != 2) {
                            // add the character to end end of the last parameter
                            if (currentByte == L'<')
                                parameters_.back() += L"&lt;";
                            else if (currentByte == L'>')
                                parameters_.back() += L"&gt;";
                            else if (currentByte == L'\"')
                                parameters_.back() += L"&quot;";
                            else
                                parameters_.back() += currentByte;
                        }

                        break;
                }
            } else {
                boost::to_upper(command_name_);

                try {
                    if (!command_processor_.handle(command_name_, parameters_))
                        CASPAR_LOG(error) << "CLK: Unknown command: " << command_name_;
                    else
                        CASPAR_LOG(info) << L"CLK: Executed valid command: " << currentCommandString_.str();
                } catch (...) {
                    CASPAR_LOG_CURRENT_EXCEPTION();
                    CASPAR_LOG(error) << "CLK: Failed to interpret command: " << currentCommandString_.str();
                }

                reset();
            }
        }
    }

  private:
    void reset()
    {
        currentState_ = ParserState::ExpectingNewCommand;
        currentCommandString_.str(L"");
        command_name_.clear();
        parameters_.clear();
    }
};

clk_protocol_strategy_factory::clk_protocol_strategy_factory(
    const std::vector<spl::shared_ptr<core::video_channel>>&    channels,
    const spl::shared_ptr<core::cg_producer_registry>&          cg_registry,
    const spl::shared_ptr<const core::frame_producer_registry>& producer_registry)
{
    add_command_handlers(command_processor_, channels, channels.at(0), cg_registry, producer_registry);
}

IO::protocol_strategy<wchar_t>::ptr
clk_protocol_strategy_factory::create(const IO::client_connection<wchar_t>::ptr& client_connection)
{
    return spl::make_shared<CLKProtocolStrategy>(client_connection, command_processor_);
}

}}} // namespace caspar::protocol::CLK
