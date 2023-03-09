#include "pch.h"
#include "MyTcpPackServer.h"


CMyTcpPackServer::CMyTcpPackServer() : m_pTcpPackServer(this) {
	;
}

 
VOID CMyTcpPackServer::InitSocket() {
	m_bIsRunning = FALSE;

	// 设置数据包最大长度（有效数据包最大长度不能超过0x3FFFFF字节(4MB-1B)，默认：262144/0x40000 (256KB)
	m_pTcpPackServer->SetMaxPackSize(PACKET_MAX_LENGTH);
	// 设置心跳检测包发送间隔
	m_pTcpPackServer->SetKeepAliveTime(60 * 1000);
	// 设置心跳检测重试包发送间隔
	m_pTcpPackServer->SetKeepAliveInterval(20 * 1000);
}


VOID CMyTcpPackServer::SetLocalAddress(PWCHAR pwszAddress, WORD wPort) {
	wcscpy_s(m_wszAddress, pwszAddress);	
	m_wPort = wPort;
}


// 启动socket服务端
BOOL CMyTcpPackServer::StartSocket() {

	BOOL bRet = m_pTcpPackServer->Start(m_wszAddress, m_wPort);
	if (!bRet) {
		return FALSE;
	}
	else {

#ifdef _DEBUG
		USES_CONVERSION;									// 使用A2W之前先声明这个
		DebugPrint("Socket服务端启动成功，模式TCP_PACK，IP=%s, PORT=%d\n", W2A(m_wszAddress), m_wPort);
#endif

		m_bIsRunning = TRUE;
		return TRUE;
	}
}


BOOL CMyTcpPackServer::StopSocket() {
	BOOL bRet = m_pTcpPackServer->Stop();
	if (bRet) {
		m_bIsRunning = FALSE;
		return TRUE;
	}
	else {
		return FALSE;
	}
}


BOOL CMyTcpPackServer::Send(CONNID dwConnID, const BYTE* pBuffer, int iLength, int iOffset) {
	if (!m_pTcpPackServer->IsConnected(dwConnID)) {
		return FALSE;
	}

	BOOL bRet = m_pTcpPackServer->Send(dwConnID, pBuffer, iLength, iOffset);
	return bRet;
}


// 回调函数的实现

EnHandleResult CMyTcpPackServer::OnPrepareListen(ITcpServer* pSender, SOCKET soListen) {
	DebugPrint("OnPrepareListen: \n");
	return HR_OK;
}


EnHandleResult CMyTcpPackServer::OnAccept(ITcpServer* pSender, CONNID dwConnID, SOCKET soClient) {
	DebugPrint("[Client %d] OnAccept: \n", dwConnID);
	return HR_OK;
}


EnHandleResult CMyTcpPackServer::OnHandShake(ITcpServer* pSender, CONNID dwConnID) {
	DebugPrint("[Client %d] OnHandShake: \n", dwConnID);
	return HR_OK;
}


EnHandleResult CMyTcpPackServer::OnSend(ITcpServer* pSender, CONNID dwConnID, const BYTE* pData, int iLength) {
	DebugPrint("[Client %d] OnSend: %d Bytes \n", dwConnID, iLength);
	return HR_OK;
}


EnHandleResult CMyTcpPackServer::OnReceive(ITcpServer* pSender, CONNID dwConnID, const BYTE* pData, int iLength) {
	DebugPrint("[Client %d] OnReceive: %d Bytes \n", dwConnID, iLength);
	return HR_OK;
}


EnHandleResult CMyTcpPackServer::OnClose(ITcpServer* pSender, CONNID dwConnID, EnSocketOperation enOperation, int iErrorCode) {
	DebugPrint("[Client %d] OnClose: \n", dwConnID);
	return HR_OK;
}


EnHandleResult CMyTcpPackServer::OnShutdown(ITcpServer* pSender) {
	DebugPrint("OnShutdown: \n");
	DebugPrint("Socket服务端关闭成功\n");
	return HR_OK;
}