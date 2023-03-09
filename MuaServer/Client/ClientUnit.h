#pragma once

#include <unordered_map>

#include "ClientSocket.h"


// 一个真实的Mua客户端对应一个CClientUnit对象，内含多个CClientSocket
class CClientUnit{
public:
	unordered_map<CONNID, CClientSocket*> m_mapClientSocketUseConnID;
	unordered_map<DWORD, CClientSocket*> m_mapClientSocketUseTag;

	// 一个Mua的客户端有且只有一个TOKEN
	QWORD		m_qwClientToken = 0;

	CONNID		m_dwMainSocketConnID = 0;

	WCHAR		m_wszIpAddress[32] = { 0 };
	WORD		m_wPort = 0;

	CRITICAL_SECTION m_cs;

public:
	CClientUnit(CONNID dwConnID);

	VOID AddClientSocket(CONNID dwConnID, CClientSocket* pClientSocket);

	CClientSocket* GetClientSocketByConnId(CONNID dwConnID);

	CClientSocket* GetClientSocketByTag(DWORD dwSocketTag);

	BOOL IsSocketTagConflict(DWORD dwSocketTag);
};

