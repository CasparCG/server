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
#include "CIIProtocolStrategy.h"
#include "CIICommandsimpl.h"
#include "..\producers\flash\FlashManager.h"
#include "..\application.h"
#include "..\fileinfo.h"

namespace caspar {
namespace cii {

using namespace utils;

const tstring CIIProtocolStrategy::MessageDelimiter = TEXT("\r\n");
const TCHAR CIIProtocolStrategy::TokenDelimiter = TEXT('\\');

CIIProtocolStrategy::CIIProtocolStrategy() {
	pChannel_ = GetApplication()->GetChannel(0);
	pCGControl_ = pChannel_->GetCGControl();

	if(!commandQueue_.Start()) {
		//throw
	}
}

CIIProtocolStrategy::~CIIProtocolStrategy() {
}

void CIIProtocolStrategy::Parse(const TCHAR* pData, int charCount, caspar::IO::ClientInfoPtr pClientInfo) {
	std::size_t pos;
	tstring msg(pData, charCount);
	tstring availibleData = currentMessage_ + msg;

	while(true) {
		pos = availibleData.find(MessageDelimiter);
		if(pos != tstring::npos)
		{
			tstring message = availibleData.substr(0,pos);

			if(message.length() > 0) {
				ProcessMessage(message);
				if(pClientInfo != 0)
					pClientInfo->Send(TEXT("*\r\n"));
			}

			std::size_t nextStartPos = pos + MessageDelimiter.length();
			if(nextStartPos < availibleData.length())
				availibleData = availibleData.substr(nextStartPos);
			else {
				availibleData.clear();
				break;
			}
		}
		else
			break;
	}
	currentMessage_ = availibleData;
}

void CIIProtocolStrategy::ProcessMessage(const tstring& message) {
	LOG << message.c_str() << LogStream::Flush;

	std::vector<tstring> tokens;
	int tokenCount = TokenizeMessage(message, &tokens);

	CIICommandPtr pCommand = Create(tokens[0]);
	if((pCommand != 0) && (tokenCount-1) >= pCommand->GetMinimumParameters()) {
		pCommand->Setup(tokens);
		commandQueue_.AddCommand(pCommand);
	}
	else {
		//report error
	}
}

int CIIProtocolStrategy::TokenizeMessage(const tstring& message, std::vector<tstring>* pTokenVector)
{
	tstringstream currentToken;

	for(unsigned int charIndex=0; charIndex<message.size(); ++charIndex) {
		if(message[charIndex] == TokenDelimiter) {
			pTokenVector->push_back(currentToken.str());
			currentToken.str(TEXT(""));
			continue;
		}

		if(message[charIndex] == TEXT('\"')) {
			currentToken << TEXT("&quot;");
		}
		else if(message[charIndex] == TEXT('<')) {
			currentToken << TEXT("&lt;");
		}
		else if(message[charIndex] == TEXT('>')) {
			currentToken << TEXT("&gt;");
		}
		else 
			currentToken << message[charIndex];
	}

	if(currentToken.str().size() > 0) {
		pTokenVector->push_back(currentToken.str());
	}

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
CIICommandPtr CIIProtocolStrategy::Create(const tstring& name) {
	CIICommandPtr result;

	switch(name[0]) {
		case TEXT('M'):
			result = CIICommandPtr(new MediaCommand(this));
			break;
		case TEXT('W'):
			result = CIICommandPtr(new WriteCommand(this));
			break;
		case TEXT('T'):
			result = CIICommandPtr(new ImagestoreCommand(this));
			break;
		case TEXT('V'):
			result = CIICommandPtr(new MiscellaneousCommand(this));
			break;
		case TEXT('Y'):
			result = CIICommandPtr(new KeydataCommand(this));
			break;

		default:
			break;
	}

	return result;
}

void CIIProtocolStrategy::WriteTemplateData(const tstring& templateName, const tstring& titleName, const tstring& xmlData) {
	tstring fullTemplateFilename = GetApplication()->GetTemplateFolder();
	if(currentProfile_.size() > 0)
	{
		fullTemplateFilename += currentProfile_;
		fullTemplateFilename += TEXT("\\");
	}
	fullTemplateFilename += templateName;

	if(!GetApplication()->FindTemplate(fullTemplateFilename))
	{
		LOG << TEXT("Failed to save instance of ") << templateName << TEXT(" as ") << titleName << TEXT(", template ") << fullTemplateFilename << TEXT(" not found") << LogStream::Flush;
		return;
	}

	MediaProducerPtr pFP = pFlashManager_->CreateProducer(GetApplication()->GetTemplateFolder()+TEXT("CG.fth"));
	if(pFP != 0)
	{
		//TODO: Initialize with valid FrameFactory
//		pFP->Initialize(0, false);

		tstringstream flashParam;
		flashParam << TEXT("<invoke name=\"Add\" returntype=\"xml\"><arguments><number>1</number><string>") << currentProfile_ << '/' <<  templateName << TEXT("</string><number>0</number><true/><string> </string><string><![CDATA[ ") << xmlData << TEXT(" ]]></string></arguments></invoke>");
		pFP->Param(flashParam.str());

		LOG << LogLevel::Verbose << TEXT("Saved an instance of ") << templateName << TEXT(" as ") << titleName << LogStream::Flush;

		PutPreparedTemplate(titleName, pFP);
	}
}

void CIIProtocolStrategy::DisplayTemplate(const tstring& titleName) {
	MediaProducerPtr pFP = GetPreparedTemplate(titleName);
	if(pFP != 0)
	{
		TransitionInfo transition;
		if(pChannel_->LoadBackground(pFP, transition)) {
			pChannel_->Play();

			LOG << LogLevel::Verbose << TEXT("Displayed title ") << titleName << LogStream::Flush;
			return;
		}
	}
	LOG << TEXT("Failed to display title ") << titleName << LogStream::Flush;
}

void CIIProtocolStrategy::DisplayMediaFile(const tstring& filename) {
	caspar::FileInfo fileInfo;
	MediaManagerPtr pMediaManager = GetApplication()->FindMediaFile(GetApplication()->GetMediaFolder()+filename, &fileInfo);
	if(pMediaManager != 0)
	{
		tstring fullFilename = filename;
		if(fileInfo.filetype.length()>0)
		{
			fullFilename += TEXT(".");
			fullFilename += fileInfo.filetype;
		}

		MediaProducerPtr pFP = pMediaManager->CreateProducer(GetApplication()->GetMediaFolder()+fullFilename);
		if(pFP != 0)
		{
			caspar::TransitionInfo transition;
			transition.type_ = Mix;
			transition.duration_ = 12;
			if(pChannel_->LoadBackground(pFP, transition)){
				pChannel_->Play();

				LOG << LogLevel::Verbose << TEXT("Displayed ") << fullFilename << LogStream::Flush;
				return;
			}
		}
	}
	LOG << TEXT("Failed to display ") << filename << LogStream::Flush;
}

MediaProducerPtr CIIProtocolStrategy::GetPreparedTemplate(const tstring& titleName) {
	MediaProducerPtr result;

	TitleList::iterator it = std::find(titles_.begin(), titles_.end(), titleName);
	if(it != titles_.end()) {
		LOG << LogLevel::Debug << TEXT("Found title with name ") << (*it).titleName << LogStream::Flush;
		result = (*it).pMediaProducer;
	}
	else {
		LOG << TEXT("Could not find title with name ") << titleName << LogStream::Flush;
	}

	return result;
}

void CIIProtocolStrategy::PutPreparedTemplate(const tstring& titleName, MediaProducerPtr& pFP) {
	LOG << LogLevel::Debug << TEXT("Saved title with name ") << titleName << LogStream::Flush;

	TitleList::iterator it = std::find(titles_.begin(), titles_.end(), titleName);
	if(it != titles_.end()) {
		titles_.remove((*it));
	}

	titles_.push_front(TitleHolder(titleName, pFP));

	if(titles_.size() >= 6)
		titles_.resize(5);
}

bool operator==(const CIIProtocolStrategy::TitleHolder& lhs, const tstring& rhs) {
	return lhs.titleName == rhs;
}
bool operator==(const tstring& lhs, const CIIProtocolStrategy::TitleHolder& rhs) {
	return lhs == rhs.titleName;
}

}	//namespace cii
}	//namespace caspar
