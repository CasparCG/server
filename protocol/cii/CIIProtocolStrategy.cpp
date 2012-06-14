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

#include <string>
#include <sstream>
#include <algorithm>
#include "CIIProtocolStrategy.h"
#include "CIICommandsimpl.h"
#include <modules/flash/producer/flash_producer.h>
#include <core/producer/transition/transition_producer.h>
#include <core/mixer/mixer.h>
#include <common/env.h>

#include <boost/algorithm/string/replace.hpp>

#if defined(_MSC_VER)
#pragma warning (push, 1) // TODO: Legacy code, just disable warnings
#endif

namespace caspar { namespace protocol { namespace cii {
	
using namespace core;

const std::wstring CIIProtocolStrategy::MessageDelimiter = TEXT("\r\n");
const TCHAR CIIProtocolStrategy::TokenDelimiter = TEXT('\\');

CIIProtocolStrategy::CIIProtocolStrategy(const std::vector<spl::shared_ptr<core::video_channel>>& channels) : pChannel_(channels.at(0)), executor_(L"CIIProtocolStrategy")
{
}

void CIIProtocolStrategy::Parse(const TCHAR* pData, int charCount, IO::ClientInfoPtr pClientInfo) 
{
	std::size_t pos;
	std::wstring msg(pData, charCount);
	std::wstring availibleData = currentMessage_ + msg;

	while(true)
	{
		pos = availibleData.find(MessageDelimiter);
		if(pos != std::wstring::npos)
		{
			std::wstring message = availibleData.substr(0,pos);

			if(message.length() > 0) {
				ProcessMessage(message, pClientInfo);
				if(pClientInfo != 0)
					pClientInfo->Send(TEXT("*\r\n"));
			}

			std::size_t nextStartPos = pos + MessageDelimiter.length();
			if(nextStartPos < availibleData.length())
				availibleData = availibleData.substr(nextStartPos);
			else 
			{
				availibleData.clear();
				break;
			}
		}
		else
			break;
	}
	currentMessage_ = availibleData;
}

void CIIProtocolStrategy::ProcessMessage(const std::wstring& message, IO::ClientInfoPtr pClientInfo)
{	
	CASPAR_LOG(info) << L"Received message from " << pClientInfo->print() << ": " << message << L"\\r\\n";

	std::vector<std::wstring> tokens;
	int tokenCount = TokenizeMessage(message, &tokens);

	CIICommandPtr pCommand = Create(tokens[0]);
	if((pCommand != 0) && (tokenCount-1) >= pCommand->GetMinimumParameters()) 
	{
		pCommand->Setup(tokens);
		executor_.begin_invoke([=]{pCommand->Execute();});
	}
	else {}	//report error	
}

int CIIProtocolStrategy::TokenizeMessage(const std::wstring& message, std::vector<std::wstring>* pTokenVector)
{
	std::wstringstream currentToken;

	for(unsigned int charIndex=0; charIndex<message.size(); ++charIndex) 
	{
		if(message[charIndex] == TokenDelimiter) 
		{
			pTokenVector->push_back(currentToken.str());
			currentToken.str(TEXT(""));
			continue;
		}

		if(message[charIndex] == TEXT('\"')) 
			currentToken << TEXT("&quot;");		
		else if(message[charIndex] == TEXT('<')) 
			currentToken << TEXT("&lt;");		
		else if(message[charIndex] == TEXT('>')) 
			currentToken << TEXT("&gt;");		
		else 
			currentToken << message[charIndex];
	}

	if(currentToken.str().size() > 0)
		pTokenVector->push_back(currentToken.str());	

	return (int)pTokenVector->size();
}

/************
// Examples (<X> = ASCIICHAR X)

I\25\3\VII\\									sätter outputtype till 'vii'
I\25\4\1\\										enablar framebuffer (ignore this)

M\C/SVTNEWS\\									pekar ut vilken grafisk profil som skall användas

W\4009\4067\Jonas Björkman\\					Skriver "Jonas Björkman" till första textfältet i template 4067 och sparar den färdiga skylten som 4009

T\7\4009.VII\A\\								lägger ut skylt 4009

Y\<205><247><202><196><192><192><200><248>\\	lägger ut skylten 4008 (<205><247><202><196><192><192><200><248> = "=g:4008h" om man drar bort 144 från varje asciivärde)

V\5\3\1\1\namn.tga\1\\							lägger ut bilden namn.tga
V\0\1\D\C\10\0\0\0\\							gör någon inställning som har med föregående kommando att göra.

*************/

/**********************
New Commands to support the Netupe automation system
V\5\13\1\1\Template\0\TabField1\TabField2...\\		Build. Ettan före Template indikerar vilket lager den nya templaten skall laddas in i. OBS. Skall inte visas efter det här steget
Y\<27>\\											Stop. Här kommer ett lagerID också att skickas med (<27> = ESC)
Y\<254>\\											Clear Canvas. Här kommer ett lagerID också att skickas med, utan det skall allt tömmas
Y\<213><243>\\										Play. Här kommer ett lagerID också att skickas med

**********************/
CIICommandPtr CIIProtocolStrategy::Create(const std::wstring& name)
{
	switch(name[0])
	{
		case TEXT('M'): return std::make_shared<MediaCommand>(this);
		case TEXT('W'): return std::make_shared<WriteCommand>(this);
		case TEXT('T'): return std::make_shared<ImagestoreCommand>(this);
		case TEXT('V'): return std::make_shared<MiscellaneousCommand>(this);
		case TEXT('Y'): return std::make_shared<KeydataCommand>(this);
		default:		return nullptr;
	}
}

void CIIProtocolStrategy::WriteTemplateData(const std::wstring& templateName, const std::wstring& titleName, const std::wstring& xmlData) 
{
	std::wstring fullTemplateFilename = env::template_folder();
	if(currentProfile_.size() > 0)
	{
		fullTemplateFilename += currentProfile_;
		fullTemplateFilename += TEXT("\\");
	}
	fullTemplateFilename += templateName;
	fullTemplateFilename = flash::find_template(fullTemplateFilename);
	if(fullTemplateFilename.empty())
	{
		CASPAR_LOG(error) << "Failed to save instance of " << templateName << TEXT(" as ") << titleName << TEXT(", template ") << fullTemplateFilename << " not found";
		return;
	}
	
	auto producer = flash::create_producer(this->GetChannel()->frame_factory(), this->GetChannel()->video_format_desc(), boost::assign::list_of(env::template_folder()+TEXT("CG.fth")));

	std::wstringstream flashParam;
	flashParam << TEXT("<invoke name=\"Add\" returntype=\"xml\"><arguments><number>1</number><string>") << currentProfile_ << '/' <<  templateName << TEXT("</string><number>0</number><true/><string> </string><string><![CDATA[ ") << xmlData << TEXT(" ]]></string></arguments></invoke>");
	producer->call(flashParam.str());

	CASPAR_LOG(info) << "Saved an instance of " << templateName << TEXT(" as ") << titleName ;

	PutPreparedTemplate(titleName, spl::shared_ptr<core::frame_producer>(std::move(producer)));
	
}

void CIIProtocolStrategy::DisplayTemplate(const std::wstring& titleName)
{
	try
	{
		pChannel_->stage().load(0, GetPreparedTemplate(titleName));
		pChannel_->stage().play(0);

		CASPAR_LOG(info) << L"Displayed title " << titleName ;
	}
	catch(caspar_exception&)
	{
		CASPAR_LOG(error) << L"Failed to display title " << titleName;
	}
}

void CIIProtocolStrategy::DisplayMediaFile(const std::wstring& filename) 
{
	transition_info transition;
	transition.type = transition_type::mix;
	transition.duration = 12;

	auto pFP = create_producer(GetChannel()->frame_factory(), GetChannel()->video_format_desc(), filename);
	auto pTransition = create_transition_producer(GetChannel()->video_format_desc().field_mode, pFP, transition);

	try
	{
		pChannel_->stage().load(0, pTransition);
	}
	catch(...)
	{
		CASPAR_LOG_CURRENT_EXCEPTION();
		CASPAR_LOG(error) << L"Failed to display " << filename ;
		return;
	}

	pChannel_->stage().play(0);

	CASPAR_LOG(info) << L"Displayed " << filename;
}

spl::shared_ptr<core::frame_producer> CIIProtocolStrategy::GetPreparedTemplate(const std::wstring& titleName)
{
	spl::shared_ptr<core::frame_producer> result(frame_producer::empty());

	TitleList::iterator it = std::find(titles_.begin(), titles_.end(), titleName);
	if(it != titles_.end()) {
		CASPAR_LOG(debug) << L"Found title with name " << it->titleName;
		result = (*it).pframe_producer;
	}
	else 
		CASPAR_LOG(error) << L"Could not find title with name " << titleName;

	return result;
}

void CIIProtocolStrategy::PutPreparedTemplate(const std::wstring& titleName, spl::shared_ptr<core::frame_producer>& pFP)
{
	CASPAR_LOG(debug) << L"Saved title with name " << titleName;

	TitleList::iterator it = std::find(titles_.begin(), titles_.end(), titleName);
	if(it != titles_.end()) {
		titles_.remove((*it));
	}

	titles_.push_front(TitleHolder(titleName, pFP));

	if(titles_.size() >= 6)
		titles_.resize(5);
}

bool operator==(const CIIProtocolStrategy::TitleHolder& lhs, const std::wstring& rhs) 
{
	return lhs.titleName == rhs;
}

bool operator==(const std::wstring& lhs, const CIIProtocolStrategy::TitleHolder& rhs)
{
	return lhs == rhs.titleName;
}

}}}
