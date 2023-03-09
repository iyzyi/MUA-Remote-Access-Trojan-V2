#include "pch.h"
#include "ClientManager.h"
#include "../MuaServer.h"
#include "../MuaServerDlg.h"


CClientManager::CClientManager(){
	InitializeCriticalSection(&m_cs);		// 初始化锁
}


CClientManager::~CClientManager() {
	//EnterCriticalSection(&m_cs);			// 加锁

	for (auto iter = m_mapClientUnit.begin(); iter != m_mapClientUnit.end();) {
		QWORD qwClientToken = iter->first;
		CClientUnit* pClientUnit = iter->second;
		iter++;										// erase当前iter后，就没法iter++了。所以要在删除前实现迭代器的迭代。
		DeleteClientUnit(pClientUnit);
	}

	//LeaveCriticalSection(&m_cs);			// 解锁
}


BOOL CClientManager::IsTokenConflict(QWORD qwClientToken)
{
	//EnterCriticalSection(&m_cs);			// 加锁

	BOOL bRet = m_mapClientUnit.find(qwClientToken) != m_mapClientUnit.end();
	
	//LeaveCriticalSection(&m_cs);			// 解锁

	return bRet;
}


VOID CClientManager::AddClientUtil(QWORD qwClientToken, CClientUnit* pClientUnit)
{
	//EnterCriticalSection(&m_cs);			// 加锁

	m_mapClientUnit.insert({ qwClientToken, pClientUnit });
	DebugPrint("m_mapClientSocket.size = %d\n", m_mapClientSocket.size());
	DebugPrint("m_mapClientUnit.size = %d\n", m_mapClientUnit.size());

	//LeaveCriticalSection(&m_cs);			// 解锁
}


VOID CClientManager::AddClientSocket(CONNID dwConnID, CClientSocket* pClientSocket)
{
	//EnterCriticalSection(&m_cs);			// 加锁
	
	m_mapClientSocket.insert({ dwConnID, pClientSocket });
	DebugPrint("m_mapClientSocket.size = %d\n", m_mapClientSocket.size());
	DebugPrint("m_mapClientUnit.size = %d\n", m_mapClientUnit.size());

	//LeaveCriticalSection(&m_cs);			// 解锁
}


CClientUnit* CClientManager::GetClientUnitByConnId(CONNID dwConnID)
{
//	EnterCriticalSection(&m_cs);			// 加锁

	CClientUnit* pRet = nullptr;
	CClientSocket* pClientSocket = GetClientSocketByConnId(dwConnID);
	if (pClientSocket != nullptr){
		QWORD qwClientToken = pClientSocket->m_qwClientToken;
		if (m_mapClientUnit.find(qwClientToken) == m_mapClientUnit.end()) {
			pRet = nullptr;
		}
		else {
			pRet= m_mapClientUnit[qwClientToken];
		}
	}
	else {
		pRet = nullptr;
	}

	//LeaveCriticalSection(&m_cs);			// 解锁
	return pRet;
}


CClientUnit* CClientManager::GetClientUnitByToken(QWORD qwClientToken) {
	//EnterCriticalSection(&m_cs);			// 加锁

	CClientUnit* pClientUnit = nullptr;
	if (m_mapClientUnit.find(qwClientToken) == m_mapClientUnit.end()) {
		pClientUnit = nullptr;				// EnterCriticalSection和LeaveCriticalSection嵌套的次数必须一致，提前return会导致未知的错误
	}
	else {
		pClientUnit = m_mapClientUnit[qwClientToken];
	}

	//LeaveCriticalSection(&m_cs);			// 解锁
	return pClientUnit;
}


CClientSocket* CClientManager::GetClientSocketByConnId(CONNID dwConnID)
{
	//EnterCriticalSection(&m_cs);			// 加锁

	CClientSocket* pClientSocket = nullptr;
	if (m_mapClientSocket.find(dwConnID) == m_mapClientSocket.end()) {
		pClientSocket = nullptr;
	}
	else {
		pClientSocket = m_mapClientSocket[dwConnID];
	}

	//LeaveCriticalSection(&m_cs);			// 解锁
	return pClientSocket;
}


