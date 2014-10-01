#include "..\stdafx.h"
#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
//extern CComModule _Module;
#include <atlcom.h>
#include <string>
#include <comutil.h>

#include "WASP_Memory.h"
//#include "../../video_format.h"
#include "QueryPerformanceCounter.h"

#define PIPE_WASPCG  L"\\\\.\\pipe\\listener"
#define PIPE_TIMEOUT 5000

#define HDRLEN 10
#define RESPONSELEN 58
#define HDR "WASP"

enum COMMAND
{
	LOAD    = 1,
	PLAY    = 2,	
	UPDATE  = 3,
	STOP    = 4
};

struct COMMANDINFO
{	
	COMMAND  eCommandType;
	CComBSTR bstrCommand;
	
	COMMANDINFO()
	{
		bstrCommand = L"";
	}
};


BOOL CWASP_Memory::GetSharedMemoryHandles()
{
	//ERR_LOG_START()

	/*if(g_stGlobalObjects.m_hMappedFile)
		return true;*/

	m_hReadThread = NULL;

	
	// Create Handle To memory mapped file
	m_hMappedFile = CreateFileMapping(INVALID_HANDLE_VALUE,				// use paging file
														NULL,								// default security 
														PAGE_READWRITE,						// read/write access
														0,									// max. object size 
														sizeof(OUTPUTINFO) + MAX_VIDEO_SIZE,// buffer size  
														WASPOUTPUTFILE);					// name of mapping object


	if (m_hMappedFile == NULL || m_hMappedFile == INVALID_HANDLE_VALUE) 
	{
		/*g_stGlobalObjects.m_hMappedFile = NULL;
		TCHAR szTestHrNo[MAX_PATH];
		_stprintf(szTestHrNo,_T(" Line [%d] in fn: %s()  CreateFileMapping failed") ,_T(__FILE__) , dwErrorCntr); */
		//LOG_ENTRY_ERROR(E_FAIL,szTestHrNo)
		OutputDebugString(L"CreateFileMapping failed");

		return false;
	}
	

	m_pFileBuff = (BYTE*) MapViewOfFile(m_hMappedFile, FILE_MAP_ALL_ACCESS, 0, 0, 0);


	/*if (g_stGlobalObjects.m_pFileBuff == NULL) 
	{
		CloseHandle(g_stGlobalObjects.m_hMappedFile);
		g_stGlobalObjects.m_hMappedFile = NULL;
		TCHAR szTestHrNo[MAX_PATH];
		_stprintf(szTestHrNo,_T(" Line [%d] in fn: %s()  MapViewOfFile failed ") ,_T(__FILE__) , dwErrorCntr); 
		LOG_ENTRY_ERROR(E_FAIL,szTestHrNo)
		return false;
	}*/

	// Read header from mapped file
	m_pOutPut = (OUTPUTINFO*)m_pFileBuff;
    InitializeCriticalSection(&m_csfreeBuffers);
		


	CreateBufferPool();
	m_bRunning = true;
	m_hReadThread = CreateThread(NULL, 0, ReadThread, this, 0, 0);
	

	// Create Events to Read and Write data to Shared memory 
	m_hReadEvt  = CreateEvent(NULL,TRUE,FALSE,WASPREADEVENT);	
	m_hWriteEvt = CreateEvent(NULL,TRUE,FALSE,WASPWRITEEVENT);


	//Connect ro Pipe
	m_hPipe = CreateFile( 
			 PIPE_WASPCG,   // pipe name 
			 GENERIC_READ |  // read and write access 
			 GENERIC_WRITE, 
			 0,              // no sharing 
			 NULL,           // default security attributes
			 OPEN_EXISTING,  // opens existing pipe 
			 0,              // default attributes 
			 NULL);          // no template file 

	if(m_hPipe == INVALID_HANDLE_VALUE )
	{
		TCHAR szTestHrNo[MAX_PATH];
		_stprintf(szTestHrNo,_T(" No Pipe available for connection %d "), GetLastError());
		OutputDebugString(szTestHrNo);

	}



	////create a new namepipe
	//m_hPipe = CreateNamedPipeW( 
	//				 PIPE_WASPCG,     // pipe name 
	//				 PIPE_ACCESS_DUPLEX ,     // read/write access 
	//				 PIPE_TYPE_MESSAGE |      // message-type pipe 
	//				 PIPE_READMODE_MESSAGE |  // message-read mode 
	//				 PIPE_WAIT,               // blocking mode 
	//				 PIPE_UNLIMITED_INSTANCES,// number of instances 
	//				 sizeof(COMMANDINFO),   // output buffer size 
	//				 sizeof(COMMANDINFO) ,   // input buffer size 
	//				 PIPE_TIMEOUT,            // client time-out 
	//				 NULL);                   // default security attributes 


	////connect to pipe..
	//if( !ConnectNamedPipe(m_hPipe, NULL) )
	//{
	//	return false;
	//	DWORD dwERRor = GetLastError();
	//}
		
	return true;

	//ERR_LOG_END()
}

