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

#include "../producers/flash/flashproducer.h"
#include "../application.h"
#include "../utils/fileexists.h"
#include "FlashCGProxy.h"

namespace caspar {
namespace CG { 

using namespace utils;

int FlashCGProxy::cgVersion_ = 0;

class FlashCGProxy16 : public FlashCGProxy
{
public:
	FlashCGProxy16::FlashCGProxy16()
	{
		pFlashProducer_ = FlashProducer::Create(GetApplication()->GetTemplateFolder()+TEXT("CG.fth"));
		if(!pFlashProducer_)
			throw std::exception("Failed to create flashproducer for templatehost");
	}

	virtual void Add(int layer, const tstring& templateName, bool playOnLoad, const tstring& label, const tstring& data) {
		tstringstream flashParam;

		tstring::size_type pos = templateName.find('.');
		tstring filename = (pos != tstring::npos) ? templateName.substr(0, pos) : templateName;
		
		flashParam << TEXT("<invoke name=\"Add\" returntype=\"xml\"><arguments><number>") << layer << TEXT("</number><string>") << filename << TEXT("</string><number>0</number>") << (playOnLoad?TEXT("<true/>"):TEXT("<false/>")) << TEXT("<string>") << label << TEXT("</string><string><![CDATA[ ") << data << TEXT(" ]]></string></arguments></invoke>");
		LOG << LogLevel::Debug << TEXT("Invoking add-command");
		pFlashProducer_->Param(flashParam.str());
	}
	virtual void Remove(int layer) {
		tstringstream flashParam;
		flashParam << TEXT("<invoke name=\"Delete\" returntype=\"xml\"><arguments><number>") << layer << TEXT("</number></arguments></invoke>");
		LOG << LogLevel::Debug << TEXT("Invoking remove-command");
		pFlashProducer_->Param(flashParam.str());
	}
	virtual void Play(int layer) {
		tstringstream flashParam;
		flashParam << TEXT("<invoke name=\"Play\" returntype=\"xml\"><arguments><number>") << layer << TEXT("</number></arguments></invoke>");
		LOG << LogLevel::Debug << TEXT("Invoking play-command");
		pFlashProducer_->Param(flashParam.str());
	}
	virtual void Stop(int layer, unsigned int mixOutDuration) {
		tstringstream flashParam;
		flashParam << TEXT("<invoke name=\"Stop\" returntype=\"xml\"><arguments><number>") << layer << TEXT("</number><number>") << mixOutDuration << TEXT("</number></arguments></invoke>");
		LOG << LogLevel::Debug << TEXT("Invoking stop-command");
		pFlashProducer_->Param(flashParam.str());
	}
	virtual void Next(int layer) {
		tstringstream flashParam;
		flashParam << TEXT("<invoke name=\"Next\" returntype=\"xml\"><arguments><number>") << layer << TEXT("</number></arguments></invoke>");
		LOG << LogLevel::Debug << TEXT("Invoking next-command");
		pFlashProducer_->Param(flashParam.str());
	}
	virtual void Update(int layer, const tstring& data) {
		tstringstream flashParam;
		flashParam << TEXT("<invoke name=\"SetData\" returntype=\"xml\"><arguments><number>") << layer << TEXT("</number><string><![CDATA[ ") << data << TEXT(" ]]></string></arguments></invoke>");
		LOG << LogLevel::Debug << TEXT("Invoking update-command");
		pFlashProducer_->Param(flashParam.str());
	}
	virtual void Invoke(int layer, const tstring& label) {
		tstringstream flashParam;
		flashParam << TEXT("<invoke name=\"ExecuteMethod\" returntype=\"xml\"><arguments><number>") << layer << TEXT("</number><string>") << label << TEXT("</string></arguments></invoke>");
		LOG << LogLevel::Debug << TEXT("Invoking invoke-command");
		pFlashProducer_->Param(flashParam.str());
	}
};

class FlashCGProxy17 : public FlashCGProxy
{
public:
	FlashCGProxy17::FlashCGProxy17()
	{
		pFlashProducer_ = FlashProducer::Create(GetApplication()->GetTemplateFolder()+TEXT("CG.fth.17"));
		if(!pFlashProducer_)
			throw std::exception("Failed to create flashproducer for templatehost");
	}

