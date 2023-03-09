#pragma once

#include "pch.h"

class CMyTcpPackServer : public CTcpServerListener {
public:
	WCHAR					m_wszAddress[32];
	WORD					m_wPort = 0;

	CTcpPackServerPtr		m_pTcpPackServer;

	BOOL					m_bIsRunning = FALSE;


public:
	CMyTcpPackServer();

	VOID SetLocalAddress(PWCHAR pwszAddress, WORD wPort);
	VOID InitSocket();
	BOOL StartSocket();
	BOOL StopSocket();
	BOOL Send(CONNID dwConnID, const BYTE* pBuffer, int iLength, int iOffset = 0);


protected:
	// CTcpServerListener的抽象函数（用于回调）全都得实现，不然会报错：不能实例化抽象类
	virtual EnHandleResult OnPrepareListen(ITcpServer* pSender, SOCKET soListen);
	virtual EnHandleResult OnAccept(ITcpServer* pSender, CONNID dwConnID, SOCKET soClient);
	virtual EnHandleResult OnHandShake(ITcpServer* pSender, CONNID dwConnID);
	virtual EnHandleResult OnSend(ITcpServer* pSender, CONNID dwConnID, const BYTE* pData, int iLength);
	virtual EnHandleResult OnReceive(ITcpServer* pSender, CONNID dwConnID, const BYTE* pData, int iLength);
	virtual EnHandleResult OnClose(ITcpServer* pSender, CONNID dwConnID, EnSocketOperation enOperation, int iErrorCode);
	virtual EnHandleResult OnShutdown(ITcpServer* pSender);
};