#include "pch.h"
#include "Service.h"


CClientManager* CService::GetClientManager() {
	return ((CMuaServerDlg*)(theApp.m_pMainWnd))->m_pClientManager;
}


CServerSocket* CService::GetServerSocket() {
	return ((CMuaServerDlg*)(theApp.m_pMainWnd))->m_pServerSocket;
}