#pragma once

#include "pch.h"

class CMyTcpPackClient : public CTcpClientListener {

public:
	WCHAR					m_wszAddress[32];
	WORD					m_wPort = 0;

	CTcpPackClientPtr		m_pTcpPackClient;

	BOOL					m_bIsRunning = FALSE;
	CONNID					m_dwConnectId;


public:
	CMyTcpPackClient();

	VOID SetRemoteAddress(LPWSTR pszAddress, WORD wPort);
	VOID InitSocket();
	BOOL StartSocket();
	BOOL StopSocket();
	BOOL Send(BYTE* pBuffer, int iLength, int iOffset = 0);
	BOOL IsConnected();


protected:
	// 重写回调函数
	virtual EnHandleResult OnPrepareConnect(ITcpClient* pSender, CONNID dwConnID, SOCKET socket);
	virtual EnHandleResult OnConnect(ITcpClient* pSender, CONNID dwConnID);
	virtual EnHandleResult OnHandShake(ITcpClient* pSender, CONNID dwConnID);
	virtual EnHandleResult OnSend(ITcpClient* pSender, CONNID dwConnID, const BYTE* pData, int iLength);
	virtual EnHandleResult OnReceive(ITcpClient* pSender, CONNID dwConnID, const BYTE* pData, int iLength);
	virtual EnHandleResult OnClose(ITcpClient* pSender, CONNID dwConnID, EnSocketOperation enOperation, int iErrorCode);
};