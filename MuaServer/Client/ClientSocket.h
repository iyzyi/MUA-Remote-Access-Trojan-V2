#pragma once


// CClientSocket表示一个来自Mua客户端的Socket连接，包括Client的MainSocket和各服务的Socket.
class CClientSocket{

private:
	PBYTE		m_pbKey			= nullptr;
	PBYTE		m_pbIv			= nullptr;

public:
	CONNID		m_dwConnID = 0;

	WCHAR		m_wszIpAddress[32] = { 0 };
	WORD		m_wPort = 0;

	// 本Socket用于什么服务（如主通信，远程SHELL，文件传输等）
	WORD		m_wServiceType = nsGeneralSocket::NULL_SERVICE;

	WORD		m_wSocketType = 0;

	// 本Socket所属的客户端的TOKEN.
	QWORD		m_qwClientToken = 0;

	// 在一个ClientUnit内部，每个ClientSocket都有唯一的Socket Tag，用于标识ClientUnit内部的不同Socket。
	DWORD		m_dwSocketTag = 0;

	// 父Socket的Tag。比如文件传输的用于列出文件列表的socket的父socket的Tag
	DWORD		m_dwParentSocketTag = 0;

	CEncrypt*	m_pEnc = nullptr;
	CDecrypt*	m_pDec = nullptr;

	CDialogEx* m_pServiceDialog = nullptr;
	LPVOID	   m_pServiceObject = nullptr;

public:
	CClientSocket(QWORD qwClientToken, WORD wServiceType, WORD wSocketType, CONNID dwConnID, DWORD dwParentSocketTag);
	~CClientSocket();

	VOID SetCrypto(CRYPTO_ALG iCryptoAlg, PBYTE pbKey, PBYTE pbIv);
	
	// 如果服务有对话框，则应调用SetServiceDialog来将对话框对象与ClientSocket关联起来；如果服务没有对话框，则应调用SetServiceObject来将服务对象与ClientSocket关联起来。
	VOID SetServiceDialog(CDialogEx* pServiceDialog);

	// 如果服务有对话框，则应调用SetServiceDialog来将对话框对象与ClientSocket关联起来；如果服务没有对话框，则应调用SetServiceObject来将服务对象与ClientSocket关联起来。
	VOID SetServiceObject(LPVOID pServiceObject);
};