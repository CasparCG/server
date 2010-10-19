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
 
#pragma once
#include <vector>

#include "CLKCommand.h"
#include "..\io\ProtocolStrategy.h"
#include "..\channel.h"
#include "..\MediaManager.h"

namespace caspar {
	namespace CG { class ICGControl; }
namespace CLK {

class CLKProtocolStrategy : public caspar::IO::IProtocolStrategy
{
public:
	CLKProtocolStrategy();
	virtual ~CLKProtocolStrategy();

	virtual void Parse(const TCHAR* pData, int charCount, caspar::IO::ClientInfoPtr pClientInfo);
	virtual UINT GetCodepage() { return CP_UTF8; }

private:
	enum ParserState {
		ExpectingNewCommand,
		ExpectingCommand,
		ExpectingClockID,
		ExpectingTime,
		ExpectingParameter
	};

	ParserState	currentState_;
	CLKCommand currentCommand_;
	tstringstream currentCommandString_;

	caspar::MediaManagerPtr pFlashManager_;
	caspar::CG::ICGControl* pCGControl_;
	ChannelPtr pChannel_;

	bool bClockLoaded_;
};

}	//namespace CLK
}	//namespace caspar