BOOL g_bInit = TRUE;

CStopwatch g_objWatch;

safe_ptr<core::basic_frame> CWASP_Memory::receive(const safe_ptr<core::frame_factory> frame_factory_)
{
	g_objWatch.Start();
	try
	{
		BYTE *pBuff  = NULL;
		while(TryEnterCriticalSection(&m_csfreeBuffers)==0)
				Sleep(1);
		try
		{
			if(m_lockedBuffers.size() > 0)
			{
				pBuff = m_lockedBuffers.front();
				m_lockedBuffers.pop();
			}
		}
		catch(...)
		{
		}
		LeaveCriticalSection(&m_csfreeBuffers);
		if(!pBuff)
		{
			OutputDebugString(L"@@@ No locked buffers ");
			return frame_;
		}


		auto format_desc = frame_factory_->get_video_format_desc();
		
		core::pixel_format_desc desc;
		desc.pix_fmt = core::pixel_format::bgra;

		DWORD dwSize   = m_dwSize;//m_pOutPut->dwWidth * m_pOutPut->dwHeight * m_pOutPut->dwBitCount/8;
		DWORD dwOffset = 0;

		if(m_pOutPut->bInterlaced)
		{
			format_desc.height = format_desc.height/2 ;
			dwSize = dwSize/2;
		}
		desc.planes.push_back(core::pixel_format_desc::plane(format_desc.width, format_desc.height,  4));


		auto frame = frame_factory_->create_frame(this, desc);	
		
		//copy field 0 to  field 0 offset in frame
		if(frame->image_data().begin() != NULL && pBuff != NULL)
		{
			//std::copy_n(pBuff, dwSize, frame->image_data().begin());	
			memcpy(frame->image_data().begin(), pBuff, dwSize);
		}

		frame->commit();		


		if(m_pOutPut->bInterlaced )
		{
			auto frame1 = frame_factory_->create_frame(this, desc);

			dwOffset += dwSize;

			//copy field 1 to  field 1 offset in frame
			if(frame1->image_data().begin() != NULL && pBuff != NULL)
			{
				//std::copy_n(pBuff+dwOffset, dwSize, frame1->image_data().begin());
				memcpy(frame1->image_data().begin(), pBuff+dwOffset, dwSize);
			}

			frame1->commit();			

			//push back to locked q
			//memset(pBuff, 0, m_dwSize);
			while(TryEnterCriticalSection(&m_csfreeBuffers)==0)
				Sleep(1);
			try
			{
				m_freeBuffers.push(pBuff);
			}
			catch(...)
			{
			}
			LeaveCriticalSection(&m_csfreeBuffers);


			format_desc.field_mode = caspar::core::field_mode::upper;
			frame_ = core::basic_frame::interlace(frame, frame1, format_desc.field_mode);


			float fTime = g_objWatch.Now();
			if(fTime > 5)
			{
				TCHAR sz[256];
				_stprintf( sz , L"@@@ receive time %f" , fTime);
					OutputDebugString( sz );
			}
			
			return  frame_;
		}

		//push back to locked q
		while(TryEnterCriticalSection(&m_csfreeBuffers)==0)
			Sleep(1);
		try
		{
			m_freeBuffers.push(pBuff);
		}
		catch(...)
		{
		}
		LeaveCriticalSection(&m_csfreeBuffers);

		frame_ = std::move(frame);
	}
	catch(...)
	{
		CASPAR_LOG(info) <<"*** CWASP_Memory::receive ***";
	}	
	return frame_;

}

