
#pragma  once
#include "GeneralSock.h"

#include "NetSpinLock.h"

class CSocketClient : public CGeneralSock
{
public:
	CSocketClient(const string strIp, unsigned short nPort);
	~CSocketClient();

	bool StartConnectGuard();
	void StopConnect();

	void PushSendMsg(const GsBuffer& strSend);
	bool PopRecvMsg(GsBuffer& strRecv);

	void HangConnect();		// connect (hang up)
	void HangSend();		// send (hang up)
	void HangReceive();		// receive (hang up)

private:

	bool ProvideConnect();
	bool ConnectServer(SOCKET &conSock, const string ip, const unsigned short port);

private:

	bool bQuitClient;

	CNetSpinLock mConnectStateLock;
	bool bConnected;

	CNetSpinLock mSendMsgLock;
	vector<GsBuffer> mSendMsgs;

	CNetSpinLock mRevMsgLock;
	vector<GsBuffer> mRevMsgs;
	
};

void _ClientSendProc(CSocketClient* pClient);
void _ClientRecieveProc(CSocketClient* pClient);
void _ClientConnectProc(CSocketClient* pClient);





