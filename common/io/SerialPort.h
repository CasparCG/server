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
 
#pragma once

#include <string>
#include <queue>
#include "..\concurrency\thread.h"
#include "ProtocolStrategy.h"

#include <vector>
#include <tbb\mutex.h>

namespace caspar {
namespace IO {

class SerialPort
{
	class SerialPortClientInfo;
	typedef std::tr1::shared_ptr<SerialPortClientInfo> SerialPortClientInfoPtr;
	friend class SerialPortClientInfo;

	class Listener;
	typedef std::tr1::shared_ptr<Listener> ListenerPtr;
	friend class Listener;

	class Writer;
	typedef std::tr1::shared_ptr<Writer> WriterPtr;
	friend class Writer;

	SerialPort(const SerialPort&);
	SerialPort& operator=(const SerialPort&);

public:
	explicit SerialPort(const ProtocolStrategyPtr& pProtocol, DWORD baudRate, BYTE parity, BYTE dataBits, BYTE stopBits, const std::wstring& portName, bool spy = false);
	virtual ~SerialPort();

	bool Start();
	void Stop();

	void SetProtocolStrategy(ProtocolStrategyPtr pPS) {
		pProtocolStrategy_ = pPS;
	}

private:
	void Setup();
	void Write(const std::wstring&);

	bool spy_;
	ProtocolStrategyPtr		pProtocolStrategy_;
	std::wstring portName_;
	HANDLE hPort_;
	DCB commSettings_;

	common::Thread listenerThread_;
	ListenerPtr pListener_;

	common::Thread writerThread_;
	WriterPtr pWriter_;

	SerialPortClientInfoPtr pClientInfo_;

// Listener
/////////////
	class Listener : public common::IRunnable
	{
		friend SerialPort;

		Listener(const Listener&);
		Listener& operator=(const Listener&);

	public:
		explicit Listener(SerialPort* pSerialPort, ProtocolStrategyPtr pPS);
		virtual ~Listener();

		void Run(HANDLE stopEvent);
		bool OnUnhandledException(const std::exception& ex) throw();

	private:
		void ProcessStatusEvent(DWORD eventMask);
		void DoRead();
		bool OnRead(int numBytesRead);

		static const int WaitTimeout;
		static const int InputBufferSize;

		SerialPort*				pPort_;
		ProtocolStrategyPtr		pProtocolStrategy_;

		common::Event		overlappedReadEvent_;
		OVERLAPPED	overlappedRead_;
		bool		bWaitingOnRead_;

		char*		pInputBuffer_;
		int			queuedChars_;
		std::vector<wchar_t> wideRecvBuffer_;
	};

// Writer
/////////////
	class Writer : public common::IRunnable
	{
	public:
		Writer(HANDLE port);
		virtual ~Writer() 
		{}

	public:
		void push_back(const std::wstring&);

		void Run(HANDLE stopEvent);
		bool OnUnhandledException(const std::exception& ex) throw();

	private:
		common::Event newStringEvent_;
		std::queue<std::wstring> sendQueue_;
		HANDLE port_;
		tbb::mutex mutex_;
	};

// ClientInfo
///////////////
	class SerialPortClientInfo : public caspar::IO::ClientInfo
	{
		SerialPort* pSerialPort_;
	public:
		SerialPortClientInfo(SerialPort* pSP) : pSerialPort_(pSP) {
		}
		virtual ~SerialPortClientInfo() 
		{}

		void Send(const std::wstring& data);
		void Disconnect() 
		{}
	};
};

typedef std::tr1::shared_ptr<SerialPort> SerialPortPtr;

}	//namespace IO
}	//namespace caspar