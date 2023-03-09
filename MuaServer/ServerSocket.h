#pragma once
#include "Socket.h"
#include "Client/ClientManager.h"


class CServerSocket : public CSocket {
public:
	CServerSocket(TCP_MODE iTcpMode);

protected:

	VOID OnReceiveWithDec(ITcpServer* pSender, CONNID dwConnID, QWORD qwClientToken, WORD wServiceType, WORD wSocketType, int iCommandID, DWORD dwParentSocketTag, MyBuffer mBuffer);

};