	virtual void Add(int layer, const tstring& templateName, bool playOnLoad, const tstring& label, const tstring& data) {
		tstringstream flashParam;

		tstring::size_type pos = templateName.find('.');
		tstring filename = (pos != tstring::npos) ? templateName.substr(0, pos) : templateName;
		
		flashParam << TEXT("<invoke name=\"Add\" returntype=\"xml\"><arguments><number>") << layer << TEXT("</number><string>") << filename << TEXT("</string>") << (playOnLoad?TEXT("<true/>"):TEXT("<false/>")) << TEXT("<string>") << label << TEXT("</string><string><![CDATA[ ") << data << TEXT(" ]]></string></arguments></invoke>");
		LOG << LogLevel::Debug << TEXT("Invoking add-command");
		pFlashProducer_->Param(flashParam.str());
	}
	virtual void Remove(int layer) {
		tstringstream flashParam;
		flashParam << TEXT("<invoke name=\"Delete\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>") << layer << TEXT("</number></property></array></arguments></invoke>");
		LOG << LogLevel::Debug << TEXT("Invoking remove-command");
		pFlashProducer_->Param(flashParam.str());
	}
	virtual void Play(int layer) {
		tstringstream flashParam;
		flashParam << TEXT("<invoke name=\"Play\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>") << layer << TEXT("</number></property></array></arguments></invoke>");
		LOG << LogLevel::Debug << TEXT("Invoking play-command");
		pFlashProducer_->Param(flashParam.str());
	}
	virtual void Stop(int layer, unsigned int mixOutDuration) {
		tstringstream flashParam;
		flashParam << TEXT("<invoke name=\"Stop\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>") << layer << TEXT("</number></property></array><number>") << mixOutDuration << TEXT("</number></arguments></invoke>");
		LOG << LogLevel::Debug << TEXT("Invoking stop-command");
		pFlashProducer_->Param(flashParam.str());
	}
	virtual void Next(int layer) {
		tstringstream flashParam;
		flashParam << TEXT("<invoke name=\"Next\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>") << layer << TEXT("</number></property></array></arguments></invoke>");
		LOG << LogLevel::Debug << TEXT("Invoking next-command");
		pFlashProducer_->Param(flashParam.str());
	}
	virtual void Update(int layer, const tstring& data) {
		tstringstream flashParam;
		flashParam << TEXT("<invoke name=\"SetData\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>") << layer << TEXT("</number></property></array><string><![CDATA[ ") << data << TEXT(" ]]></string></arguments></invoke>");
		LOG << LogLevel::Debug << TEXT("Invoking update-command");
		pFlashProducer_->Param(flashParam.str());
	}
	virtual void Invoke(int layer, const tstring& label) {
		tstringstream flashParam;
		flashParam << TEXT("<invoke name=\"Invoke\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>") << layer << TEXT("</number></property></array><string>") << label << TEXT("</string></arguments></invoke>");
		LOG << LogLevel::Debug << TEXT("Invoking invoke-command");
		pFlashProducer_->Param(flashParam.str());
	}
};

class FlashCGProxy18 : public FlashCGProxy17
{
public:
	FlashCGProxy18::FlashCGProxy18(Monitor* pMonitor)
	{
		pFlashProducer_ = FlashProducer::Create(GetApplication()->GetTemplateFolder()+TEXT("CG.fth.18"), pMonitor);
		if(!pFlashProducer_)
			throw std::exception("Failed to create flashproducer for templatehost");
	}

