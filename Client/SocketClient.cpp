
#include "SocketClient.h"

CSocketClient::CSocketClient(const string strIp, unsigned short nPort)
{
	bConnected = false;
	bQuitClient = false;
	mIp = strIp;
	mPort = nPort;
}

CSocketClient::~CSocketClient()
{
	WSACleanup();
}

void CSocketClient::PushSendMsg(const string& strSend)
{
	if (strSend.size() > 0)
	{
		mSendMsgLock.EnforceLock();
		mSendMsgs.push_back(strSend);
		mSendMsgLock.Unlock();
	}
}

bool CSocketClient::PopRecvMsg(string& strRecv)
{
	if (mRevMsgs.size() > 0){
		mRevMsgLock.EnforceLock();
		string& msg = mRevMsgs.at(0);
		strRecv.resize(msg.size());
		memcpy((void*)(strRecv.c_str()), msg.c_str(), msg.size());
		mRevMsgs.erase(mRevMsgs.begin());
		mRevMsgLock.Unlock();
		return true;
	}
	return false;
}

bool CSocketClient::StartConnectGuard()
{
	if (ProvideConnect() == false){
		return false;
	}
	bQuitClient = false;
	thread tObj(_ClientConnectProc, this);
	tObj.detach();
	return true;
}

void CSocketClient::StopConnect()
{
	mConnectStateLock.EnforceLock();
	bConnected = false;
	mConnectStateLock.Unlock();
	bQuitClient = true;
}

void CSocketClient::HangConnect()
{
	while (true)
	{
		if (bConnected == false)
		{
			if (ConnectServer(mClientSock, mIp, mPort))
			{
				mConnectStateLock.EnforceLock();
				bConnected = true;
				mConnectStateLock.Unlock();

				thread sendObj(_ClientSendProc, this);
				sendObj.detach();

				thread revObj(_ClientRecieveProc, this);
				revObj.detach();
			}
		}
		if (bQuitClient == true){
			break;
		}
		Sleep(1);
	}
}

void CSocketClient::HangSend()
{
	while (true)
	{
		string sendData;
		if (mSendMsgs.size() > 0)
		{
			mSendMsgLock.EnforceLock();
			string& msg = mSendMsgs.at(0);
			sendData.resize(msg.size());
			memcpy((void*)(sendData.c_str()), msg.c_str(), msg.size());
			mSendMsgs.erase(mSendMsgs.begin());
			mSendMsgLock.Unlock();
		}
		int se = send(mClientSock, sendData.c_str(), sendData.size(), 0);
		if (se == SOCKET_ERROR){
			mConnectStateLock.EnforceLock();
			bConnected = false;
			mConnectStateLock.Unlock();
			break;
		}
		if (bQuitClient == true){
			break;
		}
	}
}

void CSocketClient::HangReceive()
{
	string strRecv;
	while (true)
	{
		char recvBuf[1024] = "\0";
		int recvLen = recv(mClientSock, recvBuf, 1024, 0);
		if (recvLen == SOCKET_ERROR){
			mConnectStateLock.EnforceLock();
			bConnected = false;
			mConnectStateLock.Unlock();
			break;
		}
		strRecv.resize(recvLen);
		memcpy((void*)(strRecv.c_str()), recvBuf, recvLen);
		{			
			mRevMsgLock.EnforceLock();
			mRevMsgs.push_back(strRecv);
			mRevMsgLock.Unlock();
		}
		if (bQuitClient == true){
			break;
		}
	}
}

bool CSocketClient::InitWinSock()
{
	WORD verision = MAKEWORD(2, 2);
	WSADATA lpData;
	int intEr = WSAStartup(verision, &lpData);		// 指定winsock版本并初始化
	if (intEr != 0)
	{
		printf("InitWinSock: 初始化失败! \n");
		return false;
	}
	printf("InitWinSock: 初始化成功! \n");
	return true;
}

bool CSocketClient::CreateWinSock(SOCKET &listenScok)
{
	listenScok = socket(AF_INET, SOCK_STREAM, 0);	// 创建侦听socket  
	if (listenScok == INVALID_SOCKET)
	{
		printf("CreateWinSock: socket 创建失败! \n");
		return false;
	}
	printf("CreateWinSock: socket 创建成功! \n");
	return true;
}

bool CSocketClient::ProvideConnect()
{
	if (false == InitWinSock()){
		return false;
	}
	if (false == CreateWinSock(mClientSock)){
		return false;
	}
	return true;
}

bool CSocketClient::ConnectServer(SOCKET &conSock, const string ip, const unsigned short port)
{
	sockaddr_in hostAddr;				// 建立地址结构体
	hostAddr.sin_family = AF_INET;		// Tcp, Udp ...
	hostAddr.sin_port = htons(port);	// 转换成网络字节序  
	
	in_addr addr;
	inet_pton(AF_INET, ip.c_str(), (void*)&addr);
	hostAddr.sin_addr = addr;

	printf("ConnectServer: 尝试连接");
	printf(ip.c_str());
	printf(":");
	printf("%d\n", port);

	int err = connect(conSock, (sockaddr*)&hostAddr, sizeof(sockaddr));		// 向服务器提出连接请求
	if (err == INVALID_SOCKET)
	{
		DWORD dwError = GetLastError();
		printf("ConnectServer: ");
		printf("%d\n", dwError);

		shutdown(conSock, 2);
		CreateWinSock(conSock);
		return false;
	}
	return true;
}

void _ClientConnectProc(CSocketClient* pClient)
{
	if (pClient != NULL){
		pClient->HangConnect();
	}
}

void _ClientSendProc(CSocketClient* pClient)
{
	if (pClient != NULL)
	{
		pClient->HangSend();
	}
}

void _ClientRecieveProc(CSocketClient* pClient)
{
	if (pClient != NULL)
	{
		pClient->HangReceive();
	}
}
