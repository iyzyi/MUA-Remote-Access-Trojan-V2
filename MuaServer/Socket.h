#pragma once

#include "../MyHpSocket/MyHpSocket.h"
#include "../MyCrypto/MyCrypto.h"


// 注意，若采用TCP_PUSH模式，则密码算法只能采用序列密码
enum TCP_MODE { TCP_PACK, TCP_PUSH };


// 通用Socket类，主通信类、远程SHELL通信类等都派生自此类，子类必须实现OnReceiveWithDec()
class CSocket : public CMyTcpPackServer, public CMyTcpPushServer {

protected:
	TCP_MODE	m_iTcpMode		= TCP_PACK;

	WCHAR		m_wszAddress[32];
	WORD		m_wPort			= 0;


public:

	CMyTcpPackServer* m_pTcpPackServer;
	CMyTcpPushServer* m_pTcpPushServer;


private:
	BOOL IsNewClientSocket(CONNID dwConnID, const BYTE* pData, int iLength);
	BOOL InitNewClientSocket(CONNID dwConnID, const BYTE* pData, int iLength);

protected:
	EnHandleResult OnReceive(ITcpServer* pSender, CONNID dwConnID, const BYTE* pData, int iLength);
	virtual VOID OnReceiveWithDec(ITcpServer* pSender, CONNID dwConnID, QWORD qwClientToken, WORD wServiceType, WORD wSocketType, int iCommandID, DWORD dwParentSocketTag, MyBuffer mBuffer) = 0;

	EnHandleResult OnClose(ITcpServer* pSender, CONNID dwConnID, EnSocketOperation enOperation, int iErrorCode);

public:
	CSocket(TCP_MODE iTcpMode);
	~CSocket();

	VOID InitSocket(LPWSTR pwszAddress, WORD wPort);
	BOOL StartSocket();
	BOOL StopSocket();
	BOOL Disconnect(CONNID dwConnID);

	// 明文发包，用于一开始传递key和iv。一旦调用过SendWithEnc后千万不能再调用Send函数，会导致对端解密出问题
	BOOL Send(CONNID dwConnID, PBYTE pBuffer, DWORD dwLength);
	// 加密后发包
	BOOL SendWithEnc(CONNID dwConnID, int iCommandID, PBYTE pBuffer = nullptr, DWORD dwLength = 0);

	BOOL SendCreateChildSocket(CONNID dwConnID, int iCommandID, DWORD dwParentSocketTag);

	VOID CSocket::DebugPrintRecvParams(CONNID dwConnID, QWORD qwClientToken, WORD wServiceType, WORD wSocketType, int iCommandID, DWORD dwParentSocketTag, MyBuffer mBuffer);
};