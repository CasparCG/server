#pragma once

#include <string>
#include <queue>
#include "..\utils\thread.h"
#include "..\utils\lockable.h"
#include "ProtocolStrategy.h"
#include "..\controller.h"
#include "..\utils\databuffer.h"

namespace caspar {
namespace IO {

class SerialPort : public caspar::IController
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
	explicit SerialPort(DWORD baudRate, BYTE parity, BYTE dataBits, BYTE stopBits, const tstring& portName);
	virtual ~SerialPort();

	bool Start();
	void Stop();

	void SetProtocolStrategy(ProtocolStrategyPtr pPS) {
		pProtocolStrategy_ = pPS;
	}

private:
	void Setup();
	void Write(const tstring&);

	ProtocolStrategyPtr		pProtocolStrategy_;
	tstring portName_;
	HANDLE hPort_;
	DCB commSettings_;

	utils::Thread listenerThread_;
	ListenerPtr pListener_;

	utils::Thread writerThread_;
	WriterPtr pWriter_;

	SerialPortClientInfoPtr pClientInfo_;

// Listener
/////////////
	class Listener : public utils::IRunnable
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

		utils::Event		overlappedReadEvent_;
		OVERLAPPED	overlappedRead_;
		bool		bWaitingOnRead_;

		char*		pInputBuffer_;
		int			queuedChars_;
		utils::DataBuffer<wchar_t> wideRecvBuffer_;
	};

// Writer
/////////////
	class Writer : public utils::IRunnable, private utils::LockableObject
	{
	public:
		Writer(HANDLE port);
		virtual ~Writer() 
		{}

	public:
		void push_back(const tstring&);

		void Run(HANDLE stopEvent);
		bool OnUnhandledException(const std::exception& ex) throw();

	private:
		utils::Event newStringEvent_;
		std::queue<tstring> sendQueue_;
		HANDLE port_;
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

		void Send(const tstring& data);
		void Disconnect() 
		{}
	};
};

typedef std::tr1::shared_ptr<SerialPort> SerialPortPtr;

}	//namespace IO
}	//namespace caspar