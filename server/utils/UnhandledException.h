#ifndef _UNHANDLED_EXCEPTION_H_
#define _UNHANDLED_EXCEPTION_H_

#include <exception>
#include <system_error>
#include <string>

#include "Logger.h"
#include "LogLevel.h"
#include "Win32Exception.h"

#define CASPAR_TRY try
#define CASPAR_CATCH_UNHANDLED(desc) \
		catch(std::system_error& er)	\
		{	\
			try 	\
			{	\
				LOG << caspar::utils::LogLevel::Critical << desc << TEXT(" UNHANDLED EXCEPTION Message:")  << er.code().message().c_str();	\
			}	\
			catch(...){}	\
		}	\
		catch(std::exception& ex)	\
		{	\
			try 	\
			{	\
				LOG << caspar::utils::LogLevel::Critical << desc << TEXT(" UNHANDLED EXCEPTION Message:")  << ex.what();	\
			}	\
			catch(...){}	\
		}	\
		catch(...)	\
		{	\
			try 	\
			{	\
				LOG << caspar::utils::LogLevel::Critical << desc << TEXT(" UNHANDLED EXCEPTION");	\
			}	\
			catch(...){}	\
		}

#define CASPAR_RETHROW_AND_LOG(desc) \
		catch(std::system_error& er)	\
		{	\
			try 	\
			{	\
				LOG << caspar::utils::LogLevel::Critical << desc << TEXT(" Message:") << er.code().message().c_str();	\
			}	\
			catch(...){}	\
			throw;\
		}	\
		catch(std::exception& ex)	\
		{	\
			try 	\
			{	\
				LOG << caspar::utils::LogLevel::Critical << desc << TEXT(" Message:") << ex.what();	\
			}	\
			catch(...){}	\
			throw;\
		}	\
		catch(...)	\
		{	\
			try 	\
			{	\
				LOG << caspar::utils::LogLevel::Critical << desc;	\
			}	\
			catch(...){}	\
			throw;\
		}

#define CASPAR_CATCH_AND_LOG(desc) \
		catch(std::system_error& er)	\
		{	\
			try 	\
			{	\
				LOG << caspar::utils::LogLevel::Critical << desc << TEXT(" Message:") << er.code().message().c_str();	\
			}	\
			catch(...){}	\
		}	\
		catch(std::exception& ex)	\
		{	\
			try 	\
			{	\
				LOG << caspar::utils::LogLevel::Critical << desc << TEXT(" Message:") << ex.what();	\
			}	\
			catch(...){}	\
		}	\
		catch(...)	\
		{	\
			try 	\
			{	\
				LOG << caspar::utils::LogLevel::Critical << desc;	\
			}	\
			catch(...){}	\
		}

template<typename F>
void CASPAR_THREAD_GUARD(const wchar_t* desc, const F& func)	
{
	CASPAR_THREAD_GUARD(0, desc, func);
}

template<typename F>
void CASPAR_THREAD_GUARD(int restart_count, const std::wstring& desc, const F& func)	
{
		LOG << caspar::utils::LogLevel::Verbose << desc << TEXT(" Thread Started");

#ifdef WIN32
		Win32Exception::InstallHandler();
		_configthreadlocale(_DISABLE_PER_THREAD_LOCALE);
#endif
		for(int n = -1; n < restart_count; ++n)
		{
			try
			{
				if(n >= 0)
					LOG << caspar::utils::LogLevel::Critical << desc << TEXT(" Restarting Count:")  << n+1 << "/" << restart_count;
				func();
				n = restart_count;
			}
			catch(std::system_error& er)	
			{	
				try 	
				{	
					LOG << caspar::utils::LogLevel::Critical << desc << TEXT(" UNHANDLED EXCEPTION Message:")  << er.code().message().c_str();	
				}	
				catch(...){}	
			}	
			catch(std::exception& ex)	
			{	
				try 	
				{	
					LOG << caspar::utils::LogLevel::Critical << desc << TEXT(" UNHANDLED EXCEPTION Message:")  << ex.what();	
				}	
				catch(...){}	
			}	
			catch(...)	
			{	
				try 	
				{	
					LOG << caspar::utils::LogLevel::Critical << desc << TEXT(" UNHANDLED EXCEPTION");	
				}	
				catch(...){}	
			}
		}
		LOG << caspar::utils::LogLevel::Verbose << desc << TEXT(" Thread Ended");
}

#endif