CServerSocket* CClientManager::GetServerSocket() {
	return ((CMuaServerDlg*)(theApp.m_pMainWnd))->m_pServerSocket;
}


VOID CClientManager::DeleteClientSocket(CClientSocket* pClientSocket) {
	// 加锁
	//EnterCriticalSection(&m_cs);

	CClientUnit* pClientUnit = GetClientUnitByToken(pClientSocket->m_qwClientToken);

	if (pClientUnit != nullptr) {
		// 从ClientUnit的map中删掉此ClientSocket
		pClientUnit->m_mapClientSocketUseConnID.erase(pClientSocket->m_dwConnID);
		pClientUnit->m_mapClientSocketUseTag.erase(pClientSocket->m_dwSocketTag);

		// 从ClientManager的map中删掉此ClientSocket
		this->m_mapClientSocket.erase(pClientSocket->m_dwConnID);

		// 断开socket连接（这一步要在“从ClientUnit的map中删掉此ClientSocket”后面，这样OnClose的时候，GetClientSocketByConnID返回值会是nullptr）
		BOOL bRet = GetServerSocket()->Disconnect(pClientSocket->m_dwConnID);

		DebugPrint("m_mapClientSocket.size = %d\n", m_mapClientSocket.size());
		DebugPrint("m_mapClientUnit.size = %d\n", m_mapClientUnit.size());

		delete pClientSocket;
	}

	// 解锁
	//LeaveCriticalSection(&m_cs);					
}


VOID CClientManager::DeleteClientUnit(CClientUnit* pClientUnit) {
	// 加锁	
	//EnterCriticalSection(&m_cs);			

	// 断开此ClientUnit所有的ClientSocket的socket连接
	for (auto iter = pClientUnit->m_mapClientSocketUseConnID.begin(); iter != pClientUnit->m_mapClientSocketUseConnID.end();) {
		CONNID dwConnID = iter->first;
		CClientSocket* pClientSocket = iter->second;
		iter++;

		// 如果某服务socket有对应的对话框，则关闭
		// 注意只有删掉ClientUnit时才考虑关闭所有子socket对应的对话框
		// 如果只是子socket断开连接了，则不关闭对应对话框（否则，比如远程SHELL的断开连接前的数据将会丢失）
		if (pClientSocket->m_wServiceType != nsGeneralSocket::MAIN_SOCKET_SERVICE) {
			if (pClientSocket->m_pServiceDialog != nullptr) {
				delete pClientSocket->m_pServiceDialog;			// 各服务的析构函数中要DeleteClientSocket
				pClientSocket->m_pServiceDialog = nullptr;
			}
			else {
				delete pClientSocket->m_pServiceObject;
				pClientSocket->m_pServiceObject = nullptr;
			}
		}
		else {
			DeleteClientSocket(pClientSocket);
		}
	}

	// 从ClientManager的map中删掉此ClientUnit
	this->m_mapClientUnit.erase(pClientUnit->m_qwClientToken);
	DebugPrint("m_mapClientSocket.size = %d\n", m_mapClientSocket.size());
	DebugPrint("m_mapClientUnit.size = %d\n", m_mapClientUnit.size());

	// 从UI中删除此ClientUnit
	CListCtrl* pClientList = &((CMuaServerDlg*)(theApp.m_pMainWnd))->m_ClientList;
	for (DWORD dwIndex = 0; dwIndex < pClientList->GetItemCount(); dwIndex++) {
		// 获取所枚举的这一行的额外信息，pClientUnit
		LV_ITEM  lvitemData = { 0 };
		lvitemData.mask = LVIF_PARAM;
		lvitemData.iItem = dwIndex;
		pClientList->GetItem(&lvitemData);
		CClientUnit* pClientUnitTemp = (CClientUnit*)lvitemData.lParam;

		// 确定要删除的那行的索引，并删除这一行
		if (pClientUnit == pClientUnitTemp) {
			pClientList->DeleteItem(dwIndex);
			break;
		}
	}

	// 解锁	
	//LeaveCriticalSection(&pClientUnit->m_cs);			

	delete pClientUnit;
}