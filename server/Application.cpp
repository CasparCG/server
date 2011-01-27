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
 
#include "stdafx.h"

#ifdef _DEBUG
	#define _CRTDBG_MAP_ALLOC
	#include <stdlib.h>
	#include <crtdbg.h>
#endif

#include "Application.h"

#include <direct.h>
#include <io.h>
#include <time.h>
#include <stdio.h>
#include <string>

#include "Window.h"
#include "Channel.h"

#include "amcp\AMCPProtocolStrategy.h"
#include "cii\CIIProtocolStrategy.h"
#include "CLK\CLKProtocolStrategy.h"
#include "io\AsyncEventServer.h"
#include "io\SerialPort.h"

#include "cg\flashcgproxy.h"

#include "utils\FindWrapper.h"
#include "utils\FileExists.h"
#include "utils\FileOutputStream.h"
#include "utils\Thread.h"

#include "FileInfo.h"
#include "consumers\decklink\DecklinkVideoConsumer.h"
#include "consumers\OGL\OGLVideoConsumer.h"
#include "consumers\GDI\GDIVideoConsumer.h"
#include "consumers\audio\AudioConsumer.h"

#ifndef DISABLE_BLUEFISH
#include "consumers\bluefish\BlueFishVideoConsumer.h"
#endif

#include "audio\DirectSoundManager.h"

#include "producers\flash\FlashManager.h"
#include "producers\flash\CTManager.h"
#include "producers\flash\FlashAxContainer.h"
#include "producers\targa\TargaManager.h"
#include "producers\targascroll\TargaScrollManager.h"
#include "producers\ffmpeg\FFmpegManager.h"
#include "producers\color\ColorManager.h"

WTL::CAppModule _Module;

void WINAPI SvcMain(DWORD argsCount, LPTSTR* ppArgs) {
	//caspar::GetApplication()->ServiceMain();
}
void WINAPI SvcCtrlHandler(DWORD dwCtrl) {
	//caspar::GetApplication()->ServiceCtrlHandler(dwCtrl);
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) { 
	return caspar::GetApplication()->WndProc(hWnd, message, wParam, lParam);
}

namespace caspar {

using namespace utils;

enum ControllerTransports { TCP, Serial, TransportsCount };
enum ControllerProtocols { AMCP, CII, CLOCK, ProtocolsCount };

const TCHAR* Application::versionString_(TEXT("CG 1.8.1.7"));
const TCHAR* Application::serviceName_(TEXT("Caspar service"));

Application::Application(const tstring& cmdline, HINSTANCE hInstance) :	   hInstance_(hInstance), logLevel_(2), logDir_(TEXT("log")), 
																		   videoDir_(TEXT("media")),
																		   templateDir_(TEXT("templates")),
																		   dataDir_(TEXT("data")),
																		   colorManagerIndex_(-1),
																		   pAudioManager_(0),
																		   terminateEvent_(FALSE, FALSE),
																		   commandline_(cmdline)
																		   //svcStatusHandle_(0)
{
#ifdef _DEBUG
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF );
	_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG );
	_CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_DEBUG );
	_CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_DEBUG );

//	_CrtSetBreakAlloc(1393);
	Thread::EnableWin32ExceptionHandler(false);
#else
	Thread::EnableWin32ExceptionHandler(true);
#endif

	HRESULT hResult = ::OleInitialize(NULL);
	_Module.Init(NULL, hInstance, &LIBID_ATLLib);
}

Application::~Application()
{
	Destroy();

	_Module.Term();
	::OleUninitialize();
}

//////////////////////
// Run as service
void Application::RunAsService() {
	SERVICE_TABLE_ENTRY dispatchTable[] = 
	{ 
		{ TEXT(""), (LPSERVICE_MAIN_FUNCTION) SvcMain }, 
		{ NULL, NULL } 
	}; 

	// This call returns when the service has stopped. 
	// The process should simply terminate when the call returns.
//	if(!StartServiceCtrlDispatcher(dispatchTable)) { 
		//SvcReportEvent(TEXT("StartServiceCtrlDispatcher")); 
//	}
}

