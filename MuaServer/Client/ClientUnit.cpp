#include "pch.h"
#include "ClientUnit.h"
#include "ClientManager.h"
#include "../MuaServer.h"
#include "../MuaServerDlg.h"


CClientUnit::CClientUnit(CONNID dwConnID)
{
	// 获取对端IP和端口
	m_dwMainSocketConnID = dwConnID;
	int iAddressLen = 32;
	((CMuaServerDlg*)(theApp.m_pMainWnd))->m_pServerSocket->m_pTcpPackServer->m_pTcpPackServer->GetRemoteAddress(dwConnID, m_wszIpAddress, iAddressLen, m_wPort);

	// 生成TOKEN
	AutoSeededRandomPool  rng;
	rng.GenerateBlock((PBYTE)&m_qwClientToken, sizeof(QWORD));

	// 如果TOKEN冲突了就重新生成
	while (((CMuaServerDlg*)(theApp.m_pMainWnd))->m_pClientManager->IsTokenConflict(m_qwClientToken)) {
		rng.GenerateBlock((PBYTE)&m_qwClientToken, sizeof(QWORD));
	}

	InitializeCriticalSection(&m_cs);				// 初始化锁，用于DeleteClientUnit
}


VOID CClientUnit::AddClientSocket(CONNID dwConnID, CClientSocket* pClientSocket)
{
	m_mapClientSocketUseConnID.insert({ dwConnID, pClientSocket });
	m_mapClientSocketUseTag.insert({ pClientSocket->m_dwSocketTag, pClientSocket });
}


CClientSocket* CClientUnit::GetClientSocketByConnId(CONNID dwConnID)
{
	if (m_mapClientSocketUseConnID.find(dwConnID) == m_mapClientSocketUseConnID.end()) {
		return nullptr;
	}
	else {
		return m_mapClientSocketUseConnID[dwConnID];
	}
}


CClientSocket* CClientUnit::GetClientSocketByTag(DWORD dwSocketTag)
{
	if (m_mapClientSocketUseTag.find(dwSocketTag) == m_mapClientSocketUseTag.end()) {
		return nullptr;
	}
	else {
		return m_mapClientSocketUseTag[dwSocketTag];
	}
}


BOOL CClientUnit::IsSocketTagConflict(DWORD dwSocketTag)
{
	BOOL bRet = m_mapClientSocketUseTag.find(dwSocketTag) != m_mapClientSocketUseTag.end();
	return bRet;
}