
#include "SocketServer.h"

CSocketServer::CSocketServer(const string& SIp, unsigned short NPort) : 
	CGeneralSock(SIp, NPort)
{
	mClientSockets.clear();
	mRecvMsgs.clear();
	mBroadcastMsgs.clear();
	bListen = false;
}

CSocketServer::~CSocketServer()
{
	::WSACleanup();
}

bool CSocketServer::Listen()
{
	if (false == InitWinSock()){
		return false;
	}
	if (false == CreateWinSock(mSock)){
		return false;
	}
	if (false == bindIPandPort(mSock, mIp, mPort)){
		return false;
	}
	if (false == listenSocket(mSock)){
		return false;
	}
	bListen = true;

	return true;
}

void CSocketServer::StopListen()
{
	bListen = false;
}

bool CSocketServer::AsyncWait()
{
	thread tObj(_ServerConnectProc, this);
	tObj.detach();
	return true;
}

bool CSocketServer::bindIPandPort(SOCKET &listenScok, const string ip, const unsigned short port)
{
	sockaddr_in hostAddr;
	hostAddr.sin_family = AF_INET;
	hostAddr.sin_port = ::htons(port);
	in_addr addr;
	::inet_pton(AF_INET, ip.c_str(), (void*)&addr);
	hostAddr.sin_addr = addr;
	int err = ::bind(listenScok, (struct sockaddr*)&hostAddr, sizeof(sockaddr));
	if (err != 0)
	{
		return false;
	}
	return true;
}

bool CSocketServer::listenSocket(SOCKET &listenScok)
{
	int err = ::listen(listenScok, 3);
	if (err != 0)
	{
		return false;
	}
	return true;
}

void CSocketServer::UpdateClients(SOCKET sock, bool bRemove)
{
	if (sock == INVALID_SOCKET){
		return;
	}

	mRecvLock.EnforceLock();
	std::map<SOCKET, vector<GsBuffer>>::iterator recvItr = mRecvMsgs.find(sock);
	if (recvItr == mRecvMsgs.end()){
		if (bRemove == false){
			mRecvMsgs.insert(std::pair<SOCKET, vector<GsBuffer>>(sock, vector<GsBuffer>()));
		}
	}
	else{
		if (bRemove == true){
			mRecvMsgs.erase(recvItr);
		}
	}
	mRecvLock.Unlock();

	mBroadcastLock.EnforceLock();
	std::map<SOCKET, vector<GsBuffer>>::iterator brdItr = mBroadcastMsgs.find(sock);
	if (brdItr == mBroadcastMsgs.end()){
		if (bRemove == false){
			mBroadcastMsgs.insert(std::pair<SOCKET, vector<GsBuffer>>(sock, vector<GsBuffer>()));
		}
	}
	else{
		if (bRemove == true){
			mBroadcastMsgs.erase(brdItr);
		}
	}
	mBroadcastLock.Unlock();
}

void CSocketServer::Wait()
{
	SOCKET clientSock;
	while (true && IsListen())
	{
		sockaddr_in clientAddr;
		int len = sizeof(struct sockaddr);
		clientSock = ::accept(mSock, (struct sockaddr*)&clientAddr, &len);
		if (clientSock == INVALID_SOCKET)
		{
			int er = ::WSAGetLastError();
			continue;
		}
		else
		{
			UpdateClients(clientSock, false);

			std::thread recH(_ServerReceiveProc, this, clientSock);
			recH.detach();

			std::thread sedH(_ServerSendProc, this, clientSock);
			sedH.detach();
		}
		Sleep(0);
	}
}

void CSocketServer::HangupRecv(SOCKET sock)
{
	if (sock == INVALID_SOCKET){
		return;
	}
	while (true && IsListen())
	{
		GsBuffer data;
		char buf[1024] = "\0";
		int buflen = ::recv(sock, buf, 1024, 0);
		if (buflen == SOCKET_ERROR)
		{
			int eCode = ::shutdown(sock, 2);
			if (eCode == SOCKET_ERROR)
			{
				break;
			}
			break;
		}
		data.resize(buflen);
		memcpy((void*)(data.data()), buf, buflen);
		
		mRecvLock.EnforceLock();
		std::map<SOCKET, vector<GsBuffer>>::iterator msgItr = mRecvMsgs.find(sock);
		if (msgItr != mRecvMsgs.end())
		{
			mRecvMsgs.at(sock).push_back(data);
		}
		mRecvLock.Unlock();

		Sleep(0);
	}
}

void CSocketServer::HangupSend(SOCKET sock)
{
	GsBuffer data;
	while (true && IsListen())
	{
		data.clear();

		mBroadcastLock.EnforceLock();
		std::map<SOCKET, vector<GsBuffer>>::iterator msgItr = mBroadcastMsgs.find(sock);
		if (msgItr != mBroadcastMsgs.end())
		{
			vector<GsBuffer>& msgs = mBroadcastMsgs.at(sock);
			if (msgs.size() > 0)
			{
				data = msgs.at(0);
			}
		}
		mBroadcastLock.Unlock();
		
		if (data.size() > 0){
			int err = send(sock, data.data(), data.size(), 0);
		}

		Sleep(0);
	}
}

bool CSocketServer::PopMsg(SOCKET& OutSock, GsBuffer& OutMsg)
{
	bool bRes = false;
	mRecvLock.EnforceLock();
	auto msgItr = mRecvMsgs.begin();
	for (; msgItr != mRecvMsgs.end(); msgItr++)
	{
		if (msgItr->second.size() > 0){
			OutSock = msgItr->first;
			OutMsg = msgItr->second.at(0);
			msgItr->second.erase(msgItr->second.begin());
			bRes = true;
			break;
		}
	}
	mRecvLock.Unlock();
	return bRes;
}


void _ServerSendProc(CSocketServer* pServer, SOCKET sock)
{
	if (pServer)
	{
		pServer->HangupSend(sock);
	}
}

void _ServerReceiveProc(CSocketServer* pServer, SOCKET sock)
{
	if (pServer)
	{
		pServer->HangupRecv(sock);
	}
}

void _ServerConnectProc(CSocketServer* pServer)
{
	if (pServer)
	{
		pServer->Wait();
	}
}