//void Application::ServiceMain() {
//	// Register the handler function for the service
//	svcStatusHandle_ = RegisterServiceCtrlHandler(serviceName_, SvcCtrlHandler);
//	if(!svcStatusHandle_) { 
//		//SvcReportEvent(TEXT("RegisterServiceCtrlHandler")); 
//		return; 
//	} 
//
//	//These SERVICE_STATUS members remain as set here
//	serviceStatus_.dwServiceType = SERVICE_WIN32_OWN_PROCESS; 
//	serviceStatus_.dwServiceSpecificExitCode = 0;    
//
//	// Report initial status to the SCM
//	ServiceReportStatus(SERVICE_START_PENDING, NO_ERROR, 3000);
//
//	if(Initialize()) {
//		ReportSvcStatus(SERVICE_RUNNING, NO_ERROR, 0);
//
//		MSG msg;
//		bool bQuit = false;
//		int errorCode = 0;
//
//		while(!bQuit) {
//			const HANDLE waitEvents[] =  {stopEvent_, terminateEvent_ };
//			HRESULT waitResult = MsgWaitForMultipleObjects(1, &waitEvents, FALSE, 2500, QS_ALLEVENTS);
//			switch(waitResult) 
//			{
//			case WAIT_OBJECT_0:
//				bQuit = true;
//				errorCode = NO_ERROR;
//				break;
//
//			case WAIT_OBJECT_0 + 1:
//				bQuit = true;
//				errorCode = 42;
//				break;
//
//			case WAIT_OBJECT_0+2:
//				if(GetMessage(&msg, NULL, 0, 0))
//				{
//					TranslateMessage(&msg);
//					DispatchMessage(&msg);
//				}
//				else {
//					bQuit = true;
//					errorCode = msg.wParam;
//				}
//				break;
//
//			case WAIT_TIMEOUT:
//				break;
//
//			case WAIT_FAILED:
//				LOG << LogLevel::Critical << "Wait failed in main thread. Exiting";
//			default:
//				bQuit = true;
//				errorCode = 42;
//				break;
//			}
//		}
//
//		Destroy();
//		ServiceReportStatus(SERVICE_STOPPED, errorCode, 0);
//	}
//	else {
//		ServiceReportStatus(SERVICE_STOPPED, NO_ERROR, 0);
//	}
//}
//
//void Application::ServiceCtrlHandler(DWORD dwCtrl) {
//	switch(dwCtrl) {
//		case SERVICE_CONTROL_STOP:
//			ServiceReportStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);
//			this->GetShutdownEvent().Set();
//			return;
//
//		case SERVICE_CONTROL_INTERROGATE:
//			break;
//		default:
//			break;
//	}
//	ServiceReportStatus(serviceStatus_.dwCurrentState, NO_ERROR, 0);
//}
//void Application::ServiceReportStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint) {
//    static DWORD dwCheckPoint = 1;
//
//    // Fill in the SERVICE_STATUS structure.
//    serviceStatus_.dwCurrentState = dwCurrentState;
//    serviceStatus_.dwWin32ExitCode = dwWin32ExitCode;
//    serviceStatus_.dwWaitHint = dwWaitHint;
//
//    if (dwCurrentState == SERVICE_START_PENDING)
//        serviceStatus_.dwControlsAccepted = 0;
//    else serviceStatus_.dwControlsAccepted = SERVICE_ACCEPT_STOP;
//
//    if ( (dwCurrentState == SERVICE_RUNNING) ||
//           (dwCurrentState == SERVICE_STOPPED) )
//        serviceStatus_.dwCheckPoint = 0;
//    else serviceStatus_.dwCheckPoint = dwCheckPoint++;
//
//    // Report the status of the service to the SCM.
//    SetServiceStatus(svcStatusHandle_, &serviceStatus_);
//}

