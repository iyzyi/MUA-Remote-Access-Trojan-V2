#pragma once


// 注意，若采用TCP_PUSH模式，则密码算法只能采用序列密码
enum TCP_MODE { TCP_PACK, TCP_PUSH };


// 通用Socket类，主通信类、远程SHELL通信类等都派生自此类，子类必须实现OnReceiveWithDec()
class CSocket : public CMyTcpPackClient, public CMyTcpPushClient {

protected:
	TCP_MODE	m_iTcpMode		= TCP_PACK;

	WCHAR		m_wszAddress[32];
	WORD		m_wPort			= 0;

	CEncrypt*	m_pEnc			= nullptr;
	CDecrypt*	m_pDec			= nullptr;
	BYTE		m_pbKey[MAX_KEY_LEN];
	BYTE		m_pbIv[MAX_IV_LEN];

	BOOL		m_bIsMainClientSocket = FALSE;
	BOOL		m_bRecvFirstPacketReturn = FALSE;

	QWORD		m_qwClientToken = 0;
	nsGeneralSocket::SERVICE_TYPE	m_wServiceType = nsGeneralSocket::NULL_SERVICE;
	WORD		m_wSocketType = 0;
	DWORD		m_dwParentSocketTag = 0;


public:

	CMyTcpPackClient* m_pTcpPackClient;
	CMyTcpPushClient* m_pTcpPushClient;


protected:

	EnHandleResult OnReceive(ITcpClient* pSender, CONNID dwConnID, const BYTE* pData, int iLength);
	virtual VOID OnReceiveWithDec(ITcpClient* pSender, CONNID dwConnID, QWORD qwClientToken, WORD wServiceType, WORD wSocketType, int iCommandID, DWORD dwParentSocketTag, MyBuffer mBuffer) = 0;

	virtual VOID SendLoginPacket();

	BOOL CreateSendKeyIv();


public:

	CSocket(TCP_MODE iTcpMode, nsGeneralSocket::SERVICE_TYPE wServiceType, WORD wSocketType, QWORD qwClientToken = 0, DWORD dwParentSocketTag = 0);
	~CSocket();

	VOID InitSocket(LPWSTR pwszAddress, WORD wPort);
	BOOL StartSocket();
	BOOL StopSocket();

	VOID SetCryptoAlg(CRYPTO_ALG iCryptoAlg);

	// 明文发包，用于一开始传递key和iv。一旦调用过SendWithEnc后千万不能再调用Send函数，会导致对端解密出问题
	BOOL Send(PBYTE pBuffer, DWORD dwLength);
	// 加密后发包
	BOOL SendWithEnc(int iCommandID, PBYTE pBuffer = nullptr, DWORD dwLength = 0);

	BOOL IsConnected();

	VOID DebugPrintRecvParams(CONNID dwConnID, QWORD qwClientToken, WORD wServiceType, WORD wSocketType, int iCommandID, DWORD dwParentSocketTag, MyBuffer mBuffer);

	VOID SetParentSocketTag(DWORD dwParentSocketTag);
};