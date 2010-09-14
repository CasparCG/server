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
 
#include "../StdAfx.h"

#if defined(_MSC_VER)
#pragma warning (push, 1) // TODO: Legacy code, just disable warnings, will replace with boost::asio in future
#endif

#include "SerialPort.h"

#include "../log/log.h"

#include <tchar.h>

namespace caspar { namespace IO {

using namespace common;

void LogCommError(const std::wstring& msg, int errorCode);


/* From winbase.h 
#define NOPARITY            0
#define ODDPARITY           1
#define EVENPARITY          2
#define MARKPARITY          3
#define SPACEPARITY         4

#define ONESTOPBIT          0
#define ONE5STOPBITS        1
#define TWOSTOPBITS         2
*/

SerialPort::SerialPort(const ProtocolStrategyPtr& pProtocol, DWORD baudRate, BYTE parity, BYTE dataBits, BYTE stopBits, const std::wstring& portName, bool spy)
	: hPort_(0), portName_(portName), spy_(spy), pProtocolStrategy_(pProtocol)
{
	pClientInfo_ = SerialPortClientInfoPtr(new SerialPortClientInfo(this));
	ZeroMemory(&commSettings_, sizeof(DCB));
	commSettings_.DCBlength = sizeof(commSettings_);
	commSettings_.BaudRate = baudRate;
	commSettings_.fBinary = TRUE;
	commSettings_.Parity = parity;
	if(parity != 0)
		commSettings_.fParity = TRUE;

	commSettings_.ByteSize = dataBits;
	commSettings_.StopBits = stopBits;
	commSettings_.fNull = TRUE;
}

SerialPort::~SerialPort() {
	Stop();
}


bool SerialPort::Start() {
	if(pProtocolStrategy_ == 0)
		return false;

	pListener_ = ListenerPtr(new Listener(this, pProtocolStrategy_));

	if(hPort_ == 0) {

		//Open the port
		HANDLE port = CreateFile(portName_.c_str(), GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
		if(port == INVALID_HANDLE_VALUE) {
			//TODO: Throw Win32ErrorException
			LogCommError(TEXT("Failed to open port. Error: "), GetLastError());

			return false;
		}

		//Setup port
		if(!SetCommState(port, &commSettings_)) {
			//TODO: Throw Win32ErrorException
			LogCommError(TEXT("Failed to set com-settings. Error: "), GetLastError());

			CloseHandle(port);
			return false;
		}

		COMMTIMEOUTS timeouts;
		ZeroMemory(&timeouts, sizeof(timeouts));
		timeouts.ReadIntervalTimeout = MAXDWORD;
		SetCommTimeouts(port, &timeouts);

		hPort_ = port;

		if(listenerThread_.Start(pListener_.get())) {
			if(spy_) {
				//don't send anything if we're spying, it would most likely trash the incomming data
				return true;
			}
			else {
				pWriter_ = WriterPtr(new Writer(hPort_));
				return writerThread_.Start(pWriter_.get());
			}
		}
	}

	return false;
}

void SerialPort::Stop() {
	if(hPort_ != 0) {
		if(listenerThread_.IsRunning())
			listenerThread_.Stop();

		if(writerThread_.IsRunning())
			writerThread_.Stop();

		CloseHandle(hPort_);
		hPort_ = 0;
	}
}


/// Listener
//////////////
const int SerialPort::Listener::WaitTimeout = 1000;
const int SerialPort::Listener::InputBufferSize = 256;

SerialPort::Listener::Listener(SerialPort* port, ProtocolStrategyPtr pPS) : pPort_(port), pProtocolStrategy_(pPS), overlappedReadEvent_(TRUE, FALSE), bWaitingOnRead_(false), pInputBuffer_(new char[InputBufferSize]), queuedChars_(0) {
	ZeroMemory(&overlappedRead_, sizeof(OVERLAPPED));
	overlappedRead_.hEvent = overlappedReadEvent_;

	ZeroMemory(pInputBuffer_, InputBufferSize);
}

SerialPort::Listener::~Listener() {
	if(pInputBuffer_ != 0) {
		delete[] pInputBuffer_;
		pInputBuffer_ = 0;
	}
}

void SerialPort::Listener::Run(HANDLE eventStop) {
	OVERLAPPED overlappedStatus;
	ZeroMemory(&overlappedStatus, sizeof(OVERLAPPED));

	common::Event eventStatus(TRUE, FALSE);
	overlappedStatus.hEvent = eventStatus;

	SetCommMask(pPort_->hPort_, EV_BREAK | EV_CTS | EV_DSR | EV_ERR | EV_RING | EV_RLSD | EV_RXCHAR | EV_RXFLAG | EV_TXEMPTY);

	const int StopEvent = 0;
	const int StatusEvent = 1;
	const int ReadEvent = 2;

	HANDLE waitEvents[3] =  { eventStop, eventStatus, overlappedReadEvent_ };

	DWORD resultEventMask = 0;
	bool bContinue = true, bWaitingOnStatusEvent = false;
	while(bContinue) {
		if(!bWaitingOnStatusEvent) {
			if(WaitCommEvent(pPort_->hPort_, &resultEventMask, &overlappedStatus)) {
				ProcessStatusEvent(resultEventMask);
			}
			else {
				//WaitCommEvent did not complete. Is it pending or did an error occur?
				DWORD errorCode = GetLastError();
				if(errorCode == ERROR_IO_PENDING) {
					bWaitingOnStatusEvent = true;
				}
				else {
					//error in WaitCommEvent
					LogCommError(TEXT("Error in WaitCommEvent. Error: "), errorCode);
					bContinue = false;
				}
			}
		}

		if(bWaitingOnStatusEvent) {
			HRESULT waitResult = WaitForMultipleObjects((bWaitingOnRead_ ? 3 : 2), waitEvents, FALSE, WaitTimeout);
			switch(waitResult) {
				case WAIT_TIMEOUT:
				{
					break;
				}

				case WAIT_FAILED:
				{
					CASPAR_LOG(error) << "WaitForMultipleObjects Failed.";
					bContinue = false;
					break;
				}

				default:
				{
					int alertObject = waitResult - WAIT_OBJECT_0;

					if(alertObject == StopEvent) {
						CASPAR_LOG(info) << "Got stopEvent. Stopping listener-thread.";
						bContinue = false;
					}
					else if(alertObject == StatusEvent) {
						DWORD numBytes = 0;
						if(GetOverlappedResult(pPort_->hPort_, &overlappedStatus, &numBytes, FALSE)) {
							ProcessStatusEvent(resultEventMask);
						}
						else {
							LogCommError(TEXT("GetOverlappedResult Failed (StatusEvent). Error: "), GetLastError());
						}
						bWaitingOnStatusEvent = false;
					}
					else if(alertObject == ReadEvent) {
						overlappedReadEvent_.Reset();
						DWORD numBytesRead = 0;
						if(GetOverlappedResult(pPort_->hPort_, &overlappedRead_, &numBytesRead, FALSE)) {
							bWaitingOnRead_ = false;
							CASPAR_LOG(info) << "Overlapped completion sucessful.";
							OnRead(numBytesRead);
						}
						else {
							LogCommError(TEXT("GetOverlappedResult Failed (ReadEvent). Error: "), GetLastError());
						}
					}
					else {
						//unexpected error
						LogCommError(TEXT("Unexpected wait-result. Result: "), waitResult);
						bContinue = false;
					}
					break;
				}
			}
		}
	}
}

bool SerialPort::Listener::OnUnhandledException(const std::exception& ex) throw() {
	bool bDoRestart = true;

	try 
	{
		CASPAR_LOG(fatal) << "UNHANDLED EXCEPTION in serialport listener. Message: " << ex.what();

		//TODO: Cleanup and prepare for restart
	}
	catch(...)
	{
		bDoRestart = false;
	}

	return bDoRestart;
}


void SerialPort::Listener::ProcessStatusEvent(DWORD eventMask) {
	DWORD errors = 0;
	COMSTAT commStat;
	ZeroMemory(&commStat, sizeof(COMSTAT));

	ClearCommError(pPort_->hPort_, &errors, &commStat);

	if((eventMask & EV_RXCHAR) && (commStat.cbInQue > 0)) {
		queuedChars_ += static_cast<int>(commStat.cbInQue);
		if(!bWaitingOnRead_) {
			DoRead();
		}
		else {
			CASPAR_LOG(error) << "Could not read, already waiting on overlapped completion";
		}
	}
}

void SerialPort::Listener::DoRead() {
	bool bContinue;
	do {
		bContinue = false;
		DWORD numBytesRead = 0;
		int bytesToRead = std::min(InputBufferSize, queuedChars_);
		if(ReadFile(pPort_->hPort_, pInputBuffer_, bytesToRead, &numBytesRead, &overlappedRead_)) {
			bContinue = OnRead(numBytesRead);
		} else {
			DWORD errorCode = GetLastError();
			if(errorCode == ERROR_IO_PENDING) {
				CASPAR_LOG(error) << "Could not complete read. Mark as waiting for overlapped completion";
				bWaitingOnRead_ = true;
			}
			else
				LogCommError(TEXT("ReadFile Failed. Error: "), errorCode);
		}
	}
	while(bContinue);
}

bool ConvertMultiByteToWideChar(char* pSource, int sourceLength, std::vector<wchar_t>& wideBuffer)
{
	//28591 = ISO 8859-1 Latin I
	int charsWritten = 0;
	int wideBufferCapacity = MultiByteToWideChar(28591, 0, pSource, sourceLength, NULL, NULL);
	if(wideBufferCapacity > 0) 
	{
		wideBuffer.resize(wideBufferCapacity);
		charsWritten = MultiByteToWideChar(28591, 0, pSource, sourceLength, &wideBuffer[0], wideBuffer.size());
	}

	wideBuffer.resize(charsWritten);
	return (charsWritten > 0);
}

bool SerialPort::Listener::OnRead(int numBytesRead) {
	queuedChars_ -= numBytesRead;
	_ASSERT(queuedChars_ >= 0);

	//just to be safe
	queuedChars_ = std::max(queuedChars_, 0);

	if(pProtocolStrategy_ != 0) {
		//TODO: Convert from LATIN-1 codepage to wide chars
		if(ConvertMultiByteToWideChar(pInputBuffer_, numBytesRead, wideRecvBuffer_)) {
			pProtocolStrategy_->Parse(&wideRecvBuffer_[0], wideRecvBuffer_.size(), pPort_->pClientInfo_);
		}
		else {
			CASPAR_LOG(error) << "Read failed, could not convert command to UNICODE";
		}
	}

	ZeroMemory(pInputBuffer_, InputBufferSize);

	return  (queuedChars_ > 0);
}

void SerialPort::Write(const std::wstring& str) {
	if(pWriter_ != 0)
		pWriter_->push_back(str);
}

void SerialPort::SerialPortClientInfo::Send(const std::wstring& data) {
	pSerialPort_->Write(data);
}

SerialPort::Writer::Writer(HANDLE port) : port_(port), newStringEvent_(FALSE, FALSE) {
}

void SerialPort::Writer::push_back(const std::wstring& str) {
	tbb::mutex::scoped_lock lock(mutex_);

	sendQueue_.push(str);
	newStringEvent_.Set();
}

bool ConvertWideCharToLatin1(const std::wstring& wideString, std::vector<char>& destBuffer)
{
	//28591 = ISO 8859-1 Latin I
	int bytesWritten = 0;
	int multibyteBufferCapacity = WideCharToMultiByte(28591, 0, wideString.c_str(), static_cast<int>(wideString.length()), 0, 0, NULL, NULL);
	if(multibyteBufferCapacity > 0) 
	{
		destBuffer.resize(multibyteBufferCapacity);
		bytesWritten = WideCharToMultiByte(28591, 0, wideString.c_str(), static_cast<int>(wideString.length()), &destBuffer[0], destBuffer.size(), NULL, NULL);
	}
	destBuffer.resize(bytesWritten);
	return (bytesWritten > 0);
}

void SerialPort::Writer::Run(HANDLE stopEvent) {
	OVERLAPPED overlappedWrite;
	ZeroMemory(&overlappedWrite, sizeof(OVERLAPPED));
	common::Event overlappedWriteEvent(TRUE, FALSE);
	overlappedWrite.hEvent = overlappedWriteEvent;
	bool writePending = false;
	int bytesToSend = 0;

	std::vector<char> currentlySending;
	std::wstring currentlySendingString;
	unsigned int currentlySendingOffset = 0;

	HANDLE waitHandles[3] =  { stopEvent, newStringEvent_, overlappedWriteEvent };

	while(true) {
		HRESULT waitResult = WaitForMultipleObjects(3, waitHandles, FALSE, 2500);
		if(waitResult == WAIT_TIMEOUT)
			continue;
		else if(waitResult == WAIT_FAILED)
			break;

		HRESULT currentEvent = waitResult - WAIT_OBJECT_0;
		if(currentEvent == 0) {			//stopEvent
			//TODO: Cancel ev. pending write?
			break;
		}
		else if(currentEvent == 1) {	//newStringEvent_
			if(!writePending) {
				if(currentlySending.size() == 0)
				{
					tbb::mutex::scoped_lock lock(mutex_);
					//Read the next string in the queue and convert to ISO 8859-1 LATIN-1
					currentlySendingString = sendQueue_.front();
					sendQueue_.pop();
					if(!ConvertWideCharToLatin1(currentlySendingString, currentlySending))
					{
						CASPAR_LOG(error) << "Send failed, could not convert response to ISO 8859-1";
					}
					currentlySendingOffset = 0;
				}

				if(currentlySending.size() > 0) {
					bytesToSend = static_cast<int>(currentlySending.size()-currentlySendingOffset);
					DWORD bytesWritten = 0;
					if(!WriteFile(port_, &currentlySending[0] + currentlySendingOffset, bytesToSend, &bytesWritten, &overlappedWrite)) {
						DWORD errorCode = GetLastError();
						if(errorCode == ERROR_IO_PENDING) {
							writePending = true;
						}
						else {
							//TODO: Log and handle critical error
							CASPAR_LOG(error) << "Failed to send, errorcode: " << errorCode;

							currentlySending.resize(0);
							currentlySendingOffset = 0;
							break;
						}
					}
					else {	//WriteFile completed successfully
						if(bytesToSend == bytesWritten) {
							currentlySending.resize(0);
							currentlySendingOffset = 0;

							CASPAR_LOG(debug) << "Sent serialdata (imediately): " << currentlySendingString.c_str();
						}
						else {
							currentlySendingOffset += bytesWritten;
							newStringEvent_.Set();
						}
					}
				}
			}
		}
		else if(currentEvent == 2) {	//overlappedWriteEvent
			overlappedWriteEvent.Reset();
			DWORD bytesWritten = 0;
			if(!GetOverlappedResult(port_, &overlappedWrite, &bytesWritten, FALSE)) {
				DWORD errorCode = GetLastError();
				CASPAR_LOG(error) << "Failed to send, errorcode: " << errorCode;

				currentlySending.resize(0);
				currentlySendingOffset = 0;
				break;
			}
			else {
				writePending = false;
				if(bytesToSend == bytesWritten) {
					currentlySending.resize(0);
					currentlySendingOffset = 0;

					CASPAR_LOG(debug) << "Sent serialdata (overlapped): " << currentlySendingString.c_str();
					{
						tbb::mutex::scoped_lock lock(mutex_);
						if(sendQueue_.size() > 0)
							newStringEvent_.Set();
					}
				}
				else {
					currentlySendingOffset += bytesWritten;
					newStringEvent_.Set();
				}
			}
		}
	}
}

bool SerialPort::Writer::OnUnhandledException(const std::exception& ex) throw() {
	bool bDoRestart = true;

	try 
	{
		CASPAR_LOG(fatal) << "UNHANDLED EXCEPTION in serialport writer. Message: " << ex.what();

		//TODO: Cleanup and prepare for restart
	}
	catch(...)
	{
		bDoRestart = false;
	}

	return bDoRestart;
}

void LogCommError(const std::wstring& msg, int errorCode) {
	TCHAR strNumber[65];
	_itot_s(errorCode, strNumber, 10);
	std::wstring error = msg;
	error.append(strNumber);
	CASPAR_LOG(error) << error;
}

}	//namespace IO
}	//namespace caspar