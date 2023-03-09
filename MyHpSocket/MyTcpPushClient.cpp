#include "pch.h"
#include "MyTcpPushClient.h"

CMyTcpPushClient::CMyTcpPushClient() : m_pTcpPushClient(this) {
	;
}


VOID CMyTcpPushClient::InitSocket() {
	m_bIsRunning = FALSE;

	m_dwConnectId = 0;

	// 设置心跳检测包发送间隔
	m_pTcpPushClient->SetKeepAliveTime(60 * 1000);
	// 设置心跳检测重试包发送间隔
	m_pTcpPushClient->SetKeepAliveInterval(20 * 1000);
}


VOID CMyTcpPushClient::SetRemoteAddress(PWCHAR pwszAddress, WORD wPort) {
	wcscpy_s(m_wszAddress, pwszAddress);
	m_wPort = wPort;
}


BOOL CMyTcpPushClient::StartSocket() {

	BOOL bRet = TRUE;

	if (!(m_pTcpPushClient->IsConnected())) {
		bRet = m_pTcpPushClient->Start(m_wszAddress, m_wPort, 0);
		if (!bRet) {
			return FALSE;
		}
	}

	m_bIsRunning = TRUE;

	m_dwConnectId = m_pTcpPushClient->GetConnectionID();
	return bRet;
}


BOOL CMyTcpPushClient::StopSocket() {
	BOOL bRet = m_pTcpPushClient->Stop();
	if (bRet) {
		m_bIsRunning = FALSE;
		return TRUE;
	}
	else {
		return FALSE;
	}
}


BOOL CMyTcpPushClient::Send(BYTE* pBuffer, int iLength, int iOffset) {
	if (!m_pTcpPushClient->IsConnected()) {
		return FALSE;
	}

	BOOL bRet;

	if (m_bIsRunning == TRUE) {
		bRet = m_pTcpPushClient->Send(pBuffer, iLength, iOffset);
	}
	else {
		bRet = FALSE;
	}

	return bRet;
}


BOOL CMyTcpPushClient::IsConnected() {
	return m_pTcpPushClient->IsConnected();
}


// 回调函数

EnHandleResult CMyTcpPushClient::OnPrepareConnect(ITcpClient* pSender, CONNID dwConnID, SOCKET socket) {
	DebugPrint("[Client %d] OnPrepareConnect: \n", dwConnID);
	return HR_OK;
}


EnHandleResult CMyTcpPushClient::OnConnect(ITcpClient* pSender, CONNID dwConnID) {
	DebugPrint("[Client %d] OnConnect: \n", dwConnID);
	return HR_OK;
}


EnHandleResult CMyTcpPushClient::OnHandShake(ITcpClient* pSender, CONNID dwConnID) {
	DebugPrint("[Client %d] OnHandShake: \n", dwConnID);
	return HR_OK;
}


EnHandleResult CMyTcpPushClient::OnSend(ITcpClient* pSender, CONNID dwConnID, const BYTE* pData, int iLength) {
	DebugPrint("[Client %d] OnSend: %d Bytes \n", dwConnID, iLength);
	return HR_OK;
}


EnHandleResult CMyTcpPushClient::OnReceive(ITcpClient* pSender, CONNID dwConnID, const BYTE* pData, int iLength) {
	DebugPrint("[Client %d] OnReceive: %d Bytes \n", dwConnID, iLength);
	return HR_OK;
}


EnHandleResult CMyTcpPushClient::OnClose(ITcpClient* pSender, CONNID dwConnID, EnSocketOperation enOperation, int iErrorCode) {
	DebugPrint("[Client %d] OnClose: \n", dwConnID);
	return HR_OK;
}