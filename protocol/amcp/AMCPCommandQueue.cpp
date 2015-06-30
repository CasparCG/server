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

#include <common/timer.h>

namespace caspar { namespace protocol { namespace amcp {
	
AMCPCommandQueue::AMCPCommandQueue() 
	: executor_(L"AMCPCommandQueue")
{
}

AMCPCommandQueue::~AMCPCommandQueue() 
{
}

void AMCPCommandQueue::AddCommand(AMCPCommand::ptr_type pCurrentCommand)
{
	if(!pCurrentCommand)
		return;
	
	if(executor_.size() > 128)
	{
		try
		{
			CASPAR_LOG(error) << "AMCP Command Queue Overflow.";
			CASPAR_LOG(error) << "Failed to execute command:" << pCurrentCommand->print();
			pCurrentCommand->SetReplyString(L"500 FAILED\r\n");
			pCurrentCommand->SendReply();
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
		}
	}
	
	executor_.begin_invoke([=]
	{
		try
		{
			caspar::timer timer;

			try
			{
				if(pCurrentCommand->Execute()) 
					CASPAR_LOG(debug) << "Executed command: " << pCurrentCommand->print() << " " << timer.elapsed();
				else 
					CASPAR_LOG(warning) << "Failed to execute command: " << pCurrentCommand->print() << " " << timer.elapsed();
			}
			catch (file_not_found&)
			{
				CASPAR_LOG(error) << L"File not found. No match found for parameters. Check syntax.";
				pCurrentCommand->SetReplyString(L"404 " + pCurrentCommand->print() + L" FAILED\r\n");
			}
			catch (std::out_of_range&)
			{
				CASPAR_LOG(error) << L"Missing parameter. Check syntax.";
				pCurrentCommand->SetReplyString(L"402 " + pCurrentCommand->print() + L" FAILED\r\n");
			}
			catch (...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
				CASPAR_LOG(warning) << "Failed to execute command:" << pCurrentCommand->print() << " " << timer.elapsed();
				pCurrentCommand->SetReplyString(L"501 " + pCurrentCommand->print() + L" FAILED\r\n");
			}
				
			pCurrentCommand->SendReply();
			
			CASPAR_LOG(trace) << "Ready for a new command";
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
		}
	});
}

}}}
