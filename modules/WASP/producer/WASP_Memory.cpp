#include "..\stdafx.h"
#include "winbase.h"
#include <atlbase.h>
#include <atlcom.h>
#include <string>
#include <comutil.h>

#include "WASP_Memory.h"
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
	try
	{

		// Opens the "WaspMemoryOutputFile" File Mapping Object.
		// If it is not present and a new one is created instead, then do not proceeed further
		m_hMappedFile = CreateFileMapping(INVALID_HANDLE_VALUE,				// use paging file
															NULL,								// default security 
															PAGE_READWRITE,						// read/write access
															0,									// max. object size 
															sizeof(OUTPUTINFO) + MAX_VIDEO_SIZE,// buffer size  
															WASPOUTPUTFILE);					// name of mapping object


		//return if a new object is created
		if(!(GetLastError() == ERROR_ALREADY_EXISTS))
			return false;

		if (m_hMappedFile == NULL || m_hMappedFile == INVALID_HANDLE_VALUE) 
		{
			OutputDebugString(L"CreateFileMapping failed");
			return false;
		}

		m_pFileBuff = (BYTE*) MapViewOfFile(m_hMappedFile, FILE_MAP_ALL_ACCESS, 0, 0, 0);

		// Read header from mapped file
		m_pOutPut = (OUTPUTINFO*)m_pFileBuff;
		
		CreateBufferPool();
		
		//Create Read Thread
		m_hReadThread = CreateThread(NULL, 0, ReadThread, this, 0, 0);
		m_bRunning = true;

		// Create Events to Read and Write data to Shared memory 
		m_hReadEvt  = CreateEvent(NULL,TRUE,FALSE,WASPREADEVENT);	
		m_hWriteEvt = CreateEvent(NULL,TRUE,FALSE,WASPWRITEEVENT);

		//Connect to Pipe
		m_hPipe = CreateFile( 
				 PIPE_WASPCG,	 // pipe name 
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
			_stprintf(szTestHrNo,_T("@@@ No Pipe available for connection %d "), GetLastError());
			OutputDebugString(szTestHrNo);

		}
	}
	catch(...)
	{
		OutputDebugString(L"@@@ Exception in CWASP_Memory::GetSharedMemoryHandles");
	}
		
	return true;
}



CStopwatch g_objWatch;

