/*
* copyright (c) 2010 Sveriges Television AB <info@casparcg.com>
*
*  This file is part of CasparCG.
*
*    CasparCG is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    CasparCG is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.

*    You should have received a copy of the GNU General Public License
*    along with CasparCG.  If not, see <http://www.gnu.org/licenses/>.
*
*/
 
#include "..\..\stdafx.h"
#include <algorithm>
#include <locale>
#include "CLKCommand.h"

namespace caspar { namespace CLK {

CLKCommand::CLKCommand() : clockID_(0), command_(CLKInvalidCommand) {}

CLKCommand::~CLKCommand() {}

const std::wstring& CLKCommand::GetData() 
{
	std::wstringstream dataStream;

	dataStream << TEXT("<templateData>");	
	dataStream << TEXT("<componentData id=\"command\">");
	dataStream << TEXT("<command id=\"") << commandString_ << TEXT("\" time=\"") << time_ << TEXT("\" clockID=\"") << clockID_ << TEXT("\">");

	std::vector<std::wstring>::const_iterator it = parameters_.begin();
	std::vector<std::wstring>::const_iterator end = parameters_.end();
	for(; it != end; ++it) {
		dataStream << TEXT("<parameter>") << (*it) << TEXT("</parameter>"); 
	}

	dataStream << TEXT("</command>"); 
	dataStream << TEXT("</componentData>"); 
	dataStream << TEXT("</templateData>");

	dataCache_ = dataStream.str();
	return dataCache_;
}

bool CLKCommand::SetCommand() 
{
	bool bResult = true;
	std::transform(commandString_.begin(), commandString_.end(), commandString_.begin(), toupper);

	if(commandString_ == TEXT("DUR"))
		command_ = CLKDuration;
	else if(commandString_ == TEXT("NEWDUR"))
		command_ = CLKNewDuration;
	else if(commandString_ == TEXT("NEXTEVENT"))
		command_ = CLKNextEvent;
	else if(commandString_ == TEXT("STOP"))
		command_ = CLKStop;
	else if(commandString_ == TEXT("UNTIL"))
		command_ = CLKUntil;
	else if(commandString_ == TEXT("ADD"))
		command_ = CLKAdd;
	else if(commandString_ == TEXT("SUB"))
		command_ = CLKSub;
	else if(commandString_ == TEXT("RESET"))
		command_ = CLKReset;
	else 
	{
		command_ = CLKInvalidCommand;
		bResult = false;
	}

	return bResult;
}

void CLKCommand::Clear() 
{
	dataCache_.clear();
	commandString_.clear();
	time_.clear();
	command_ = CLKDuration;
	clockID_ = 0;
	parameters_.clear();
}

}}