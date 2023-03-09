#pragma once
#include "Socket.h"


class CMainSocket : public CSocket {

protected:
	BOOL		m_bServerSendCryptoList = FALSE;




public:
	CMainSocket(TCP_MODE iTcpMode);

	//BOOL StartSocket();


protected:
	VOID OnReceiveWithDec(ITcpClient* pSender, CONNID dwConnID, QWORD qwClientToken, WORD wServiceType, WORD wSocketType, int iCommandID, DWORD dwParentSocketTag, MyBuffer mBuffer);

	VOID SendLoginPacket();
	DWORD GetParentSocketTag(MyBuffer mBuffer);
};



string GetHostName();
string GetOsVersion();
string GetCpuType();
string GetMemoryInfo();
DWORD GetCameraNum();
