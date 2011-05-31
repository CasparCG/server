#include "StdAfx.h"
#include "Application.h"
#include "utils\FileOutputStream.h"

//the easy way to make it possible to forward WndProc messages into the application-object
caspar::Application* pGlobal_Application = 0;

namespace caspar {
	Application* GetApplication()
	{
		return pGlobal_Application;
	}
}

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int)
{
	int returnValue = 0;
	tstring commandline(lpCmdLine);
	
	//if(commandline == TEXT("install"))
	//{
	//	caspar::Application::InstallService();
	//}
	//else if(commandline == TEXT("uninstall"))
	//{
	//	caspar::Application::UninstallService();
	//}
	//else {
		try 
		{
			caspar::utils::OutputStreamPtr pOutputStream(new caspar::utils::FileOutputStream(TEXT("startup.log")));
			caspar::utils::Logger::GetInstance().SetOutputStream(pOutputStream);
			caspar::utils::Logger::GetInstance().SetTimestamp(true);

			caspar::Application app(lpCmdLine, hInstance);
			pGlobal_Application = &app;

			//if(commandline.find(TEXT("service")) != tstring::npos)
			//	app.RunAsService();
			//else
				returnValue = app.RunAsWindow();
		}
		catch(const std::exception& ex)
		{
			LOG << caspar::utils::LogLevel::Critical << TEXT("UNHANDLED EXCEPTION in main thread. Message: ") << ex.what();
		}
//	}

	return returnValue;
}
