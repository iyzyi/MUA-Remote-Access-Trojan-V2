#include "pch.h"
#include "ClientSocket.h"
#include "../MuaServerDlg.h"
#include "../MuaServer.h"


CClientSocket::CClientSocket(QWORD qwClientToken, WORD wServiceType, WORD wSocketType, CONNID dwConnID, DWORD dwParentSocketTag){
	m_qwClientToken = qwClientToken;
	m_dwConnID = dwConnID;
	m_wServiceType = wServiceType;
	m_wSocketType = wSocketType;
	m_dwParentSocketTag = dwParentSocketTag;

	// 获取对端IP和端口
	int iAddressLen = 32;
	((CMuaServerDlg*)(theApp.m_pMainWnd))->m_pServerSocket->m_pTcpPackServer->m_pTcpPackServer->GetRemoteAddress(dwConnID, m_wszIpAddress, iAddressLen, m_wPort);

	// 生成Tag
	AutoSeededRandomPool  rng;
	rng.GenerateBlock((PBYTE)&m_dwSocketTag, sizeof(DWORD));

	// 如果Socket的Tag冲突了就重新生成
	CClientUnit* pClientUnit = ((CMuaServerDlg*)(theApp.m_pMainWnd))->m_pClientManager->GetClientUnitByToken(qwClientToken);
	if (pClientUnit == nullptr) {
		rng.GenerateBlock((PBYTE)&m_dwSocketTag, sizeof(DWORD));
	}
	else {
		while (pClientUnit->IsSocketTagConflict(m_dwSocketTag)) {
			rng.GenerateBlock((PBYTE)&m_dwSocketTag, sizeof(DWORD));
		}
	}
}


CClientSocket::~CClientSocket(){
	if (m_pbKey != nullptr) {
		delete m_pbKey;
		m_pbKey = nullptr;
	}
	if (m_pbIv != nullptr) {
		delete m_pbIv;
		m_pbIv = nullptr;
	}
	if (m_pEnc != nullptr) {
		delete m_pEnc;
		m_pEnc = nullptr;
	}
	if (m_pDec != nullptr) {
		delete m_pDec;
		m_pDec = nullptr;
	}
}


VOID CClientSocket::SetCrypto(CRYPTO_ALG iCryptoAlg, PBYTE pbKey, PBYTE pbIv) {
	m_pbKey = new BYTE[MAX_KEY_LEN];
	m_pbIv = new BYTE[MAX_IV_LEN];
	memcpy(m_pbKey, pbKey, MAX_KEY_LEN);
	memcpy(m_pbIv, pbIv, MAX_IV_LEN);

	m_pEnc = new CEncrypt();
	m_pEnc->Init(iCryptoAlg, m_pbKey, m_pbIv);

	m_pDec = new CDecrypt();
	m_pDec->Init(iCryptoAlg, m_pbKey, m_pbIv);
}


// 如果服务有对话框，则应调用SetDialog来将对话框对象与ClientSocket关联起来；如果服务没有对话框，则应调用SetService来将服务对象与ClientSocket关联起来。
VOID CClientSocket::SetServiceDialog(CDialogEx* pServiceDialog) {
	m_pServiceDialog = pServiceDialog;
}

// 如果服务有对话框，则应调用SetDialog来将对话框对象与ClientSocket关联起来；如果服务没有对话框，则应调用SetService来将服务对象与ClientSocket关联起来。
VOID CClientSocket::SetServiceObject(LPVOID pServiceObject) {
	m_pServiceObject = pServiceObject;
}