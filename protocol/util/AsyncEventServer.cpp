/*
* Copyright (c) 2011 Sveriges Television AB <info@casparcg.com>
*
* This file is part of CasparCG (www.casparcg.com).
*
* CasparCG is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* CasparCG is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with CasparCG. If not, see <http://www.gnu.org/licenses/>.
*
* Author: Nicklas P Andersson
*/

 
// AsyncEventServer.cpp: implementation of the AsyncEventServer class.
//
//////////////////////////////////////////////////////////////////////

#include "../stdafx.h"

#include "AsyncEventServer.h"
#include "SocketInfo.h"

#include <common/log/log.h>
#include <string>
#include <algorithm>
#include <boost/algorithm/string/replace.hpp>

#if defined(_MSC_VER)
#pragma warning (push, 1) // TODO: Legacy code, just disable warnings, will replace with boost::asio in future
#endif

namespace caspar { namespace IO {
	
#define CASPAR_MAXIMUM_SOCKET_CLIENTS	(MAXIMUM_WAIT_OBJECTS-1)	

long AsyncEventServer::instanceCount_ = 0;
//////////////////////////////
// AsyncEventServer constructor
// PARAMS: port(TCP-port the server should listen to)
// COMMENT: Initializes the WinSock2 library
AsyncEventServer::AsyncEventServer(const safe_ptr<IProtocolStrategy>& pProtocol, int port) : port_(port), pProtocolStrategy_(pProtocol)
{
	if(instanceCount_ == 0) {
		WSADATA wsaData;
		if(WSAStartup(MAKEWORD(2,2), &wsaData) != NO_ERROR)
			throw std::exception("Error initializing WinSock2");
		else {
			CASPAR_LOG(info) << "WinSock2 Initialized.";
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

void AsyncEventServer::SetClientDisconnectHandler(ClientDisconnectEvent handler) {
	socketInfoCollection_.onSocketInfoRemoved = handler;
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
		CASPAR_LOG(error) << "Failed to create listenSocket";
		return false;
	}
	
	pListenSocketInfo_ = SocketInfoPtr(new SocketInfo(listenSocket, this));

	if(WSAEventSelect(pListenSocketInfo_->socket_, pListenSocketInfo_->event_, FD_ACCEPT|FD_CLOSE) == SOCKET_ERROR) {
		CASPAR_LOG(error) << "Failed to enter EventSelect-mode for listenSocket";
		return false;
	}

	if(bind(pListenSocketInfo_->socket_, (sockaddr*)&sockAddr, sizeof(sockAddr)) == SOCKET_ERROR) {
		CASPAR_LOG(error) << "Failed to bind listenSocket";
		return false;
	}

	if(listen(pListenSocketInfo_->socket_, SOMAXCONN) == SOCKET_ERROR) {
		CASPAR_LOG(error) << "Failed to listen";
		return false;
	}

	socketInfoCollection_.AddSocketInfo(pListenSocketInfo_);

	//start thread: the entrypoint is Run(EVENT stopEvent)
	if(!listenThread_.Start(this)) {
		CASPAR_LOG(error) << "Failed to create ListenThread";
		return false;
	}

	CASPAR_LOG(info) << "Listener successfully initialized";
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

		DWORD waitResult = WSAWaitForMultipleEvents(std::min<DWORD>(static_cast<DWORD>(socketInfoCollection_.Size()+1), MAXIMUM_WAIT_OBJECTS), waitHandlesCopy, FALSE, 1500, FALSE);
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
						CASPAR_LOG(debug) << "OnAccept (ErrorCode: " << networkEvents.iErrorCode[FD_ACCEPT_BIT] << TEXT(")");
						OnError(waitEvent, networkEvents.iErrorCode[FD_ACCEPT_BIT]);
					}
				}

				if(networkEvents.lNetworkEvents & FD_CLOSE) {
					if(networkEvents.iErrorCode[FD_CLOSE_BIT] == 0)
						OnClose(pSocketInfo);
					else {
						CASPAR_LOG(debug) << "OnClose (ErrorCode: " << networkEvents.iErrorCode[FD_CLOSE_BIT] << TEXT(")");
						OnError(waitEvent, networkEvents.iErrorCode[FD_CLOSE_BIT]);
					}
					continue;
				}

				if(networkEvents.lNetworkEvents & FD_READ) {
					if(networkEvents.iErrorCode[FD_READ_BIT] == 0)
						OnRead(pSocketInfo);
					else {
						CASPAR_LOG(debug) << "OnRead (ErrorCode: " << networkEvents.iErrorCode[FD_READ_BIT] << TEXT(")");
						OnError(waitEvent, networkEvents.iErrorCode[FD_READ_BIT]);
					}
				}

				if(networkEvents.lNetworkEvents & FD_WRITE) {
					if(networkEvents.iErrorCode[FD_WRITE_BIT] == 0)
						OnWrite(pSocketInfo);
					else {
						CASPAR_LOG(debug) << "OnWrite (ErrorCode: " << networkEvents.iErrorCode[FD_WRITE_BIT] << TEXT(")");
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
		CASPAR_LOG(fatal) << "UNHANDLED EXCEPTION in TCPServers listeningthread. Message: " << ex.what();
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
		CASPAR_LOG(warning) << "Wait for listenThread timed out.";
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

	try
	{ // !!!!!!!!!!!!
		if(socketInfoCollection_.Size() >= 50) 
		{
			HANDLE handle = nullptr;
			{
				tbb::mutex::scoped_lock lock(socketInfoCollection_.mutex_);				
				handle = *(socketInfoCollection_.waitEvents_.begin()+0);
				if(handle == pListenSocketInfo_->event_)
					handle = *(socketInfoCollection_.waitEvents_.begin()+1);
			}
			socketInfoCollection_.RemoveSocketInfo(handle);
			CASPAR_LOG(error) << " Too many connections. Removed last used connection.";
		}
	}catch(...){}

	//Determine if we can handle one more client
	if(socketInfoCollection_.Size() >= CASPAR_MAXIMUM_SOCKET_CLIENTS) {
		CASPAR_LOG(error) << "Could not accept ) << too many connections).";
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

	CASPAR_LOG(info) << "Accepted connection from " << pClientSocket->host_.c_str() << " " << socketInfoCollection_.Size();

	return true;
}

bool ConvertMultiByteToWideChar(UINT codePage, char* pSource, int sourceLength, std::vector<wchar_t>& wideBuffer, int& countLeftovers)
{
	if(codePage == CP_UTF8) {
		countLeftovers = 0;
		//check from the end of pSource for ev. uncompleted UTF-8 byte sequence
		if(pSource[sourceLength-1] & 0x80) {
			//The last byte is part of a multibyte sequence. If the sequence is not complete, we need to save the partial sequence
			int bytesToCheck = std::min(4, sourceLength);	//a sequence contains a maximum of 4 bytes
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
	}

	int charsWritten = 0;
	int sourceBytesToProcess = sourceLength-countLeftovers;
	int wideBufferCapacity = MultiByteToWideChar(codePage, 0, pSource, sourceBytesToProcess, NULL, NULL);
	if(wideBufferCapacity > 0) 
	{
		wideBuffer.resize(wideBufferCapacity);
		charsWritten = MultiByteToWideChar(codePage, 0, pSource, sourceBytesToProcess, &wideBuffer[0], wideBuffer.size());
	}
	//copy the leftovers to the front of the buffer
	if(countLeftovers > 0) {
		memcpy(pSource, &(pSource[sourceBytesToProcess]), countLeftovers);
	}

	wideBuffer.resize(charsWritten);
	return (charsWritten > 0);
}

//////////////////////////////
// AsyncEventServer::OnRead
// PARAMS: ...
// COMMENT: Called then something arrives on the socket that has to be read
bool AsyncEventServer::OnRead(SocketInfoPtr& pSI) {
	int recvResult = SOCKET_ERROR;
	
	try
	{ // !!!!!!!!!!!!
		if(pSI)
		{
			tbb::mutex::scoped_lock lock(socketInfoCollection_.mutex_);
			auto it = std::find(socketInfoCollection_.waitEvents_.begin(), socketInfoCollection_.waitEvents_.end(), pSI->event_);
			if(it != socketInfoCollection_.waitEvents_.end())
			{
				auto handle = *it;
				socketInfoCollection_.waitEvents_.erase(it);
				socketInfoCollection_.waitEvents_.push_back(handle);
			}
		}
	}catch(...){}

	int maxRecvLength = sizeof(pSI->recvBuffer_)-pSI->recvLeftoverOffset_;
	recvResult = recv(pSI->socket_, pSI->recvBuffer_+pSI->recvLeftoverOffset_, maxRecvLength, 0);
	while(recvResult != SOCKET_ERROR) {
		if(recvResult == 0) {
			CASPAR_LOG(info) << "Client " << pSI->host_.c_str() << TEXT(" disconnected");

			socketInfoCollection_.RemoveSocketInfo(pSI);
			return true;
		}

		//Convert to widechar
		if(ConvertMultiByteToWideChar(pProtocolStrategy_->GetCodepage(), pSI->recvBuffer_, recvResult + pSI->recvLeftoverOffset_, pSI->wideRecvBuffer_, pSI->recvLeftoverOffset_))		
			pProtocolStrategy_->Parse(&pSI->wideRecvBuffer_[0], pSI->wideRecvBuffer_.size(), pSI);		
		else			
			CASPAR_LOG(error) << "Read from " << pSI->host_.c_str() << TEXT(" failed, could not convert command to UNICODE");
			
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

bool ConvertWideCharToMultiByte(UINT codePage, const std::wstring& wideString, std::vector<char>& destBuffer)
{
	int bytesWritten = 0;
	int multibyteBufferCapacity = WideCharToMultiByte(codePage, 0, wideString.c_str(), static_cast<int>(wideString.length()), 0, 0, NULL, NULL);
	if(multibyteBufferCapacity > 0) 
	{
		destBuffer.resize(multibyteBufferCapacity);
		bytesWritten = WideCharToMultiByte(codePage, 0, wideString.c_str(), static_cast<int>(wideString.length()), &destBuffer[0], destBuffer.size(), NULL, NULL);
	}
	destBuffer.resize(bytesWritten);
	return (bytesWritten > 0);
}

void AsyncEventServer::DoSend(SocketInfo& socketInfo) {
	//Locks the socketInfo-object so that no one else tampers with the sendqueue at the same time
	tbb::mutex::scoped_lock lock(mutex_);

	while(!socketInfo.sendQueue_.empty() || socketInfo.currentlySending_.size() > 0) {
		if(socketInfo.currentlySending_.size() == 0) {
			//Read the next string in the queue and convert to UTF-8
			if(!ConvertWideCharToMultiByte(pProtocolStrategy_->GetCodepage(), socketInfo.sendQueue_.front(), socketInfo.currentlySending_))
			{
				CASPAR_LOG(error) << "Send to " << socketInfo.host_.c_str() << TEXT(" failed, could not convert response to UTF-8");
			}
			socketInfo.currentlySendingOffset_ = 0;
		}

		if(socketInfo.currentlySending_.size() > 0) {
			int bytesToSend = static_cast<int>(socketInfo.currentlySending_.size()-socketInfo.currentlySendingOffset_);
			int sentBytes = send(socketInfo.socket_, &socketInfo.currentlySending_[0] + socketInfo.currentlySendingOffset_, bytesToSend, 0);
			if(sentBytes == SOCKET_ERROR) {
				int errorCode = WSAGetLastError();
				if(errorCode == WSAEWOULDBLOCK) {
					CASPAR_LOG(debug) << "Send to " << socketInfo.host_.c_str() << TEXT(" would block, sending later");
					break;
				}
				else {
					LogSocketError(TEXT("Send"), errorCode);
					OnError(socketInfo.event_, errorCode);

					socketInfo.currentlySending_.resize(0);
					socketInfo.currentlySendingOffset_ = 0;
					socketInfo.sendQueue_.pop();
					break;
				}
			}
			else {
				if(sentBytes == bytesToSend) {
					
					if(sentBytes < 512)
					{
						boost::replace_all(socketInfo.sendQueue_.front(), L"\n", L"\\n");
						boost::replace_all(socketInfo.sendQueue_.front(), L"\r", L"\\r");
						CASPAR_LOG(info) << L"Sent message to " << socketInfo.host_.c_str() << L": " << socketInfo.sendQueue_.front().c_str();
					}
					else
						CASPAR_LOG(info) << "Sent more than 512 bytes to " << socketInfo.host_.c_str();

					socketInfo.currentlySending_.resize(0);
					socketInfo.currentlySendingOffset_ = 0;
					socketInfo.sendQueue_.pop();
				}
				else {
					socketInfo.currentlySendingOffset_ += sentBytes;
					CASPAR_LOG(info) << "Sent partial message to " << socketInfo.host_.c_str();
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
	CASPAR_LOG(info) << "Client " << pSI->host_.c_str() << TEXT(" was disconnected");

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
			CASPAR_LOG(info) << "Client " << pSocketInfo->host_.c_str() << TEXT(" was disconnected, Errorcode ") << errorCode;
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

	CASPAR_LOG(error) << "Failed to " << pStr << TEXT(" Errorcode: ") << socketError;
}


//////////////////////////////
//  SocketInfoCollection
//////////////////////////////

AsyncEventServer::SocketInfoCollection::SocketInfoCollection() : bDirty_(false) {
}

AsyncEventServer::SocketInfoCollection::~SocketInfoCollection() {
}

bool AsyncEventServer::SocketInfoCollection::AddSocketInfo(SocketInfoPtr& pSocketInfo) {
	tbb::mutex::scoped_lock lock(mutex_);

	waitEvents_.resize(waitEvents_.size()+1);
	bool bSuccess = socketInfoMap_.insert(SocketInfoMap::value_type(pSocketInfo->event_, pSocketInfo)).second;
	if(bSuccess) {
		waitEvents_[waitEvents_.size()-1] = pSocketInfo->event_;
		bDirty_ = true;
	}

	return bSuccess;
}

void AsyncEventServer::SocketInfoCollection::RemoveSocketInfo(SocketInfoPtr& pSocketInfo) {
	if(pSocketInfo != 0) {
		RemoveSocketInfo(pSocketInfo->event_);
	}
}
void AsyncEventServer::SocketInfoCollection::RemoveSocketInfo(HANDLE waitEvent) {
	tbb::mutex::scoped_lock lock(mutex_);

	//Find instance
	SocketInfoPtr pSocketInfo;
	SocketInfoMap::iterator it = socketInfoMap_.find(waitEvent);
	SocketInfoMap::iterator end = socketInfoMap_.end();
	if(it != end)
		pSocketInfo = it->second;

	if(pSocketInfo) {
		pSocketInfo->pServer_ = NULL;

		socketInfoMap_.erase(waitEvent);

		HandleVector::iterator it = std::find(waitEvents_.begin(), waitEvents_.end(), waitEvent);
		if(it != waitEvents_.end()) {
			std::swap((*it), waitEvents_.back());
			waitEvents_.resize(waitEvents_.size()-1);

			bDirty_ = true;
		}
	}
	if(onSocketInfoRemoved)
		onSocketInfoRemoved(pSocketInfo);
}

bool AsyncEventServer::SocketInfoCollection::FindSocketInfo(HANDLE key, SocketInfoPtr& pResult) {
	tbb::mutex::scoped_lock lock(mutex_);

	SocketInfoMap::iterator it = socketInfoMap_.find(key);
	SocketInfoMap::iterator end = socketInfoMap_.end();
	if(it != end)
		pResult = it->second;

	return (it != end);
}

void AsyncEventServer::SocketInfoCollection::CopyCollectionToArray(HANDLE* pDest, int maxCount) {
	tbb::mutex::scoped_lock lock(mutex_);

	memcpy(pDest, &(waitEvents_[0]), std::min( maxCount, static_cast<int>(waitEvents_.size()) ) * sizeof(HANDLE) );
}

void AsyncEventServer::SocketInfoCollection::Clear() {
	tbb::mutex::scoped_lock lock(mutex_);

	socketInfoMap_.clear();
	waitEvents_.clear();
}

}	//namespace IO
}	//namespace caspar