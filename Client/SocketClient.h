
#pragma  once
#include <xstring>
#include <vector>
#include <WinSock2.h>  
#include <WS2tcpip.h>  
#include <thread>

#include "Cach.h"
#include "NetSpinLock.h"

#pragma comment(lib,"ws2_32.lib")
using namespace std;

class CSocketClient
{
public:
	CSocketClient(const string strIp, unsigned short nPort);
	~CSocketClient();

	bool StartConnectGuard();
	void StopConnect();

	void PushSendMsg(const string& strSend);
	bool PopRecvMsg(string& strRecv);

	void HangConnect();		// connect (hang up)
	void HangSend();		// send (hang up)
	void HangReceive();		// receive (hang up)

private:

	bool InitWinSock();
	bool CreateWinSock(SOCKET &listenScok);
	bool ProvideConnect();
	bool ConnectServer(SOCKET &conSock, const string ip, const unsigned short port);

private:

	string mIp;
	unsigned short mPort;
	bool bQuitClient;
	SOCKET mClientSock;

	CNetSpinLock mConnectStateLock;
	bool bConnected;

	CNetSpinLock mSendMsgLock;
	vector<string> mSendMsgs;

	CNetSpinLock mRevMsgLock;
	vector<string> mRevMsgs;
	
};

void _ClientSendProc(CSocketClient* pClient);
void _ClientRecieveProc(CSocketClient* pClient);
void _ClientConnectProc(CSocketClient* pClient);





