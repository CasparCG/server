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

 
#include "..\stdafx.h"

#include "CLKProtocolStrategy.h"
#include "clk_commands.h"

#include <modules/flash/producer/cg_producer.h>

#include <string>
#include <algorithm>
#include <locale>

namespace caspar { namespace protocol { namespace CLK {
	
CLKProtocolStrategy::CLKProtocolStrategy(
	const std::vector<safe_ptr<core::video_channel>>& channels) 
	: currentState_(ExpectingNewCommand)
{
	add_command_handlers(command_processor_, channels.at(0));
}

void CLKProtocolStrategy::reset()
{
	currentState_ = ExpectingNewCommand;
	currentCommandString_.str(L"");
	command_name_.clear();
	parameters_.clear();
}

void CLKProtocolStrategy::Parse(const TCHAR* pData, int charCount, IO::ClientInfoPtr pClientInfo) 
{
	for (int index = 0; index < charCount; ++index) 
	{
		TCHAR currentByte = pData[index];
		
		if (currentByte < 32)
			currentCommandString_ << L"<" << static_cast<int>(currentByte) << L">";
		else
			currentCommandString_ << currentByte;

		if (currentByte != 0)
		{
			switch (currentState_)
			{
				case ExpectingNewCommand:
					if (currentByte == 1) 
						currentState_ = ExpectingCommand;
					//just throw anything else away
					break;
				case ExpectingCommand:
					if (currentByte == 2) 
						currentState_ = ExpectingParameter;
					else
						command_name_ += currentByte;
					break;
				case ExpectingParameter:
					//allocate new parameter
					if (parameters_.size() == 0 || currentByte == 2)
						parameters_.push_back(std::wstring());

					if (currentByte != 2)
					{
						//add the character to end end of the last parameter
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
		}
		else 
		{
			std::transform(
				command_name_.begin(), command_name_.end(), 
				command_name_.begin(), 
				toupper);

			try
			{
				if (!command_processor_.handle(command_name_, parameters_))
				{
					CASPAR_LOG(error) << "CLK: Unknown command: " << command_name_;
				}
				else
				{
					CASPAR_LOG(debug) << L"CLK: Executed valid command: " 
						<< currentCommandString_.str();
				}
			} 
			catch (...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
				CASPAR_LOG(error) << "CLK: Failed to interpret command: " 
					<< currentCommandString_.str();
			}

			reset();
		}
	}
}

}}}
