// AsyncEventServer.h: interface for the AsyncServer class.
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ASYNCEVENTSERVER_H__0BFA29CB_BE4C_46A0_9CAE_E233ED27A8EC__INCLUDED_)
#define AFX_ASYNCEVENTSERVER_H__0BFA29CB_BE4C_46A0_9CAE_E233ED27A8EC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <string>
#include <map>
#include <vector>

#include "..\utils\thread.h"
#include "..\utils\lockable.h"

#include "ProtocolStrategy.h"
#include "..\controller.h"
#include "SocketInfo.h"

namespace caspar {
namespace IO {

class AsyncEventServer : public utils::IRunnable, public caspar::IController
{
	static long instanceCount_;

	AsyncEventServer();
	AsyncEventServer(const AsyncEventServer&);
	AsyncEventServer& operator=(const AsyncEventServer&);

public:
	explicit AsyncEventServer(int port);
	~AsyncEventServer();

	bool Start();
	void SetProtocolStrategy(ProtocolStrategyPtr pPS) {
		pProtocolStrategy_ = pPS;
	}

	void Stop();

	
private:
	utils::Thread	listenThread_;
	void Run(HANDLE stopEvent);
	bool OnUnhandledException(const std::exception&) throw();

	bool OnAccept(SocketInfoPtr&);
	bool OnRead(SocketInfoPtr&);
	void OnWrite(SocketInfoPtr&);
	void OnClose(SocketInfoPtr&);
	void OnError(HANDLE waitEvent, int errorCode);

	SocketInfoPtr		pListenSocketInfo_;
	ProtocolStrategyPtr	pProtocolStrategy_;
	int					port_;

	friend class SocketInfo;
	void DoSend(SocketInfo&);
	void DisconnectClient(SocketInfo&);

	void LogSocketError(const TCHAR* pStr, int socketError = 0);

	class SocketInfoCollection : private utils::LockableObject
	{
		SocketInfoCollection(const SocketInfoCollection&);
		SocketInfoCollection& operator=(const SocketInfoCollection&);

		typedef std::map<HANDLE, SocketInfoPtr> SocketInfoMap;
		typedef std::vector<HANDLE> HandleVector;

	public:
		SocketInfoCollection();
		~SocketInfoCollection();

		bool AddSocketInfo(SocketInfoPtr& pSocketInfo);
		void RemoveSocketInfo(SocketInfoPtr& pSocketInfo);
		void RemoveSocketInfo(HANDLE);
		bool FindSocketInfo(HANDLE, SocketInfoPtr& pResult);
		void CopyCollectionToArray(HANDLE*, int maxCount);

		bool IsDirty() {
			return bDirty_;
		}
		void ClearDirty() {
			bDirty_ = false;
		}

		std::size_t Size() {
			return waitEvents_.size();
		}
		void Clear();

	private:
		SocketInfoMap socketInfoMap_;
		HandleVector waitEvents_;
		bool bDirty_;
	};
	SocketInfoCollection socketInfoCollection_;
};
typedef std::tr1::shared_ptr<AsyncEventServer> AsyncEventServerPtr;

}	//namespace IO
}	//namespace caspar

#endif // !defined(AFX_ASYNCEVENTSERVER_H__0BFA29CB_BE4C_46A0_9CAE_E233ED27A8EC__INCLUDED_)