	virtual void Add(int layer, const tstring& templateName, bool playOnLoad, const tstring& label, const tstring& data) {
		tstringstream flashParam;
		flashParam << TEXT("<invoke name=\"Add\" returntype=\"xml\"><arguments><number>") << layer << TEXT("</number><string>") << templateName << TEXT("</string>") << (playOnLoad?TEXT("<true/>"):TEXT("<false/>")) << TEXT("<string>") << label << TEXT("</string><string><![CDATA[ ") << data << TEXT(" ]]></string></arguments></invoke>");
		LOG << LogLevel::Debug << TEXT("Invoking add-command");
		pFlashProducer_->Param(flashParam.str());
	}
};

FlashCGProxy::FlashCGProxy()
{}

FlashCGProxy::~FlashCGProxy()
{}

FlashCGProxyPtr FlashCGProxy::Create(Monitor* pMonitor)
{
	FlashCGProxyPtr result;
	switch(cgVersion_) {
		case 18:
			result.reset(new FlashCGProxy18(pMonitor));
			break;
		case 17:
			result.reset(new FlashCGProxy17());
			break;
		case 16:
			result.reset(new FlashCGProxy16());
			break;

		default:
			break;
	}

	return result;
}

void FlashCGProxy::SetCGVersion() {
	if(exists(GetApplication()->GetTemplateFolder()+TEXT("cg.fth.18"))) {
		LOG << TEXT("Running version 1.8 template graphics.");
		cgVersion_ = 18;
	}
	else if(exists(GetApplication()->GetTemplateFolder()+TEXT("cg.fth.17"))) {
		LOG << TEXT("Running version 1.7 template graphics.");
		cgVersion_ = 17;
	}
	else if(exists(GetApplication()->GetTemplateFolder()+TEXT("cg.fth"))) {
		LOG << TEXT("Running version 1.6 template graphics.");
		cgVersion_ = 16;
	}
	else {
		LOG << TEXT("No templatehost found. Template graphics will be disabled");
		cgVersion_ = 0;
	}
}

bool FlashCGProxy::Initialize(FrameManagerPtr pFrameManager) {
	return pFlashProducer_->Initialize(pFrameManager);
}

FrameBuffer& FlashCGProxy::GetFrameBuffer() {
	return pFlashProducer_->GetFrameBuffer();
}

bool FlashCGProxy::IsEmpty() const {
	return pFlashProducer_->IsEmpty();
}
void FlashCGProxy::SetEmptyAlert(EmptyCallback callback) {
	pFlashProducer_->SetEmptyAlert(callback);
}
void FlashCGProxy::Stop() {
	pFlashProducer_->Stop();
}

void FlashCGProxy::Clear() {
	pFlashProducer_->Stop();
}

/*
void FlashCGProxy::Add(int layer, const tstring& templateName, bool playOnLoad, const tstring& label, const tstring& data) {
	tstringstream flashParam;

#if TEMPLATEHOST_VERSION < 1700
	flashParam << TEXT("<invoke name=\"Add\" returntype=\"xml\"><arguments><number>") << layer << TEXT("</number><string>") << templateName << TEXT("</string><number>0</number>") << (playOnLoad?TEXT("<true/>"):TEXT("<false/>")) << TEXT("<string>") << label << TEXT("</string><string><![CDATA[ ") << data << TEXT(" ]]></string></arguments></invoke>");
#else
	flashParam << TEXT("<invoke name=\"Add\" returntype=\"xml\"><arguments><number>") << layer << TEXT("</number><string>") << templateName << TEXT("</string>") << (playOnLoad?TEXT("<true/>"):TEXT("<false/>")) << TEXT("<string>") << label << TEXT("</string><string><![CDATA[ ") << data << TEXT(" ]]></string></arguments></invoke>");
#endif

	LOG << LogLevel::Debug << TEXT("Invoking add-command");
	pFlashProducer_->Param(flashParam.str());
}

void FlashCGProxy::Remove(int layer) {
	tstringstream flashParam;
#if TEMPLATEHOST_VERSION < 1700
	flashParam << TEXT("<invoke name=\"Delete\" returntype=\"xml\"><arguments><number>") << layer << TEXT("</number></arguments></invoke>");
#else
	flashParam << TEXT("<invoke name=\"Delete\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>") << layer << TEXT("</number></property></array></arguments></invoke>");
#endif
	LOG << LogLevel::Debug << TEXT("Invoking remove-command");
	pFlashProducer_->Param(flashParam.str());
}


void FlashCGProxy::Play(int layer) {
	tstringstream flashParam;
#if TEMPLATEHOST_VERSION < 1700
	flashParam << TEXT("<invoke name=\"Play\" returntype=\"xml\"><arguments><number>") << layer << TEXT("</number></arguments></invoke>");
#else
	flashParam << TEXT("<invoke name=\"Play\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>") << layer << TEXT("</number></property></array></arguments></invoke>");
#endif

	LOG << LogLevel::Debug << TEXT("Invoking play-command");
	pFlashProducer_->Param(flashParam.str());
}

void FlashCGProxy::Stop(int layer, unsigned int mixOutDuration) {
	tstringstream flashParam;
#if TEMPLATEHOST_VERSION < 1700
	flashParam << TEXT("<invoke name=\"Stop\" returntype=\"xml\"><arguments><number>") << layer << TEXT("</number><number>") << mixOutDuration << TEXT("</number></arguments></invoke>");
#else
	flashParam << TEXT("<invoke name=\"Stop\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>") << layer << TEXT("</number></property></array><number>") << mixOutDuration << TEXT("</number></arguments></invoke>");
#endif
	LOG << LogLevel::Debug << TEXT("Invoking stop-command");
	pFlashProducer_->Param(flashParam.str());
}

void FlashCGProxy::Next(int layer) {
	tstringstream flashParam;
#if TEMPLATEHOST_VERSION < 1700
	flashParam << TEXT("<invoke name=\"Next\" returntype=\"xml\"><arguments><number>") << layer << TEXT("</number></arguments></invoke>");
#else
	flashParam << TEXT("<invoke name=\"Next\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>") << layer << TEXT("</number></property></array></arguments></invoke>");
#endif
	LOG << LogLevel::Debug << TEXT("Invoking next-command");
	pFlashProducer_->Param(flashParam.str());
}

void FlashCGProxy::Update(int layer, const tstring& data) {
	tstringstream flashParam;
#if TEMPLATEHOST_VERSION < 1700
	flashParam << TEXT("<invoke name=\"SetData\" returntype=\"xml\"><arguments><number>") << layer << TEXT("</number><string><![CDATA[ ") << data << TEXT(" ]]></string></arguments></invoke>");
#else
	flashParam << TEXT("<invoke name=\"SetData\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>") << layer << TEXT("</number></property></array><string><![CDATA[ ") << data << TEXT(" ]]></string></arguments></invoke>");
#endif
	LOG << LogLevel::Debug << TEXT("Invoking update-command");
	pFlashProducer_->Param(flashParam.str());
}

void FlashCGProxy::Invoke(int layer, const tstring& label) {
	tstringstream flashParam;
#if TEMPLATEHOST_VERSION < 1700
	flashParam << TEXT("<invoke name=\"ExecuteMethod\" returntype=\"xml\"><arguments><number>") << layer << TEXT("</number><string>") << label << TEXT("</string></arguments></invoke>");
#else
	flashParam << TEXT("<invoke name=\"Invoke\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>") << layer << TEXT("</number></property></array><string>") << label << TEXT("</string></arguments></invoke>");
#endif
	LOG << LogLevel::Debug << TEXT("Invoking invoke-command");
	pFlashProducer_->Param(flashParam.str());
}
*/
}	//namespace CG
}	//namespace caspar