safe_ptr<core::basic_frame> CWASP_Memory::receive(const safe_ptr<core::frame_factory> frame_factory_)
{

	float fTime = g_objWatch.Now();
	g_objWatch.Start();
	if(fTime < 38 || (fTime>42))
	{
		TCHAR sz[256];
		_stprintf( sz , L"@@@ receive time %f" , fTime);
			OutputDebugString( sz );
	}


	/*if(m_bInit && m_lockedBuffers.size() >=1)
	{
		m_bInit = FALSE;

		TCHAR sz[256];
		_stprintf( sz , L"@@@ Init %d" , m_lockedBuffers.size());
		OutputDebugString( sz );
	}
	
	if(m_bInit)
		return frame_;
*/
	
	try
	{
		BYTE *pBuff  = NULL;
		while(TryEnterCriticalSection(m_csfreeBuffers) ==0)
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
		LeaveCriticalSection(m_csfreeBuffers);
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
			/*TCHAR sz[256];
			_stprintf( sz , L"@@@ copying field 0 %d" , dwSize);
			OutputDebugString( sz );*/
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
				/*TCHAR sz[256];
				_stprintf( sz , L"@@@ copying field 1 %d" , dwSize);
				OutputDebugString( sz );*/

				//std::copy_n(pBuff+dwOffset, dwSize, frame1->image_data().begin());
				memcpy(frame1->image_data().begin(), pBuff+dwOffset, dwSize);
			}

			frame1->commit();			

			//push back to locked q
			//memset(pBuff, 0, m_dwSize);
			while(TryEnterCriticalSection(m_csfreeBuffers) == 0)
				Sleep(1);
			try
			{
				m_freeBuffers.push(pBuff);
			}
			catch(...)
			{
			}
			LeaveCriticalSection(m_csfreeBuffers);


			format_desc.field_mode = caspar::core::field_mode::upper;
			frame_ = core::basic_frame::interlace(frame, frame1, format_desc.field_mode);


			
			
			return  frame_;
		}

		//push back to locked q
		while(TryEnterCriticalSection(m_csfreeBuffers) ==0)
			Sleep(1);
		try
		{
			m_freeBuffers.push(pBuff);
		}
		catch(...)
		{
		}
		LeaveCriticalSection(m_csfreeBuffers);

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
							m_hPipe,				// pipe handle 
							pDataCmd,				// message  
							iLen,					// message length 
							&dwBytesSent,           // bytes written 
							NULL);                  // not overlapped 

		 if( !bSuccess )
		 {
			 TCHAR sz[256];
			 _stprintf( sz , L"WriteFile Error : %d" , GetLastError() );
			 OutputDebugString( sz );
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
					m_hPipe,				// pipe handle 
					pCmdBuff,				// message 
					RESPONSELEN,			// message length 
					&cbWritten,				// bytes written 
					NULL);                  // not overlapped 

	 if( !bSuccess /*|| (stricmp(pCmdBuff, "RESPONSE") != 0)*/)
	 {
		 TCHAR sz[256];
		 _stprintf( sz , L"ReadFile Error : %d" , GetLastError() );
		 OutputDebugString( sz );
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

		for(int iCnt = 0; iCnt < 3; iCnt++)
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
			if(m_freeBuffers.size() <= 0)
			{
				/*TCHAR sz[256];
				_stprintf( sz , L"@@@ no free buffers  locked cnt %d" , m_lockedBuffers.size());
				 OutputDebugString( sz );*/
				Sleep(1);
				continue;
			}
			else
			{
				//signal memory output to write the frame
				InterlockedExchange(&(m_pOutPut->iVal), 1);				
			}


			//Wait for the read event from memory out	
			CStopwatch objwatch;
			if(WAIT_TIMEOUT == WaitForSingleObject (m_hReadEvt, 1000))
			{
				CASPAR_LOG(info) <<"*** Waiting MemoryOutPut Data Event TimeOVER ***";
				OutputDebugString(L"@@@ Waiting MemoryOutPut Data Event TimeOVER");
			}
			InterlockedExchange(&(m_pOutPut->iVal), 0);	

			float fTime = g_objThrdWatch.Now();
			if(fTime > 41 || fTime < 38)
			{
				TCHAR sz[256];
				_stprintf( sz , L"@@@ Event signalled in  %f %d" , fTime,  m_lockedBuffers.size());
				 OutputDebugString( sz );
			}
			g_objThrdWatch.Start();
			

			ResetEvent(m_hReadEvt);
			DWORD dwOffset = sizeof(OUTPUTINFO);

			//lock CricSec
			while(TryEnterCriticalSection(m_csfreeBuffers)==0)
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
			}
			catch(...)
			{

			}
  			LeaveCriticalSection(m_csfreeBuffers);

			
		}
		catch(...)
		{
			OutputDebugString(L"@@@ Exception in CWASP_Memory::Readproc");
			CASPAR_LOG(info)<<(L"Exception in CWASP_Memory::Readproc");
		}
	}
}

void CWASP_Memory::FreeResources()
{
	try
	{
		//Stop the Read Thread
		m_bRunning = false;

		//Wait for the Read Thread handle to be signalled
		if(WaitForSingleObject(m_hReadThread, INFINITE) == WAIT_OBJECT_0)
		{
			CloseHandle(m_hReadThread);
			m_hReadThread = NULL;
		}

		//After closing the thread handles, close all other handles
		//Close the Pipe Handle
		if (m_hPipe && (m_hPipe != INVALID_HANDLE_VALUE))
		{
			CloseHandle(m_hPipe);
			m_hPipe = NULL;
		}

		//Unmapvieoffile
		if(m_pFileBuff)
			UnmapViewOfFile(m_pFileBuff);

		//Close the shared memory Handle
		if (m_hMappedFile && (m_hMappedFile != INVALID_HANDLE_VALUE))
		{
			CloseHandle(m_hMappedFile);
			m_hMappedFile = NULL;
		}

		//Close the Read Event handle
		if (m_hReadEvt && (m_hReadEvt != INVALID_HANDLE_VALUE))
		{
			CloseHandle(m_hReadEvt);
			m_hReadEvt = NULL;
		}

		//Close the Write Event handle
		if (m_hWriteEvt && (m_hWriteEvt != INVALID_HANDLE_VALUE))
		{
			CloseHandle(m_hReadThread);
			m_hWriteEvt = NULL;
		}

		//clearing the buffers
		for(int i = 0;i < m_lockedBuffers.size(); i++)
		{
			BYTE* pByte = m_lockedBuffers.front();
			
			if(pByte)
				delete pByte;
			
			pByte = NULL;

			m_lockedBuffers.pop();
		}

		for(int i = 0;i < m_freeBuffers.size(); i++)
		{
			BYTE* pByte = m_freeBuffers.front();
			
			if(pByte)
				delete pByte;
			
			pByte = NULL;

			m_freeBuffers.pop();
		}

		//delete Critical Section
		if(m_csfreeBuffers)
			DeleteCriticalSection(m_csfreeBuffers);

		m_csfreeBuffers = NULL;
	}

	catch(...)
	{
		CASPAR_LOG(info) <<L"Exception in CWASP_Memory::FreeResources";
	}
}