#include "pch.h"
#include "MyTcpPushServer.h"


CMyTcpPushServer::CMyTcpPushServer() : m_pTcpPushServer(this) {
	;
}


VOID CMyTcpPushServer::InitSocket() {
	m_bIsRunning = FALSE;

	// 设置心跳检测包发送间隔
	m_pTcpPushServer->SetKeepAliveTime(60 * 1000);
	// 设置心跳检测重试包发送间隔
	m_pTcpPushServer->SetKeepAliveInterval(20 * 1000);
}


VOID CMyTcpPushServer::SetLocalAddress(PWCHAR pwszAddress, WORD wPort) {
	wcscpy_s(m_wszAddress, pwszAddress);
	m_wPort = wPort;
}


// 启动socket服务端
BOOL CMyTcpPushServer::StartSocket() {

	BOOL bRet = m_pTcpPushServer->Start(m_wszAddress, m_wPort);
	if (!bRet) {
		return FALSE;
	}
	else {

#ifdef _DEBUG
		USES_CONVERSION;									// 使用A2W之前先声明这个
		DebugPrint("Socket服务端启动成功，模式TCP_PUSH，IP=%s, PORT=%d\n", W2A(m_wszAddress), m_wPort);
#endif

		m_bIsRunning = TRUE;
		return TRUE;
	}
}


BOOL CMyTcpPushServer::StopSocket() {
	BOOL bRet = m_pTcpPushServer->Stop();
	if (bRet) {
		m_bIsRunning = FALSE;
		return TRUE;
	}
	else {
		return FALSE;
	}
}


BOOL CMyTcpPushServer::Send(CONNID dwConnID, const BYTE* pBuffer, int iLength, int iOffset) {
	if (!m_pTcpPushServer->IsConnected(dwConnID)) {
		return FALSE;
	}

	BOOL bRet = m_pTcpPushServer->Send(dwConnID, pBuffer, iLength, iOffset);
	return bRet;
}


// 回调函数的实现

EnHandleResult CMyTcpPushServer::OnPrepareListen(ITcpServer* pSender, SOCKET soListen) {
	DebugPrint("OnPrepareListen: \n");
	return HR_OK;
}


EnHandleResult CMyTcpPushServer::OnAccept(ITcpServer* pSender, CONNID dwConnID, SOCKET soClient) {
	DebugPrint("[Client %d] OnAccept: \n", dwConnID);
	return HR_OK;
}


EnHandleResult CMyTcpPushServer::OnHandShake(ITcpServer* pSender, CONNID dwConnID) {
	DebugPrint("[Client %d] OnHandShake: \n", dwConnID);
	return HR_OK;
}


EnHandleResult CMyTcpPushServer::OnSend(ITcpServer* pSender, CONNID dwConnID, const BYTE* pData, int iLength) {
	DebugPrint("[Client %d] OnSend: %d Bytes \n", dwConnID, iLength);
	return HR_OK;
}


EnHandleResult CMyTcpPushServer::OnReceive(ITcpServer* pSender, CONNID dwConnID, const BYTE* pData, int iLength) {
	DebugPrint("[Client %d] OnReceive: %d Bytes \n", dwConnID, iLength);
	return HR_OK;
}


EnHandleResult CMyTcpPushServer::OnClose(ITcpServer* pSender, CONNID dwConnID, EnSocketOperation enOperation, int iErrorCode) {
	DebugPrint("[Client %d] OnClose: \n", dwConnID);
	return HR_OK;
}


EnHandleResult CMyTcpPushServer::OnShutdown(ITcpServer* pSender) {
	DebugPrint("OnShutdown: \n");
	DebugPrint("Socket服务端关闭成功\n");
	return HR_OK;
}