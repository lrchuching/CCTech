
#pragma once
#include <map>
#include "GeneralSock.h"
#include "NetSpinLock.h"

class CSocketServer : public CGeneralSock
{
public:
	CSocketServer(const string& SIp, unsigned short NPort);
	~CSocketServer();

	bool Listen();
	void StopListen();

	bool AsyncWait();
	void Wait();
	void HangupRecv(SOCKET sock);
	void HangupSend(SOCKET sock);

	inline bool IsListen() {
		return bListen;
	}

	bool PopMsg(SOCKET& OutSock, GsBuffer& OutMsg);

private:
	bool bindIPandPort(SOCKET &listenScok, const string ip, const unsigned short port);
	bool listenSocket(SOCKET &listenScok);

	void UpdateClients(SOCKET sock, bool bRemove = false);

private:
	CNetSpinLock	mSvrStateLock;
	bool			bListen;

	CNetSpinLock	mClientsLock;
	vector<SOCKET>	mClientSockets;

	CNetSpinLock					mRecvLock;
	map<SOCKET, vector<GsBuffer>>	mRecvMsgs;
	CNetSpinLock					mBroadcastLock;
	map<SOCKET, vector<GsBuffer>>	mBroadcastMsgs;
};


void _ServerSendProc(CSocketServer* pServer, SOCKET sock);
void _ServerReceiveProc(CSocketServer* pServer, SOCKET sock);
void _ServerConnectProc(CSocketServer* pServer);


