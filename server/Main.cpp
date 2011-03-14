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
 
#include "StdAfx.h"
#include "Application.h"
#include "utils\FileOutputStream.h"
#include <tbb/task_scheduler_init.h>
#include <tbb/tbbmalloc_proxy.h>

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
	
	tbb::task_scheduler_init task_scheduler(max(2, tbb::task_scheduler_init::default_num_threads()));
	
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
