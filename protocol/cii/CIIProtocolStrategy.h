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

 
#pragma once

#include <core/video_channel.h>

#include "../util/ProtocolStrategy.h"
#include "CIICommand.h"

#include <core/producer/stage.h>

#include <common/concurrency/executor.h>

namespace caspar { namespace protocol { namespace cii {

class CIIProtocolStrategy : public IO::IProtocolStrategy
{
public:
	CIIProtocolStrategy(const std::vector<safe_ptr<core::video_channel>>& channels);

	void Parse(const TCHAR* pData, int charCount, IO::ClientInfoPtr pClientInfo);
	UINT GetCodepage() {return 28591;}	//ISO 8859-1

	void SetProfile(const std::wstring& profile) {currentProfile_ = profile;}

	safe_ptr<core::video_channel> GetChannel() const{return this->pChannel_;}

	void DisplayMediaFile(const std::wstring& filename);
	void DisplayTemplate(const std::wstring& titleName);
	void WriteTemplateData(const std::wstring& templateName, const std::wstring& titleName, const std::wstring& xmlData);

public:
	struct TitleHolder
	{
		TitleHolder() : titleName(TEXT("")), pframe_producer(core::frame_producer::empty())	{}
		TitleHolder(const std::wstring& name, safe_ptr<core::frame_producer> pFP) : titleName(name), pframe_producer(pFP) {}
		TitleHolder(const TitleHolder& th) : titleName(th.titleName), pframe_producer(th.pframe_producer) {}
		const TitleHolder& operator=(const TitleHolder& th) 
		{
			titleName = th.titleName;
			pframe_producer = th.pframe_producer;
		}
		bool operator==(const TitleHolder& rhs) 
		{
			return pframe_producer == rhs.pframe_producer;
		}

		std::wstring titleName;
		safe_ptr<core::frame_producer> pframe_producer;
		friend CIIProtocolStrategy;
	};
private:

	typedef std::list<TitleHolder> TitleList;
	TitleList titles_;
	safe_ptr<core::frame_producer> GetPreparedTemplate(const std::wstring& name);
	void PutPreparedTemplate(const std::wstring& name, safe_ptr<core::frame_producer>& pframe_producer);

	static const TCHAR TokenDelimiter;
	static const std::wstring MessageDelimiter;

	void ProcessMessage(const std::wstring& message, IO::ClientInfoPtr pClientInfo);
	int TokenizeMessage(const std::wstring& message, std::vector<std::wstring>* pTokenVector);
	CIICommandPtr Create(const std::wstring& name);

	executor executor_;
	std::wstring currentMessage_;

	std::wstring currentProfile_;
	safe_ptr<core::video_channel> pChannel_;
};

}}}