#pragma once

#include <unordered_map>

#include "ClientUnit.h"
#include "../ServerSocket.h"


class CServerSocket;


// 管理所有的Mua客户端
class CClientManager{
private:
	unordered_map<QWORD, CClientUnit*> m_mapClientUnit;
	unordered_map<CONNID, CClientSocket*> m_mapClientSocket;

	CRITICAL_SECTION m_cs;

public:
	CClientManager();
	~CClientManager();

	// 为新ClientUtil分配TOKEN前需要检查此TOKEN是否和其他的ClientUtil的TOKEN冲突
	BOOL IsTokenConflict(QWORD qwClientToken);

	VOID AddClientUtil(QWORD qwClientToken, CClientUnit* pClientUnit);
	VOID AddClientSocket(CONNID dwConnID, CClientSocket* pClientSocket);

	CClientUnit* GetClientUnitByConnId(CONNID dwConnID);
	CClientUnit* GetClientUnitByToken(QWORD qwClientToken);
	CClientSocket* GetClientSocketByConnId(CONNID dwConnID);

	CServerSocket* GetServerSocket();

	VOID DeleteClientSocket(CClientSocket* pClientSocket);
	VOID DeleteClientUnit(CClientUnit* pClientUnit);
};