BOOL CWASP_Memory::SendCommandToPipe( std::wstring strCommand)
{
	try
	{
		 OutputDebugString(L"@@@ SendCommandToPipe");
		 CComBSTR bstr = strCommand.c_str();
		 CHAR *pDataCmd = _com_util::ConvertBSTRToString(bstr);
		 short iLen =  strlen(pDataCmd);

		 DWORD dwBytesSent =0; 
		 bool bSuccess = WriteFile( 
							m_hPipe,               // pipe handle 
							pDataCmd,             // message  //pCmdBuff
							iLen,     // message length 
							&dwBytesSent,             // bytes written 
							NULL);                  // not overlapped 

		 if( !bSuccess )
		 {
			 TCHAR sz[256];
			 _stprintf( sz , L"WriteFile Error : %d" , GetLastError() );
			 OutputDebugString( sz );

			// hr = GetLastError();
		 }

		  TCHAR sz[1000];
		 _stprintf( sz , L"@@@ BYTES sent over pipe  %d " , dwBytesSent);
		 OutputDebugString( sz );

		 if(pDataCmd)
			 delete pDataCmd;
	}
	catch(...)
	{
		CASPAR_LOG(info) <<"*** Exception SendCommandToPipe ***";
	}

	 return true;
}


BOOL CWASP_Memory::ReadCommandFromPipe( )
{
	 CHAR* pCmdBuff = new CHAR[RESPONSELEN];

	 DWORD cbWritten=0; 
	 bool bSuccess = ReadFile( 
					m_hPipe,               // pipe handle 
					pCmdBuff,             // message 
					RESPONSELEN,     // message length 
					&cbWritten,             // bytes written 
					NULL);                  // not overlapped 

	 if( !bSuccess /*|| (stricmp(pCmdBuff, "RESPONSE") != 0)*/)
	 {
		 TCHAR sz[256];
		 _stprintf( sz , L"ReadFile Error : %d" , GetLastError() );
		 OutputDebugString( sz );
		// hr = GetLastError();
	 }

	 return bSuccess;
}

short CWASP_Memory::GetCheckSum(CHAR *pCmdBuff)
{
	short iChkSum = 0;
	for(int i = 0; pCmdBuff[i] != '\0'; i++)
	{
		iChkSum += pCmdBuff[i];
	}
	return iChkSum;
}

BOOL  CWASP_Memory::CreateBufferPool()
{
	try
	{
		m_dwSize = m_pOutPut->dwWidth * m_pOutPut->dwHeight * m_pOutPut->dwBitCount/8;
		/*if(m_pOutPut->bInterlaced)
			m_dwSize = m_dwSize/2;*/

		for(int iCnt = 0; iCnt < 5; iCnt++)
		{
			BYTE* pBuff = new BYTE[m_dwSize];
			m_freeBuffers.push(pBuff);
		}
	}
	catch(...)
	{

	}

	return true;
}

DWORD WINAPI CWASP_Memory::ReadThread(LPVOID lpParameter)
{
	CWASP_Memory *pThis = (CWASP_Memory *)lpParameter;

	pThis->Readproc();

	return 0;	
}

CStopwatch g_objThrdWatch;
void CWASP_Memory::Readproc()
{
	while(m_bRunning)
	{
		try
		{
			//Wait for the read event from memory out	
			CStopwatch objwatch;
			if(WAIT_TIMEOUT == WaitForSingleObject (m_hReadEvt, 1000))
			{
				CASPAR_LOG(info) <<"*** Waiting MemoryOutPut Data Event TimeOVER ***";
				OutputDebugString(L"@@@ Waiting MemoryOutPut Data Event TimeOVER");
			}

			float fTime = g_objThrdWatch.Now();
			//if(fTime > 5)
			{
				TCHAR sz[256];
				_stprintf( sz , L"@@@ Event signalled in  %f wait time %f" , fTime, objwatch.Now());
				 OutputDebugString( sz );
			}
			g_objThrdWatch.Start();
			

			ResetEvent(m_hReadEvt);
			DWORD dwOffset = sizeof(OUTPUTINFO);

			//lock.........................................
			while(TryEnterCriticalSection(&m_csfreeBuffers)==0)
					Sleep(1);

			try
			{
				if(m_freeBuffers.size()>0)
				{				
					//copy the frame received from memout to bufferpool			
					BYTE *pBuff = m_freeBuffers.front();
					m_freeBuffers.pop();
					if(pBuff)
					{
						memcpy(pBuff, m_pFileBuff+dwOffset, m_dwSize);
						m_lockedBuffers.push(pBuff);
					}
								
				}
				else
					OutputDebugString(L"@@@ No free buffers");
			}
			catch(...)
			{

			}
  			LeaveCriticalSection(&m_csfreeBuffers);
		}
		catch(...)
		{
			CASPAR_LOG(info)<<(L"@@@ Exception in read proc");
		
		}
	}
}

