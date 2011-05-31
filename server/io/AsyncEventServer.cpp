// AsyncEventServer.cpp: implementation of the AsyncEventServer class.
//
//////////////////////////////////////////////////////////////////////

#include "..\stdafx.h"

#include "AsyncEventServer.h"
#include "SocketInfo.h"

#include <string>
#include <algorithm>

namespace caspar {
namespace IO {

using namespace utils;

#define CASPAR_MAXIMUM_SOCKET_CLIENTS	(MAXIMUM_WAIT_OBJECTS-1)	

long AsyncEventServer::instanceCount_ = 0;
//////////////////////////////
// AsyncEventServer constructor
// PARAMS: port(TCP-port the server should listen to)
// COMMENT: Initializes the WinSock2 library
AsyncEventServer::AsyncEventServer(int port) : port_(port)
{
	if(instanceCount_ == 0) {
		WSADATA wsaData;
		if(WSAStartup(MAKEWORD(2,2), &wsaData) != NO_ERROR)
			throw std::exception("Error initializing WinSock2");
		else {
			LOG << TEXT("WinSock2 Initialized.") << LogStream::Flush;
		}
	}

	InterlockedIncrement(&instanceCount_);
}

/////////////////////////////
// AsyncEventServer destructor
AsyncEventServer::~AsyncEventServer() {
	Stop();

	InterlockedDecrement(&instanceCount_);
	if(instanceCount_ == 0)
		WSACleanup();
}

//////////////////////////////
// AsyncEventServer::Start
// RETURNS: true at successful startup
bool AsyncEventServer::Start() {
	if(listenThread_.IsRunning())
		return false;

	socketInfoCollection_.Clear();

	sockaddr_in sockAddr;
	ZeroMemory(&sockAddr, sizeof(sockAddr));
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_addr.s_addr = INADDR_ANY;
	sockAddr.sin_port = htons(port_);
	
	SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(listenSocket == INVALID_SOCKET) {
		LOG << TEXT("Failed to create listenSocket") << LogStream::Flush;
		return false;
	}
	
	pListenSocketInfo_ = SocketInfoPtr(new SocketInfo(listenSocket, this));

	if(WSAEventSelect(pListenSocketInfo_->socket_, pListenSocketInfo_->event_, FD_ACCEPT|FD_CLOSE) == SOCKET_ERROR) {
		LOG << TEXT("Failed to enter EventSelect-mode for listenSocket") << LogStream::Flush;
		return false;
	}

	if(bind(pListenSocketInfo_->socket_, (sockaddr*)&sockAddr, sizeof(sockAddr)) == SOCKET_ERROR) {
		LOG << TEXT("Failed to bind listenSocket") << LogStream::Flush;
		return false;
	}

	if(listen(pListenSocketInfo_->socket_, SOMAXCONN) == SOCKET_ERROR) {
		LOG << TEXT("Failed to listen") << LogStream::Flush;
		return false;
	}

	socketInfoCollection_.AddSocketInfo(pListenSocketInfo_);

	//start thread: the entrypoint is Run(EVENT stopEvent)
	if(!listenThread_.Start(this)) {
		LOG << TEXT("Failed to create ListenThread") << LogStream::Flush;
		return false;
	}

	LOG << TEXT("Listener successfully initialized") << LogStream::Flush;
	return true;
}

void AsyncEventServer::Run(HANDLE stopEvent)
{
	WSANETWORKEVENTS networkEvents;

	HANDLE waitHandlesCopy[MAXIMUM_WAIT_OBJECTS];
	waitHandlesCopy[0] = stopEvent;

	while(true)	{
		//Update local copy of the array of wait-handles if nessecery
		if(socketInfoCollection_.IsDirty()) {
			socketInfoCollection_.CopyCollectionToArray(&(waitHandlesCopy[1]), CASPAR_MAXIMUM_SOCKET_CLIENTS);
			socketInfoCollection_.ClearDirty();
		}

		DWORD waitResult = WSAWaitForMultipleEvents(min(static_cast<DWORD>(socketInfoCollection_.Size()+1), MAXIMUM_WAIT_OBJECTS), waitHandlesCopy, FALSE, 1500, FALSE);
		if(waitResult == WAIT_TIMEOUT)
			continue;
		else if(waitResult == WAIT_FAILED)
			break;
		else {
			DWORD eventIndex = waitResult - WAIT_OBJECT_0;

			HANDLE waitEvent = waitHandlesCopy[eventIndex];
			SocketInfoPtr pSocketInfo;

			if(eventIndex == 0)	//stopEvent
				break;
			else if(socketInfoCollection_.FindSocketInfo(waitEvent, pSocketInfo)) {
				WSAEnumNetworkEvents(pSocketInfo->socket_, waitEvent, &networkEvents);

				if(networkEvents.lNetworkEvents & FD_ACCEPT) {
					if(networkEvents.iErrorCode[FD_ACCEPT_BIT] == 0)
						OnAccept(pSocketInfo);
					else {
						LOG << LogLevel::Debug << TEXT("OnAccept (ErrorCode: ") << networkEvents.iErrorCode[FD_ACCEPT_BIT] << TEXT(")") << LogStream::Flush;
						OnError(waitEvent, networkEvents.iErrorCode[FD_ACCEPT_BIT]);
					}
				}

				if(networkEvents.lNetworkEvents & FD_CLOSE) {
					if(networkEvents.iErrorCode[FD_CLOSE_BIT] == 0)
						OnClose(pSocketInfo);
					else {
						LOG << LogLevel::Debug << TEXT("OnClose (ErrorCode: ") << networkEvents.iErrorCode[FD_CLOSE_BIT] << TEXT(")") << LogStream::Flush;
						OnError(waitEvent, networkEvents.iErrorCode[FD_CLOSE_BIT]);
					}
					continue;
				}

				if(networkEvents.lNetworkEvents & FD_READ) {
					if(networkEvents.iErrorCode[FD_READ_BIT] == 0)
						OnRead(pSocketInfo);
					else {
						LOG << LogLevel::Debug << TEXT("OnRead (ErrorCode: ") << networkEvents.iErrorCode[FD_READ_BIT] << TEXT(")") << LogStream::Flush;
						OnError(waitEvent, networkEvents.iErrorCode[FD_READ_BIT]);
					}
				}

				if(networkEvents.lNetworkEvents & FD_WRITE) {
					if(networkEvents.iErrorCode[FD_WRITE_BIT] == 0)
						OnWrite(pSocketInfo);
					else {
						LOG << LogLevel::Debug << TEXT("OnWrite (ErrorCode: ") << networkEvents.iErrorCode[FD_WRITE_BIT] << TEXT(")") << LogStream::Flush;
						OnError(waitEvent, networkEvents.iErrorCode[FD_WRITE_BIT]);
					}
				}
			}
			else {
				//Could not find the waitHandle in the SocketInfoCollection.
				//It must have been removed during the last call to WSAWaitForMultipleEvents
			}
		}
	}
}

bool AsyncEventServer::OnUnhandledException(const std::exception& ex) throw() {
	bool bDoRestart = true;

	try 
	{
		LOG << TEXT("UNHANDLED EXCEPTION in TCPServers listeningthread. Message: ") << ex.what() << LogStream::Flush;
	}
	catch(...)
	{
		bDoRestart = false;
	}

	return bDoRestart;
}

///////////////////////////////
// AsyncEventServer:Stop
// COMMENT: Shuts down
void AsyncEventServer::Stop()
{
	//TODO: initiate shutdown on all clients connected
//	for(int i=0; i < _totalActiveSockets; ++i) {
//		shutdown(_pSocketInfo[i]->_socket, SD_SEND);
//	}

	if(!listenThread_.Stop()) {
		LOG << TEXT("Wait for listenThread timed out.") << LogStream::Flush;
	}

	socketInfoCollection_.Clear();
}

////////////////////////////////////////////////////////////////////
//
// MESSAGE HANDLERS   
//
////////////////////////////////////////////////////////////////////


//////////////////////////////
// AsyncEventServer::OnAccept
// PARAMS: ...
// COMMENT: Called when a new client connects
bool AsyncEventServer::OnAccept(SocketInfoPtr& pSI) {
	sockaddr_in	clientAddr;
	int addrSize = sizeof(clientAddr);
	SOCKET clientSocket = WSAAccept(pSI->socket_, (sockaddr*)&clientAddr, &addrSize, NULL, NULL);
	if(clientSocket == INVALID_SOCKET) {
		LogSocketError(TEXT("Accept"));
		return false;
	}

	SocketInfoPtr pClientSocket(new SocketInfo(clientSocket, this));

	//Determine if we can handle one more client
	if(socketInfoCollection_.Size() >= CASPAR_MAXIMUM_SOCKET_CLIENTS) {
		LOG << TEXT("Could not accept (too many connections).");
		return true;
	}

	if(WSAEventSelect(pClientSocket->socket_, pClientSocket->event_, FD_READ | FD_WRITE | FD_CLOSE) == SOCKET_ERROR) {
		LogSocketError(TEXT("Accept (failed create event for new client)"));
		return false;
	}

	TCHAR addressBuffer[32];
	MultiByteToWideChar(CP_ACP, 0, inet_ntoa(clientAddr.sin_addr), -1, addressBuffer, 32);
	pClientSocket->host_ = addressBuffer;

	socketInfoCollection_.AddSocketInfo(pClientSocket);

	LOG << TEXT("Accepted connection from ") << pClientSocket->host_.c_str() << LogStream::Flush;

	return true;
}

bool ConvertMultiByteToWideChar(char* pSource, int sourceLength, caspar::utils::DataBuffer<wchar_t>& wideBuffer, int& countLeftovers)
{
	countLeftovers = 0;
	//check from the end of pSource for ev. uncompleted UTF-8 byte sequence
	if(pSource[sourceLength-1] & 0x80) {
		//The last byte is part of a multibyte sequence. If the sequence is not complete, we need to save the partial sequence
		int bytesToCheck = min(4, sourceLength);	//a sequence is contains a maximum of 4 bytes
		int currentLeftoverIndex = sourceLength-1;
		for(; bytesToCheck > 0; --bytesToCheck, --currentLeftoverIndex) {
			++countLeftovers;
			if(pSource[currentLeftoverIndex] & 0x80) {
				if(pSource[currentLeftoverIndex] & 0x40) { //The two high-bits are set, this is the "header"
					int expectedSequenceLength = 2;
					if(pSource[currentLeftoverIndex] & 0x20)
						++expectedSequenceLength;
					if(pSource[currentLeftoverIndex] & 0x10)
						++expectedSequenceLength;

					if(countLeftovers < expectedSequenceLength) {
						//The sequence is incomplete. Leave the leftovers to be interpreted with the next call
						break;
					}
					//The sequence is complete, there are no leftovers. 
					//...OR...
					//error. Let the conversion-function take the hit.
					countLeftovers = 0;
					break;
				}
			}
			else {
				//error. Let the conversion-function take the hit.
				countLeftovers = 0;
				break;
			}
		}
		if(countLeftovers == 4) {
			//error. Let the conversion-function take the hit.
			countLeftovers = 0;
		}
	}

	int charsWritten = 0;
	int sourceBytesToProcess = sourceLength-countLeftovers;
	int wideBufferCapacity = MultiByteToWideChar(CP_UTF8, 0, pSource, sourceBytesToProcess, NULL, NULL);
	if(wideBufferCapacity > 0) 
	{
		wideBuffer.Realloc(wideBufferCapacity);
		charsWritten = MultiByteToWideChar(CP_UTF8, 0, pSource, sourceBytesToProcess, wideBuffer.GetPtr(), wideBuffer.GetCapacity());
	}
	//copy the leftovers to the front of the buffer
	if(countLeftovers > 0) {
		memcpy(pSource, &(pSource[sourceBytesToProcess]), countLeftovers);
	}

	wideBuffer.SetLength(charsWritten);
	return (charsWritten > 0);
}

//////////////////////////////
// AsyncEventServer::OnRead
// PARAMS: ...
// COMMENT: Called then something arrives on the socket that has to be read
bool AsyncEventServer::OnRead(SocketInfoPtr& pSI) {
	int recvResult = SOCKET_ERROR;

	int maxRecvLength = sizeof(pSI->recvBuffer_)-pSI->recvLeftoverOffset_;
	recvResult = recv(pSI->socket_, pSI->recvBuffer_+pSI->recvLeftoverOffset_, maxRecvLength, 0);
	while(recvResult != SOCKET_ERROR) {
		if(recvResult == 0) {
			LOG << TEXT("Client ") << pSI->host_.c_str() << TEXT(" disconnected") << LogStream::Flush;

			socketInfoCollection_.RemoveSocketInfo(pSI);
			return true;
		}

		if(pProtocolStrategy_ != 0) {
			//Convert to widechar
			if(ConvertMultiByteToWideChar(pSI->recvBuffer_, recvResult + pSI->recvLeftoverOffset_, pSI->wideRecvBuffer_, pSI->recvLeftoverOffset_))
				pProtocolStrategy_->Parse(pSI->wideRecvBuffer_.GetPtr(), pSI->wideRecvBuffer_.GetLength(), pSI);
			else
			{
				LOG << TEXT("Read from ") << pSI->host_.c_str() << TEXT(" failed, could not convert command to UNICODE") << LogStream::Flush;
			}
		}

		maxRecvLength = sizeof(pSI->recvBuffer_)-pSI->recvLeftoverOffset_;
		recvResult = recv(pSI->socket_, pSI->recvBuffer_+pSI->recvLeftoverOffset_, maxRecvLength, 0);
	}

	if(recvResult == SOCKET_ERROR) {
		int errorCode = WSAGetLastError();
		if(errorCode == WSAEWOULDBLOCK)
			return true;
		else {
			LogSocketError(TEXT("Read"), errorCode);
			OnError(pSI->event_, errorCode);
		}
	}

	return false;
}

//////////////////////////////
// AsyncEventServer::OnWrite
// PARAMS: ...
// COMMENT: Called when the socket is ready to send more data
void AsyncEventServer::OnWrite(SocketInfoPtr& pSI) {
	DoSend(*pSI);	
}

bool ConvertWideCharToMultiByte(const std::wstring& wideString, caspar::utils::DataBuffer<char>& destBuffer)
{
	int bytesWritten = 0;
	int multibyteBufferCapacity = WideCharToMultiByte(CP_UTF8, 0, wideString.c_str(), static_cast<int>(wideString.length()), 0, 0, NULL, NULL);
	if(multibyteBufferCapacity > 0) 
	{
		destBuffer.Realloc(multibyteBufferCapacity);
		bytesWritten = WideCharToMultiByte(CP_UTF8, 0, wideString.c_str(), static_cast<int>(wideString.length()), destBuffer.GetPtr(), destBuffer.GetCapacity(), NULL, NULL);
	}
	destBuffer.SetLength(bytesWritten);
	return (bytesWritten > 0);
}

void AsyncEventServer::DoSend(SocketInfo& socketInfo) {
	//Locks the socketInfo-object so that no one else tampers with the sendqueue at the same time
	SocketInfo::Lock lock(socketInfo);

	while(!socketInfo.sendQueue_.empty() || socketInfo.currentlySending_.GetLength() > 0) {
		if(socketInfo.currentlySending_.GetLength() == 0) {
			//Read the next string in the queue and convert to UTF-8
			if(!ConvertWideCharToMultiByte(socketInfo.sendQueue_.front(), socketInfo.currentlySending_))
			{
				LOG << TEXT("Send to ") << socketInfo.host_.c_str() << TEXT(" failed, could not convert response to UTF-8") << LogStream::Flush;
			}
			socketInfo.currentlySendingOffset_ = 0;
		}

		if(socketInfo.currentlySending_.GetLength() > 0) {
			int bytesToSend = static_cast<int>(socketInfo.currentlySending_.GetLength()-socketInfo.currentlySendingOffset_);
			int sentBytes = send(socketInfo.socket_, socketInfo.currentlySending_.GetPtr(socketInfo.currentlySendingOffset_), bytesToSend, 0);
			if(sentBytes == SOCKET_ERROR) {
				int errorCode = WSAGetLastError();
				if(errorCode == WSAEWOULDBLOCK) {
					LOG << LogLevel::Debug << TEXT("Send to ") << socketInfo.host_.c_str() << TEXT(" would block, sending later") << LogStream::Flush;
					break;
				}
				else {
					LogSocketError(TEXT("Send"), errorCode);
					OnError(socketInfo.event_, errorCode);

					socketInfo.currentlySending_.SetLength(0);
					socketInfo.currentlySendingOffset_ = 0;
					socketInfo.sendQueue_.pop();
					break;
				}
			}
			else {
				if(sentBytes == bytesToSend) {
					if(sentBytes < 200)
						LOG << LogLevel::Verbose << TEXT("Sent ") << socketInfo.sendQueue_.front().c_str() << TEXT(" to ") << socketInfo.host_.c_str() << LogStream::Flush;
					else
						LOG << LogLevel::Verbose << TEXT("Sent more than 200 bytes to ") << socketInfo.host_.c_str() << LogStream::Flush;

					socketInfo.currentlySending_.SetLength(0);
					socketInfo.currentlySendingOffset_ = 0;
					socketInfo.sendQueue_.pop();
				}
				else {
					socketInfo.currentlySendingOffset_ += sentBytes;
					LOG << LogLevel::Verbose << TEXT("Sent partial message to ") << socketInfo.host_.c_str() << LogStream::Flush;
				}
			}
		}
		else
			socketInfo.sendQueue_.pop();
	}
}

//////////////////////////////
// AsyncEventServer::OnClose
// PARAMS: ...
// COMMENT: Called when a client disconnects / is disconnected
void AsyncEventServer::OnClose(SocketInfoPtr& pSI) {
	LOG << TEXT("Client ") << pSI->host_.c_str() << TEXT(" was disconnected") << LogStream::Flush;

	socketInfoCollection_.RemoveSocketInfo(pSI);
}

//////////////////////////////
// AsyncEventServer::OnError
// PARAMS: ...
// COMMENT: Called when an errorcode is recieved
void AsyncEventServer::OnError(HANDLE waitEvent, int errorCode) {
	if(errorCode == WSAENETDOWN || errorCode == WSAECONNABORTED || errorCode == WSAECONNRESET || errorCode == WSAESHUTDOWN || errorCode == WSAETIMEDOUT || errorCode == WSAENOTCONN || errorCode == WSAENETRESET) {
		SocketInfoPtr pSocketInfo;
		if(socketInfoCollection_.FindSocketInfo(waitEvent, pSocketInfo)) {
			LOG << TEXT("Client ") << pSocketInfo->host_.c_str() << TEXT(" was disconnected, Errorcode ") << errorCode << LogStream::Flush;
		}

		socketInfoCollection_.RemoveSocketInfo(waitEvent);
	}
}

//////////////////////////////
// AsyncEventServer::DisconnectClient
// PARAMS: ...
// COMMENT: The client is removed from the actual client-list when an FD_CLOSE notification is recieved
void AsyncEventServer::DisconnectClient(SocketInfo& socketInfo) {
	int result = shutdown(socketInfo.socket_, SD_SEND);
	if(result == SOCKET_ERROR)
		OnError(socketInfo.event_, result);
}

//////////////////////////////
// AsyncEventServer::LogSocketError
void AsyncEventServer::LogSocketError(const TCHAR* pStr, int socketError) {
	if(socketError == 0)
		socketError = WSAGetLastError();

	LOG << TEXT("Failed to ") << pStr << TEXT(" Errorcode: ") << socketError << LogStream::Flush;
}


//////////////////////////////
//  SocketInfoCollection
//////////////////////////////

AsyncEventServer::SocketInfoCollection::SocketInfoCollection() : bDirty_(false) {
}

AsyncEventServer::SocketInfoCollection::~SocketInfoCollection() {
}

bool AsyncEventServer::SocketInfoCollection::AddSocketInfo(SocketInfoPtr& pSocketInfo) {
	Lock lock(*this);

	waitEvents_.resize(waitEvents_.size()+1);
	bool bSuccess = socketInfoMap_.insert(SocketInfoMap::value_type(pSocketInfo->event_, pSocketInfo)).second;
	if(bSuccess) {
		waitEvents_[waitEvents_.size()-1] = pSocketInfo->event_;
		bDirty_ = true;
	}

	return bSuccess;
}

void AsyncEventServer::SocketInfoCollection::RemoveSocketInfo(SocketInfoPtr& pSocketInfo) {
	Lock lock(*this);

	if(pSocketInfo != 0) {
		HANDLE waitEvent = pSocketInfo->event_;
		socketInfoMap_.erase(waitEvent);

		HandleVector::iterator it = std::find(waitEvents_.begin(), waitEvents_.end(), waitEvent);
		if(it != waitEvents_.end()) {
			std::swap((*it), waitEvents_.back());
			waitEvents_.resize(waitEvents_.size()-1);

			bDirty_ = true;
		}
	}
}
void AsyncEventServer::SocketInfoCollection::RemoveSocketInfo(HANDLE waitEvent) {
	Lock lock(*this);

	socketInfoMap_.erase(waitEvent);

	HandleVector::iterator it = std::find(waitEvents_.begin(), waitEvents_.end(), waitEvent);
	if(it != waitEvents_.end()) {
		std::swap((*it), waitEvents_.back());
		waitEvents_.resize(waitEvents_.size()-1);

		bDirty_ = true;
	}
}

bool AsyncEventServer::SocketInfoCollection::FindSocketInfo(HANDLE key, SocketInfoPtr& pResult) {
	Lock lock(*this);

	SocketInfoMap::iterator it = socketInfoMap_.find(key);
	SocketInfoMap::iterator end = socketInfoMap_.end();
	if(it != end)
		pResult = it->second;

	return (it != end);
}

void AsyncEventServer::SocketInfoCollection::CopyCollectionToArray(HANDLE* pDest, int maxCount) {
	Lock lock(*this);

	memcpy(pDest, &(waitEvents_[0]), min( maxCount, static_cast<int>(waitEvents_.size()) ) * sizeof(HANDLE) );
}

void AsyncEventServer::SocketInfoCollection::Clear() {
	Lock lock(*this);

	socketInfoMap_.clear();
	waitEvents_.clear();
}

}	//namespace IO
}	//namespace caspar