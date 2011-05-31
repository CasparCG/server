#ifndef _CASPAR_APPLICATION_H__
#define _CASPAR_APPLICATION_H__

#pragma once

#include "Window.h"
#include "Controller.h"
#include "MediaManager.h"
#include "audio\audiomanager.h"
#include "utils\event.h"

#include <string>
#include <vector>
#include <list>
#include <map>

namespace caspar {

	class Channel;
	typedef std::tr1::shared_ptr<Channel> ChannelPtr;

	class IVideoConsumer;
	typedef std::tr1::shared_ptr<IVideoConsumer> VideoConsumerPtr;

	class FileInfo;

	class Application
	{
	public:
		explicit Application(const tstring& cmdline, HINSTANCE);
		~Application();

		static void InstallService();
		static void UninstallService();
		
		void RunAsService();
		int RunAsWindow();

		//void ServiceMain();
		//void ServiceCtrlHandler(DWORD dwCtrl);

		LRESULT WndProc(HWND, UINT, WPARAM, LPARAM);

		const TCHAR* GetVersionString() const {
			return versionString_;
		}
		WindowPtr& GetMainWindow() {
			return pWindow_;
		}
		caspar::audio::IAudioManager* GetAudioManager() {
			return pAudioManager_;
		}
		const tstring& GetMediaFolder() {
			return videoDir_;
		}
		const tstring& GetTemplateFolder() {
			return templateDir_;
		}
		const tstring& GetDataFolder() {
			return dataDir_;
		}

		ChannelPtr GetChannel(unsigned int deviceIndex);

		bool FindTemplate(const tstring& templateName, tstring* pExtension = NULL);
		MediaManagerPtr FindMediaFile(const tstring& filename, FileInfo* pFileInfo=0);
		MediaManagerPtr GetColorMediaManager() {
			if(colorManagerIndex_ != -1)
				return sourceMediaManagers_[colorManagerIndex_];
			else 
				return MediaManagerPtr();
		}

		tstring GetSetting(const tstring& key);
		utils::Event& GetTerminateEvent() { return terminateEvent_; }
		//utils::Event& GetStopEvent() { return stopEvent_; }

	private:
		bool Initialize();
		void Destroy();

		typedef std::map<tstring, tstring> SettingsMap;

		void CreateVideoChannel(int index, VideoConsumerPtr videoConsumer);
		ControllerPtr CreateTCPController(HKEY);
		ControllerPtr CreateSerialController(HKEY);

		bool LoadSettings10();
		bool LoadSettings15();
		bool LoadSettings17();
		void ParseCommandline();

		bool CheckDirectories();
		bool CheckDirectory(tstring& directory);

		void SetupControllers();

		WindowPtr							pWindow_;

		std::vector<ChannelPtr>		videoChannels_;
		std::vector<MediaManagerPtr>	sourceMediaManagers_;
		std::vector<ControllerPtr>			controllers_;

		caspar::audio::IAudioManager*		pAudioManager_;

		//Once the program is initiated, these directories are guaranteed to have a trailing '\'
		tstring		videoDir_;
		tstring		templateDir_;
		tstring		logDir_;
		tstring		dataDir_;

		int				logLevel_;
		static const TCHAR*	versionString_;
		
		static const TCHAR*	serviceName_;
		//SERVICE_STATUS_HANDLE   svcStatusHandle_;
		//SERVICE_STATUS			serviceStatus_;
		//void ServiceReportStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint);

		int				colorManagerIndex_;
		utils::Event	terminateEvent_;
		//utils::Event	stopEvent_;

		HINSTANCE hInstance_;
		tstring commandline_;
		SettingsMap settings_;
	};

}	//namespace caspar

#endif	//_CASPAR_APPLICATION_H__