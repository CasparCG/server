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

#include "..\Producers\flash\FlashManager.h"
#include "..\Producers\flash\FlashProducer.h"

#include "..\Application.h"
#include "..\transitioninfo.h"

#include "FlashCGManager.h"

namespace caspar {
namespace CG {

using namespace utils;

FlashCGManager::FlashCGManager(caspar::Channel* pChannel) : pChannel_(pChannel), pFlashManager_(new caspar::FlashManager()) {
}

FlashCGManager::~FlashCGManager() {
	if(pFlashManager_ != 0) {
		delete pFlashManager_;
		pFlashManager_ = 0;
	}
}

void FlashCGManager::DisplayActive() {
/*	if(pChannel_->GetActiveProducer() != activeCGProducer_) {
		LOG << LogLevel::Debug << TEXT("Had to display active cg-producer");

		caspar::TransitionInfo transition;
		if(pChannel_->LoadBackground(activeCGProducer_, transition)){
			pChannel_->Play();
		}
		else {
			LOG << TEXT("Failed to display active cg-producer");
		}
	}*/
}
FlashProducerPtr FlashCGManager::CreateNewProducer()
{
#if TEMPLATEHOST_VERSION < 1700
	return std::tr1::dynamic_pointer_cast<FlashProducer, MediaProducer>(pFlashManager_->CreateProducer(GetApplication()->GetTemplateFolder()+TEXT("CG.fth")));
#else
	return std::tr1::dynamic_pointer_cast<FlashProducer, MediaProducer>(pFlashManager_->CreateProducer(GetApplication()->GetTemplateFolder()+TEXT("CG.fth.17")));
#endif
}

void FlashCGManager::Add(int layer, const tstring& templateName, unsigned int mixInDuration, bool playOnLoad, const tstring& startFromLabel, const tstring& data) {
	if(activeCGProducer_ == 0 || activeCGProducer_->IsEmpty()) {
		activeCGProducer_ = CreateNewProducer();
		
		LOG << TEXT("Created new flashproducer");
	}

	if(activeCGProducer_ != 0) {
		tstringstream flashParam;
		flashParam << TEXT("<invoke name=\"Add\" returntype=\"xml\"><arguments><number>") << layer << TEXT("</number><string>") << templateName << TEXT("</string><number>") << mixInDuration << TEXT("</number>") << (playOnLoad?TEXT("<true/>"):TEXT("<false/>")) << TEXT("<string>") << startFromLabel << TEXT("</string><string><![CDATA[ ") << data << TEXT(" ]]></string></arguments></invoke>");
		DisplayActive();
		activeCGProducer_->Param(flashParam.str());
		LOG << LogLevel::Debug << TEXT("Invoked add-command");
	}
}

void FlashCGManager::Remove(int layer) {
	if(activeCGProducer_ != 0) {
		tstringstream flashParam;
#if TEMPLATEHOST_VERSION < 1700
		flashParam << TEXT("<invoke name=\"Delete\" returntype=\"xml\"><arguments><number>") << layer << TEXT("</number></arguments></invoke>");
#else
		flashParam << TEXT("<invoke name=\"Delete\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>") << layer << TEXT("</number></property></array></arguments></invoke>");
#endif
		activeCGProducer_->Param(flashParam.str());
		DisplayActive();
	}
}

void FlashCGManager::Clear() {
	activeCGProducer_ = CreateNewProducer();
	DisplayActive();
}

void FlashCGManager::Play(int layer) {
	if(activeCGProducer_ != 0) {
		tstringstream flashParam;
#if TEMPLATEHOST_VERSION < 1700
		flashParam << TEXT("<invoke name=\"Play\" returntype=\"xml\"><arguments><number>") << layer << TEXT("</number></arguments></invoke>");
#else
		flashParam << TEXT("<invoke name=\"Play\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>") << layer << TEXT("</number></property></array></arguments></invoke>");
#endif

		activeCGProducer_->Param(flashParam.str());
		LOG << LogLevel::Debug << TEXT("Invoked play-command");
		DisplayActive();
	}
}

void FlashCGManager::Stop(int layer, unsigned int mixOutDuration) {
	if(activeCGProducer_ != 0) {
		tstringstream flashParam;
#if TEMPLATEHOST_VERSION < 1700
		flashParam << TEXT("<invoke name=\"Stop\" returntype=\"xml\"><arguments><number>") << layer << TEXT("</number><number>") << mixOutDuration << TEXT("</number></arguments></invoke>");
#else
		flashParam << TEXT("<invoke name=\"Stop\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>") << layer << TEXT("</number></property></array><number>") << mixOutDuration << TEXT("</number></arguments></invoke>");
#endif
		activeCGProducer_->Param(flashParam.str());
		LOG << LogLevel::Debug << TEXT("Invoked stop-command");
		DisplayActive();
	}
}

void FlashCGManager::Next(int layer) {
	if(activeCGProducer_ != 0) {
		tstringstream flashParam;
#if TEMPLATEHOST_VERSION < 1700
		flashParam << TEXT("<invoke name=\"Next\" returntype=\"xml\"><arguments><number>") << layer << TEXT("</number></arguments></invoke>");
#else
		flashParam << TEXT("<invoke name=\"Next\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>") << layer << TEXT("</number></property></array></arguments></invoke>");
#endif
		activeCGProducer_->Param(flashParam.str());
		DisplayActive();
	}
}

void FlashCGManager::Goto(int layer, const tstring& label) {
	if(activeCGProducer_ != 0) {
		tstringstream flashParam;
#if TEMPLATEHOST_VERSION < 1700
		flashParam << TEXT("<invoke name=\"GotoLabel\" returntype=\"xml\"><arguments><number>") << layer << TEXT("</number><string>") << label << TEXT("</string></arguments></invoke>");
#else
		flashParam << TEXT("<invoke name=\"GotoLabel\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>") << layer << TEXT("</number></property></array><string>") << label << TEXT("</string></arguments></invoke>");
#endif
		activeCGProducer_->Param(flashParam.str());
		DisplayActive();
	}
}

void FlashCGManager::Update(int layer, const tstring& data) {
	if(activeCGProducer_ != 0) {
		tstringstream flashParam;
#if TEMPLATEHOST_VERSION < 1700
		flashParam << TEXT("<invoke name=\"SetData\" returntype=\"xml\"><arguments><number>") << layer << TEXT("</number><string><![CDATA[ ") << data << TEXT(" ]]></string></arguments></invoke>");
#else
		flashParam << TEXT("<invoke name=\"SetData\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>") << layer << TEXT("</number></property></array><string><![CDATA[ ") << data << TEXT(" ]]></string></arguments></invoke>");
#endif
		activeCGProducer_->Param(flashParam.str());
		DisplayActive();
	}
}

void FlashCGManager::Invoke(int layer, const tstring& methodSpec) {
	if(activeCGProducer_ != 0) {
		tstringstream flashParam;
#if TEMPLATEHOST_VERSION < 1700
		flashParam << TEXT("<invoke name=\"ExecuteMethod\" returntype=\"xml\"><arguments><number>") << layer << TEXT("</number><string>") << methodSpec << TEXT("</string></arguments></invoke>");
#else
		flashParam << TEXT("<invoke name=\"ExecuteMethod\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>") << layer << TEXT("</number></property></array><string>") << methodSpec << TEXT("</string></arguments></invoke>");
#endif
		activeCGProducer_->Param(flashParam.str());
		DisplayActive();
	}
}

}	//namespace CG
}	//namespace caspar