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
 
#include "..\StdAfx.h"

#include <string>
#include <sstream>
#include <algorithm>
#include "CLKProtocolStrategy.h"
#include "..\CG\CGControl.h"
#include "..\producers\flash\FlashManager.h"
#include "..\application.h"
#include "..\fileinfo.h"

namespace caspar {
namespace CLK {

using namespace utils;

CLKProtocolStrategy::CLKProtocolStrategy() : currentState_(ExpectingNewCommand), bClockLoaded_(false) {
	pChannel_ = GetApplication()->GetChannel(0);
	pCGControl_ = pChannel_->GetCGControl();
}

CLKProtocolStrategy::~CLKProtocolStrategy() {
}

void CLKProtocolStrategy::Parse(const TCHAR* pData, int charCount, caspar::IO::ClientInfoPtr pClientInfo) {
	for(int index=0; index < charCount; ++index) {
		if(currentState_ == ExpectingNewCommand)
			currentCommandString_.str(TEXT(""));

		TCHAR currentByte = pData[index];
		if(currentByte < 32)
			currentCommandString_ << TEXT("<") << (int)currentByte << TEXT(">");
		else
			currentCommandString_ << currentByte;

		if(currentByte != 0) {
			switch(currentState_) {
				case ExpectingNewCommand:
					if(currentByte == 1) {
						currentState_ = ExpectingCommand;
					}
					//just throw anything else away
					break;

				case ExpectingCommand:
					if(currentByte == 2) {
						if(!currentCommand_.SetCommand()) {
							LOG << TEXT("CLK: Failed to interpret command");
							currentState_ = ExpectingNewCommand;
							currentCommand_.Clear();
						}
						else {
							currentState_ = ExpectingClockID;
						}
					}
					else
						currentCommand_.commandString_ += currentByte;
					break;

				case ExpectingClockID:
					if(currentByte == 2)
						currentState_ = currentCommand_.NeedsTime() ? ExpectingTime : ExpectingParameter;
					else
						currentCommand_.clockID_ = currentByte - TCHAR('0');
					break;

				case ExpectingTime:
					if(currentByte == 2)
						currentState_ = ExpectingParameter;
					else
						currentCommand_.time_ += currentByte;
					break;

				case ExpectingParameter:
					//allocate new parameter
					if(currentCommand_.parameters_.size() == 0 || currentByte == 2)
						currentCommand_.parameters_.push_back(tstring());

					//add the character to end end of the last parameter
					if(currentByte == TEXT('<'))
						currentCommand_.parameters_[currentCommand_.parameters_.size()-1] += TEXT("&lt;");
					else if(currentByte == TEXT('>'))
						currentCommand_.parameters_[currentCommand_.parameters_.size()-1] += TEXT("&gt;");
					else if(currentByte == TEXT('\"'))
						currentCommand_.parameters_[currentCommand_.parameters_.size()-1] += TEXT("&quot;");
					else
						currentCommand_.parameters_[currentCommand_.parameters_.size()-1] += currentByte;

					break;
			}
		}
		else {
			if(currentState_ == ExpectingCommand) {
				if(!currentCommand_.SetCommand())
					LOG << TEXT("CLK: Failed to interpret command");
			}

			if(currentCommand_.command_ == CLKCommand::CLKReset) {
				pCGControl_->Clear();
				bClockLoaded_ = false;
				
				LOG << TEXT("CLK: Recieved and executed reset-command");
			}
			else if(currentCommand_.command_ != CLKCommand::CLKInvalidCommand) {
				if(!bClockLoaded_) {
					pCGControl_->Add(0, TEXT("hawrysklocka/clock"), true, TEXT(""), currentCommand_.GetData());
					bClockLoaded_ = true;
				}
				else {
					pCGControl_->Update(0, currentCommand_.GetData());
				}

				LOG << LogLevel::Debug << TEXT("CLK: Clockdata sent: ") << currentCommand_.GetData();
				LOG << TEXT("CLK: Executed valid command: ") << currentCommandString_.str();
			}

			currentState_ = ExpectingNewCommand;
			currentCommand_.Clear();
		}
	}
}

}	//namespace CLK
}	//namespace caspar