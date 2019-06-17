
#pragma  once
#include <xstring>
#include <vector>
#include <WinSock2.h>  
#include <WS2tcpip.h>  
#include <thread>

#pragma comment(lib,"ws2_32.lib")
using namespace std;

typedef vector<char> GsBuffer;
typedef struct __ClientGsBuffer{
	SOCKET				mSock;
	vector<GsBuffer>	mMsgs;
}CliBuffer;

class CGeneralSock
{
public:
	CGeneralSock(const string strIp, unsigned short nPort){
		mIp = strIp;
		mPort = nPort;
		mSock = INVALID_SOCKET;
		InitWinSock();
	}

	~CGeneralSock(){
		::WSACleanup();
	}

protected:

	bool InitWinSock(){
		WORD verision = MAKEWORD(2, 2);
		WSADATA lpData;
		int intEr = ::WSAStartup(verision, &lpData);		// 指定winsock版本并初始化
		if (intEr != 0){
			int le = ::WSAGetLastError();
			return false;
		}
		return true;
	}

	bool CreateWinSock(SOCKET &listenScok){
		listenScok = socket(AF_INET, SOCK_STREAM, 0);	// 创建侦听socket  
		if (listenScok == INVALID_SOCKET){
			int le = ::WSAGetLastError();
			return false;
		}
		return true;
	}

protected:

	string mIp;
	unsigned short mPort;
	SOCKET mSock;

};