///////////////////
// Run as window
int Application::RunAsWindow() {
	if(Initialize() && pWindow_ != 0) {
		MSG msg;

		while (GetMessage(&msg, NULL, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		return (int)msg.wParam;

	//	bool bQuit = false;

	//	while(!bQuit) {
	//		const HANDLE terminateEvent = terminateEvent_;
	//		HRESULT waitResult = MsgWaitForMultipleObjects(1, &terminateEvent, FALSE, 2500, QS_ALLEVENTS);
	//		switch(waitResult) 
	//		{
	//		case WAIT_OBJECT_0:
	//			return 42;

	//		case WAIT_OBJECT_0+1:
	//			if(GetMessage(&msg, NULL, 0, 0))
	//			{
	//				TranslateMessage(&msg);
	//				DispatchMessage(&msg);
	//			}
	//			else
	//				return msg.wParam;

	//		case WAIT_TIMEOUT:
	//			break;

	//		case WAIT_FAILED:
	//			LOG << LogLevel::Critical << "Wait failed in main thread. Exiting";
	//		default:
	//			return 99;
	//		}
	//	}
	//	return 0;
	}
	return -1;
}

LRESULT Application::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	TCHAR timeString[256];
	static int oldyday = 0;
	static UINT_PTR timerID = 0;
	
	switch (message) 
	{
		case WM_PAINT:
			BeginPaint(hWnd, &ps);
			EndPaint(hWnd, &ps);
			break;

		case WM_CREATE: 
			{
				timerID = SetTimer(hWnd,1,1000,NULL);
				InvalidateRect(hWnd,NULL,true);

				__time64_t timevalue = _time64(NULL);
				tm timeStruct;
				_localtime64_s(&timeStruct, &timevalue);

				oldyday = timeStruct.tm_yday;

				TCHAR logFile[512];
				_stprintf_s<>(logFile, TEXT("%slog %04d-%02d-%02d.log"), logDir_.c_str(), timeStruct.tm_year + 1900, timeStruct.tm_mon + 1, timeStruct.tm_mday);
				Logger::GetInstance().SetOutputStream(OutputStreamPtr(new FileOutputStream(logFile, true)));
			}
			break;

		case WM_TIMER:
			if(wParam == timerID)
			{
				__time64_t timevalue = _time64(NULL);
				tm timeStruct;
				_localtime64_s(&timeStruct, &timevalue);

				if(timeStruct.tm_yday != oldyday)
				{
					LOG << "new day";

					TCHAR logFile[512];

					oldyday = timeStruct.tm_yday;
					_stprintf_s<>(logFile, TEXT("%slog %04d-%02d-%02d.log"), logDir_.c_str(), timeStruct.tm_year + 1900, timeStruct.tm_mon + 1, timeStruct.tm_mday);
					Logger::GetInstance().SetOutputStream(OutputStreamPtr(new FileOutputStream(logFile, true)));
				}
				
#ifdef DISABLE_BLUEFISH
				const TCHAR* strCompability = TEXT(" (No bluefish)");
#else
				const TCHAR* strCompability = TEXT("");
#endif
				_stprintf_s<>(timeString, TEXT("Caspar %s%s - %02d:%02d:%02d"), GetVersionString(), strCompability, timeStruct.tm_hour, timeStruct.tm_min, timeStruct.tm_sec);
				SetWindowText(pWindow_->getHwnd(), timeString);
			}
			break;
			
		case WM_ENDSESSION:
			if(wParam == TRUE) {
				LOG << TEXT("APPLICATION: User shutdown or rebooting system.\r\n\r\n");
				if(timerID != 0)
					KillTimer(hWnd, timerID);

				Destroy();
			}
			else {
				LOG << TEXT("APPLICATION: Received ENDSESSION notification.");
			}
			break;

		case WM_SETFOCUS: 
			break;

		case WM_CHAR:
			break;

		case WM_SIZE: 
			break;

		case WM_DESTROY:
			if(timerID != 0)
				KillTimer(hWnd, timerID);

			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

/////////////////////////////
// Application::Initialize
// PARAMS: hinstance(processens instans. Forwardad från WinMain)
// RETURNS: true if every component was successfully initialized.
// COMMENT: Initializes all the components
/////////////////////////////
bool Application::Initialize()
{
#ifdef _DEBUG
	MessageBox(NULL, TEXT("Now is the time to connect for remote debugging..."), TEXT("Debug"), MB_OK | MB_TOPMOST);
#endif

	try
	{
		_configthreadlocale(_DISABLE_PER_THREAD_LOCALE);
		std::locale::global(std::locale(""));

		//Hämtar inställningar
		LoadSettings10();
		LoadSettings15();
		LoadSettings17();
		ParseCommandline();

		if(!CheckDirectories())
			return false;


		Logger::GetInstance().SetLevel(static_cast<LogLevel::LogLevelEnum>(logLevel_ + 1));

		pWindow_ = WindowPtr(new Window());
		pWindow_->Initialize(hInstance_, TEXT("Caspar"), TEXT("SVT_CASPAR"));

		if(caspar::flash::FlashAxContainer::CheckForFlashSupport())
			caspar::CG::FlashCGProxy::SetCGVersion();
		else {
			LOG << LogLevel::Critical << TEXT("No flashplayer activex-control installed. Flash support will be disabled");
		}

		caspar::directsound::DirectSoundManager::GetInstance()->Initialize(pWindow_->getHwnd(), 2, 48000, 16);
		pAudioManager_ = caspar::directsound::DirectSoundManager::GetInstance();

		sourceMediaManagers_.push_back(MediaManagerPtr(new FlashManager()));
		sourceMediaManagers_.push_back(MediaManagerPtr(new CTManager()));
		sourceMediaManagers_.push_back(MediaManagerPtr(new TargaManager()));
		sourceMediaManagers_.push_back(MediaManagerPtr(new TargaScrollMediaManager()));
		sourceMediaManagers_.push_back(MediaManagerPtr(new ffmpeg::FFMPEGManager()));
		sourceMediaManagers_.push_back(MediaManagerPtr(new ColorManager()));

		colorManagerIndex_ = static_cast<int>(sourceMediaManagers_.size()-1);

		////////////////////////////
		// SETUP VideoOut Channels
		int videoChannelIndex = 1;
#ifndef DISABLE_BLUEFISH
		int videoDeviceCount = caspar::bluefish::BlueFishVideoConsumer::EnumerateDevices();
		LOG << TEXT("BLUEFISH: Found ") << videoDeviceCount << TEXT(" video cards.");

		for(int bluefishIndex = 1; bluefishIndex<=videoDeviceCount; ++bluefishIndex, ++videoChannelIndex) {
			CreateVideoChannel(videoChannelIndex, caspar::bluefish::BlueFishVideoConsumer::Create(bluefishIndex));
		}
#endif

		//Decklink
		if(GetSetting(TEXT("nodecklink")) != TEXT("true")) {
			VideoConsumerPtr pDecklinkConsumer(caspar::decklink::DecklinkVideoConsumer::Create());
			if(pDecklinkConsumer)
				CreateVideoChannel(videoChannelIndex++, pDecklinkConsumer);
		}

		if(GetSetting(TEXT("gdichannel")) == TEXT("true") && pWindow_ != 0) {
			CreateVideoChannel(videoChannelIndex++, VideoConsumerPtr(new gdi::GDIVideoConsumer(pWindow_->getHwnd(), FrameFormatDescription::FormatDescriptions[FFormat576p2500])));
		}
		else if(GetSetting(TEXT("oglchannel")) == TEXT("true") && pWindow_ != 0)
		{
			ogl::Stretch stretch = ogl::Fill;
			tstring stretchStr = GetSetting(TEXT("stretch"));
			if(stretchStr == TEXT("none"))
				stretch = ogl::None;
			else if(stretchStr == TEXT("uniform"))
				stretch = ogl::Uniform;
			else if(stretchStr == TEXT("uniformtofill"))
				stretch = ogl::UniformToFill;

			tstring screenStr = GetSetting(TEXT("displaydevice")).c_str();
			int screen = 0;
			if(screenStr != TEXT(""))
				screen = _wtoi(screenStr.c_str());

			tstring strVideoMode = GetSetting(TEXT("oglvideomode"));
			if(strVideoMode == TEXT(""))
				strVideoMode = GetSetting(TEXT("videomode"));
			
			FrameFormat casparVideoFormat = FFormat576p2500;
			if(strVideoMode != TEXT(""))
				casparVideoFormat = caspar::GetVideoFormat(strVideoMode);

			CreateVideoChannel(videoChannelIndex++, VideoConsumerPtr(new ogl::OGLVideoConsumer(pWindow_->getHwnd(), FrameFormatDescription::FormatDescriptions[casparVideoFormat], screen, stretch)));
		}

		CreateVideoChannel(videoChannelIndex++, VideoConsumerPtr(new audio::AudioConsumer(FrameFormatDescription::FormatDescriptions[FFormat576p2500])));
		CreateVideoChannel(videoChannelIndex++, VideoConsumerPtr(new audio::AudioConsumer(FrameFormatDescription::FormatDescriptions[FFormat576p2500])));

		if(videoChannels_.size() < 1)
		{
			LOG << TEXT("No channels found, quitting");
			return false;
		}

		//////////////////////
		// Setup controllers
		SetupControllers();

		///////////////////////////
		// Initiate videochannels
		for(unsigned int i=0;i<videoChannels_.size();++i) {
			ChannelPtr pChannel = GetChannel(i);

			if(pChannel != 0) {
				pChannel->Clear();
			}
		}
	}
	catch(const std::exception& ex) {
		LOG << LogLevel::Critical << TEXT("Initialization exception: ") << ex.what();
		MessageBoxA(NULL, ex.what(), "Error", MB_OK | MB_TOPMOST);
		return false;
	}

	LOG << TEXT("Successful initialization.");
	return true;
}

/////////////////////////////////////////
// Application::Destroy
// COMMENT: shuts down all the components in the correct order
void Application::Destroy() {
	//First kill all possibilities for new commands to arrive...
	controllers_.clear();

	//...Then shutdown all channels, stopping playback etc...
	videoChannels_.clear();

	//...finally delete all mediamanagers
	sourceMediaManagers_.clear();

	LOG << TEXT("Shutdown complete.\r\n\r\n");
}

/////////////////////////////////////
// Application::CreateVideoChannel
// PARAMS: index(each channel has to have a unique index), videoConsumer(the consumer)
// COMMENT: Creates a channel and connects a consumer to it
void Application::CreateVideoChannel(int index, VideoConsumerPtr pVideoConsumer) {
	if(pVideoConsumer != 0) {
		ChannelPtr pChannel(new Channel(index, pVideoConsumer));
		if(pChannel != 0 && pChannel->Initialize()) {
			videoChannels_.push_back(pChannel);
		}
		else {
			LOG << TEXT("Failed to create channel ") <<  index;
		}
	}
	else {
		LOG << TEXT("Failed to create consumer for channel ") << index << TEXT(". Removing channel completely.");
	}
}

//////////////////////////////////
// Application::GetChannel
// PARAM: deviceIndex(the index of the requested channel)
// RETURNS: a (smart)pointer to the requested channel.
ChannelPtr Application::GetChannel(unsigned int deviceIndex) {
	ChannelPtr result;
	if(deviceIndex < videoChannels_.size())
		result = videoChannels_[deviceIndex];

	return result;
}

/////////////////////////////
// Application::FindFile
// PARAM: filename(fileame without the ending), pFileInfo(pointer to a structure that should be populated with info about the file)
// RETURNS: (smart)pointer to the MediaManager that handles the specified file
// COMMENT: Checks if the file exists and if it can be opened by any of the active mediamanagers.
MediaManagerPtr Application::FindMediaFile(const tstring& filename, FileInfo* pFileInfo)
{
	for(unsigned int index=0;index<sourceMediaManagers_.size();++index)
	{
		const std::vector<tstring>* pExtensions;
		int extensionCount = sourceMediaManagers_[index]->GetSupportedExtensions(pExtensions);
		for(int extensionIndex=0; extensionIndex<extensionCount; ++extensionIndex)
		{
			tstring fullFilename = filename;
			if((*pExtensions)[extensionIndex].length() > 0) {
				fullFilename += TEXT('.');
				fullFilename += (*pExtensions)[extensionIndex];
			}

			WIN32_FIND_DATA findInfo;
			FindWrapper findWrapper(fullFilename, &findInfo);
			if(findWrapper.Success()) {
				if(pFileInfo != 0) {
					pFileInfo->filename = filename;
					pFileInfo->filetype = (*pExtensions)[extensionIndex];

					unsigned __int64 fileSize = findInfo.nFileSizeHigh;
					fileSize *= 0x100000000;
					fileSize += findInfo.nFileSizeLow;

					pFileInfo->size = fileSize;
				}

				return sourceMediaManagers_[index];
			}
		}
	}

	return MediaManagerPtr();
}

bool Application::FindTemplate(const tstring& templateName, tstring* pExtension) {
	bool bResult = exists(templateName + TEXT(".ft"));
	if(bResult) {
		if(pExtension != NULL)
			*pExtension = TEXT(".ft");
	}
	else {
		bResult = exists(templateName + TEXT(".ct"));
		if(bResult && pExtension != NULL)
			*pExtension = TEXT(".ct");
	}
	return bResult;
}

void Application::SetupControllers()
{
	HKEY hKey = 0;
	HKEY hSubkey = 0;
	try {
		if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Sveriges Television AB\\Caspar CG\\Controllers"), 0,KEY_READ, &hKey) == ERROR_SUCCESS)
		{
			DWORD keyCount = 0;
			if(RegQueryInfoKey(hKey, NULL, NULL, NULL, &keyCount, NULL, NULL, NULL, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
				tstring subkeyName = TEXT("0");
				for(DWORD keyIndex = 0; keyIndex < min(keyCount, 10); ++keyIndex) {
					subkeyName[0] = TEXT('0')+static_cast<TCHAR>(keyIndex);

					if(RegOpenKeyEx(hKey, subkeyName.c_str(), 0, KEY_READ, &hSubkey) == ERROR_SUCCESS) {
						DWORD protocolIndex = 0, transportIndex = 0;
						DWORD size = 0;
						LONG returnValue = 0;

						size = sizeof(DWORD);
						returnValue = RegQueryValueEx(hSubkey, TEXT("transport"), 0, 0, (BYTE*)&transportIndex, &size);
						if(returnValue != ERROR_SUCCESS || transportIndex >= TransportsCount)
							continue;

						size = sizeof(DWORD);
						returnValue = RegQueryValueEx(hSubkey, TEXT("protocol"), 0, 0, (BYTE*)&protocolIndex, &size);
						if(returnValue != ERROR_SUCCESS || protocolIndex >= ProtocolsCount)
							continue;

						//Setup controller
						ControllerPtr pController;
						switch(transportIndex) {
							case TCP:
								pController = CreateTCPController(hSubkey);
								break;
							case Serial:
								pController = CreateSerialController(hSubkey);
								break;
						}

						if(pController != 0) {
							//there is a Controller, create ProtocolStrategy
							caspar::IO::ProtocolStrategyPtr pProtocol;
							switch(protocolIndex) {
								case AMCP:
									pProtocol = caspar::IO::ProtocolStrategyPtr(new caspar::amcp::AMCPProtocolStrategy());
									break;
								case CII:
									pProtocol = caspar::IO::ProtocolStrategyPtr(new caspar::cii::CIIProtocolStrategy());
									break;
								case CLOCK:
									pProtocol = caspar::IO::ProtocolStrategyPtr(new caspar::CLK::CLKProtocolStrategy());
									break;
							}

							if(pProtocol != 0) {
								//Both Controller and ProtocolStrategy created. Initialize!
								pController->SetProtocolStrategy(pProtocol);
								if(pController->Start())
									controllers_.push_back(pController);
								else {
									LOG << TEXT("Failed to start controller.");
								}
							}
							else {
								LOG << TEXT("failed to create protocol.");
							}
						}
						else {
							LOG << TEXT("Failed to create controller.");
						}
						
						RegCloseKey(hSubkey);
						hSubkey = 0;
					}
					else {
						LOG << TEXT("Failed to read controller-settings.");
					}
				}
			}
			RegCloseKey(hKey);
		}
		else {
			LOG << TEXT("Failed to read controller-settings.");
		}
	}
	catch(std::exception&) {
		if(hSubkey != 0)
			RegCloseKey(hSubkey);

		if(hKey != 0)
			RegCloseKey(hKey);
	}
}

ControllerPtr Application::CreateTCPController(HKEY hSubkey) {
	DWORD port = 5250;

	//read port setting
	DWORD size = sizeof(DWORD);
	RegQueryValueEx(hSubkey, TEXT("TCPPort"), 0, 0, (BYTE*)&port, &size);

	ControllerPtr pAsyncEventServer(new caspar::IO::AsyncEventServer(port));
	dynamic_cast<caspar::IO::AsyncEventServer*>(pAsyncEventServer.get())->SetClientDisconnectHandler(std::tr1::bind(&Application::OnClientDisconnected, this, std::tr1::placeholders::_1));

	return pAsyncEventServer;
}

ControllerPtr Application::CreateSerialController(HKEY hSubkey)
{
	DWORD baudRate = 19200;
	DWORD dataBits = 8;
	DWORD parity = NOPARITY;
	DWORD stopBits = ONESTOPBIT;
	tstring portName;
	TCHAR pPortname[256];
	ZeroMemory(pPortname, sizeof(pPortname));

	//read baudrate setting
	DWORD size = sizeof(DWORD);
	RegQueryValueEx(hSubkey, TEXT("SerialBaudrate"), 0, 0, (BYTE*)&baudRate, &size);

	//read databits setting
	size = sizeof(DWORD);
	RegQueryValueEx(hSubkey, TEXT("SerialDatabits"), 0, 0, (BYTE*)&dataBits, &size);

	//read parity setting
	size = sizeof(DWORD);
	RegQueryValueEx(hSubkey, TEXT("SerialParity"), 0, 0, (BYTE*)&parity, &size);

	//read stopbits setting
	size = sizeof(DWORD);
	RegQueryValueEx(hSubkey, TEXT("SerialStopbits"), 0, 0, (BYTE*)&stopBits, &size);

	//read portname setting
	size = sizeof(pPortname) - sizeof(TCHAR);
	LONG returnValue = RegQueryValueEx(hSubkey, TEXT("SerialPortname"), 0, 0, (BYTE*)pPortname, &size);
	if(returnValue == ERROR_SUCCESS && size > 0)
		portName = pPortname;

	return ControllerPtr(new caspar::IO::SerialPort(baudRate, static_cast<BYTE>(parity), static_cast<BYTE>(dataBits), static_cast<BYTE>(stopBits), portName));
}

void Application::OnClientDisconnected(caspar::IO::SocketInfoPtr pClient) 
{
	Monitor::ClearListener(pClient);
}

//////////////////////////////////
// Application::LoadSetting10
bool Application::LoadSettings10()
{
	HKEY hKey;
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Sveriges Television AB\\Caspar CG\\1.0"), 0,KEY_READ, &hKey) == ERROR_SUCCESS)
	{
		TCHAR pString[512];

		//read media-folder setting
		DWORD size = sizeof(pString) - sizeof(TCHAR);
		LONG returnValue = RegQueryValueEx(hKey, TEXT("MediaFolder"), 0, 0, (BYTE*)pString, &size);
		if(returnValue == ERROR_SUCCESS && size > 0)
			videoDir_ = pString;

		//read log-folder setting
		size = sizeof(pString) - sizeof(TCHAR);
		returnValue = RegQueryValueEx(hKey, TEXT("LogFolder"), 0, 0, (BYTE*)pString, &size);
		if(returnValue == ERROR_SUCCESS && size > 0)
			logDir_ = pString;

		//read loglevel
		size = sizeof(int);
		returnValue = RegQueryValueEx(hKey, TEXT("LogLevel"), 0, 0, (BYTE*)&logLevel_, &size);

		//close handle after all settings have been saved
		RegCloseKey(hKey);
	}
	else {
		LOG << TEXT("No 1.0 settings defined. Using defaults.");
	}

	return true;
}

//////////////////////////////////
// Application::LoadSetting15
bool Application::LoadSettings15()
{
	HKEY hKey;
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Sveriges Television AB\\Caspar CG\\1.5"), 0,KEY_READ, &hKey) == ERROR_SUCCESS)
	{
		TCHAR pString[512];

		//read template-folder setting
		DWORD size = sizeof(pString) - sizeof(TCHAR);
		LONG returnValue = RegQueryValueEx(hKey, TEXT("TemplateFolder"), 0, 0, (BYTE*)pString, &size);
		if(returnValue == ERROR_SUCCESS && size > 0)
			templateDir_ = pString;

		//close handle after all settings have been saved
		RegCloseKey(hKey);
	}
	else {
		LOG << TEXT("No 1.5 settings defined. Using defaults.");
	}

	return true;
}

//////////////////////////////////
// Application::LoadSetting17
bool Application::LoadSettings17()
{
	HKEY hKey;
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Sveriges Television AB\\Caspar CG\\1.7"), 0,KEY_READ, &hKey) == ERROR_SUCCESS)
	{
		TCHAR pString[512];

		//read data-folder setting
		DWORD size = sizeof(pString) - sizeof(TCHAR);
		LONG returnValue = RegQueryValueEx(hKey, TEXT("DataFolder"), 0, 0, (BYTE*)pString, &size);
		if(returnValue == ERROR_SUCCESS && size > 0)
			dataDir_ = pString;

		//close handle after all settings have been saved
		RegCloseKey(hKey);
	}
	else {
		LOG << TEXT("No 1.7 settings defined. Using defaults.");
	}

	return true;
}

bool Application::CheckDirectories()
{
	bool returnValue = true;

	if(logDir_.size() == 0) {
		TCHAR pString[512];
		if(_tgetcwd(pString, _MAX_PATH+1)) {
			logDir_ = pString;
			logDir_ += TEXT("\\log");
			_tmkdir(logDir_.c_str());
		}
	}

	if(!CheckDirectory(logDir_)) {
		LOG << TEXT("Failed to aquire log-folder, logging to c:\\");
		logDir_ = TEXT("c:\\");
	}

	if(!CheckDirectory(videoDir_)) {
		LOG << TEXT("Media-folder does not exists");
		tstring error = TEXT("Kunde inte hitta mediafolder. ");
		error += videoDir_;
		MessageBox(NULL, error.c_str(), TEXT("Kritiskt fel"), MB_OK);
		returnValue = false;
	}

	if(!CheckDirectory(templateDir_)) {
		LOG << TEXT("Template-folder does not exists");
		tstring error = TEXT("Kunde inte hitta templatefolder. ");
		error += templateDir_;
		MessageBox(NULL, error.c_str(), TEXT("Kritiskt fel"), MB_OK);
		returnValue = false;
	}

	if(returnValue) {
		if(!CheckDirectory(dataDir_)) {
			LOG << TEXT("Data-folder does not exists, default to same as media-folder");
			dataDir_ = videoDir_;
		}
	}

	LOG << TEXT("Media-folder: ") << videoDir_.c_str();
	LOG << TEXT("Template-folder: ") << templateDir_.c_str();
	LOG << TEXT("Log-folder: ") << logDir_.c_str();
	LOG << TEXT("Data-folder: ") << dataDir_.c_str();

	return returnValue;
}

bool Application::CheckDirectory(tstring& directory) {
	if(directory.size() == 0 || !exists(directory, true))
		return false;

	bool bIsRelative = false;
	if(directory.size() >= 2) {
		if(directory[1] != TEXT(':')) {
			bIsRelative = true;
		}
	}
	else 
		bIsRelative = true;

	if(bIsRelative) {
		TCHAR pCurrentWorkingDirectory[_MAX_PATH+1];
		if(_tgetcwd(pCurrentWorkingDirectory, _MAX_PATH)) {
			tstring fullPath = pCurrentWorkingDirectory;
			fullPath.push_back(TEXT('\\'));
			fullPath.append(directory);
			directory = fullPath;
		}
	}

	if(directory[directory.size()-1] != TEXT('\\'))
		directory += '\\';

	return true;
}

///////////////////////////
//
// Commandline parameters
//
// hd=[720p50, 1080i50] chooses hd-mode for videochannels if availible
// spy{=true} instructs the serialport to NOT SEND ANYTHING
// internalkey(=true) instructs the videochannels to perform internal keying if availible
void Application::ParseCommandline() {
	tstring currentToken = TEXT("");
	tstring lastKey = TEXT("");

	bool inValue=false, inQuote=false;

	unsigned int charIndex = 0;
	for(charIndex = 0; charIndex < commandline_.size(); ++charIndex) {
		TCHAR ch = commandline_[charIndex];
		switch(ch) {
			case TEXT('\"'):
				inQuote = !inQuote;
				if(currentToken.size() > 0) {
					if(inValue) {
						settings_.insert(SettingsMap::value_type(lastKey, currentToken));
						lastKey.clear();
						inValue = false;
					}
					else {
						settings_.insert(SettingsMap::value_type(currentToken, TEXT("true")));
					}

					currentToken.clear();
				}
				break;

			case TEXT('='):
				if(currentToken.size() > 0) {
					lastKey = currentToken;
					currentToken.clear();
					inValue = true;
				}
				break;

			case TEXT(' '):
				if(inQuote)
					currentToken += ch;
				else {
					if(currentToken.size() > 0) {
						if(inValue) {
							settings_.insert(SettingsMap::value_type(lastKey, currentToken));
							lastKey.clear();
							inValue = false;
						}
						else {
							settings_.insert(SettingsMap::value_type(currentToken, TEXT("true")));
						}

						currentToken.clear();
					}
				}
				break;

			default:
				currentToken += ch;
				break;
		}
	}

	if(currentToken.size() > 0) {
		if(inValue) {
			settings_.insert(SettingsMap::value_type(lastKey, currentToken));
		}
		else {
			settings_.insert(SettingsMap::value_type(currentToken, TEXT("true")));
		}
	}
}

tstring Application::GetSetting(const tstring& key) {
	SettingsMap::iterator it = settings_.find(key);
	if(it != settings_.end())
		return (*it).second;
	else
		return TEXT("");
}

//////////////////////////////
// ServiceHelpers

void Application::InstallService() 
{
	tstringstream message;
	SC_HANDLE schSCManager;
	SC_HANDLE schService;
	TCHAR szPath[MAX_PATH];

	if(!GetModuleFileName(NULL, szPath, MAX_PATH)) {
		message << TEXT("Error: ") << GetLastError();
		MessageBox(NULL, message.str().c_str(), TEXT("Cannot install service"), MB_OK);
		return;
	}

	// Get a handle to the SCM database. 
	schSCManager = OpenSCManager( 
		NULL,                    // local computer
		NULL,                    // ServicesActive database 
		SC_MANAGER_ALL_ACCESS);  // full access rights 

	if(schSCManager == NULL) {
		message << TEXT("OpenSCManager failed. Error: ") << GetLastError();
		MessageBox(NULL, message.str().c_str(), TEXT("Cannot install service"), MB_OK);
		return;
	}

	// Create the service
	tstring imagePath(szPath);
	imagePath.append(TEXT(" service"));

	schService = CreateService( 
		schSCManager,              // SCM database 
		serviceName_,              // name of service 
		serviceName_,              // service name to display 
		SERVICE_ALL_ACCESS,        // desired access 
		SERVICE_WIN32_OWN_PROCESS, // service type 
		SERVICE_DEMAND_START,      // start type 
		SERVICE_ERROR_NORMAL,      // error control type 
		imagePath.c_str(),         // path to service's binary 
		NULL,                      // no load ordering group 
		NULL,                      // no tag identifier 
		NULL,                      // no dependencies 
		NULL,                      // LocalSystem account 
		NULL);                     // no password 

	if(schService == NULL) {
		message << TEXT("CreateService failed. Error: ") << GetLastError(); 
		CloseServiceHandle(schSCManager);
		MessageBox(NULL, message.str().c_str(), TEXT("Cannot install service"), MB_OK);
		return;
	}
	else
		MessageBox(NULL, TEXT("Service installed successfully"), TEXT("Done!"), MB_OK);

	CloseServiceHandle(schService); 
	CloseServiceHandle(schSCManager);
}

void Application::UninstallService()
{
	tstringstream message;
	SC_HANDLE schSCManager;
	SC_HANDLE schService;

	// Get a handle to the SCM database. 
	schSCManager = OpenSCManager( 
		NULL,                    // local computer
		NULL,                    // ServicesActive database 
		SC_MANAGER_ALL_ACCESS);  // full access rights 
 
	if(schSCManager == NULL){
		message << TEXT("OpenSCManager failed. Error: ") << GetLastError();
		MessageBox(NULL, message.str().c_str(), TEXT("Cannot uninstall service"), MB_OK);
		return;
	}

	// Get a handle to the service.
	schService = OpenService( 
		schSCManager,         // SCM database 
		serviceName_,  // name of service 
		DELETE);              // need delete access 
 
	if (schService == NULL)
	{ 
		message << TEXT("OpenService failed. Error: ") << GetLastError();
		MessageBox(NULL, message.str().c_str(), TEXT("Cannot uninstall service"), MB_OK);
		CloseServiceHandle(schSCManager);
		return;
	}

	// Delete the service.
	if (!DeleteService(schService)) 
	{
		message << TEXT("DeleteService failed. Error: ") << GetLastError();
		MessageBox(NULL, message.str().c_str(), TEXT("Cannot uninstall service"), MB_OK);
	}
	else
		MessageBox(NULL, TEXT("Service uninstalled successfully"), TEXT("Done!"), MB_OK);

	CloseServiceHandle(schService); 
	CloseServiceHandle(schSCManager);
}

}