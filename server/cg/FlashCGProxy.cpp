#include "..\StdAfx.h"

#include "../producers/flash/flashproducer.h"
#include "../application.h"
#include "FlashCGProxy.h"

namespace caspar {
namespace CG { 

using namespace utils;

FlashCGProxy::FlashCGProxy()
{
#if TEMPLATEHOST_VERSION < 1700
	pFlashProducer_ = FlashProducer::Create(GetApplication()->GetTemplateFolder()+TEXT("CG.fth"));
#else
	pFlashProducer_ = FlashProducer::Create(GetApplication()->GetTemplateFolder()+TEXT("CG.fth.17"));
#endif
	if(!pFlashProducer_)
		throw std::exception("Failed to create flashproducer for templatehost");
}

FlashCGProxy::~FlashCGProxy()
{
}

FlashCGProxyPtr FlashCGProxy::Create()
{
	return FlashCGProxyPtr(new FlashCGProxy());
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

void FlashCGProxy::Clear() {
	pFlashProducer_->Stop();
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

}	//namespace CG
}	//namespace caspar