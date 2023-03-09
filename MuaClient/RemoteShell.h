#pragma once
#include "Socket.h"


// CMD命令的最大长度
#define SHELL_MAX_LENGTH 2048

#define SEND_BUFFER_MAX_LENGTH 8096


class CRemoteShell : public CSocket{
public:
	QWORD					m_qwClientToken;

	HANDLE					m_hExitThreadEvent;

	CRITICAL_SECTION		m_ExecuteCs;
	
	HANDLE					m_hRead;
	HANDLE					m_hWrite;

	HANDLE					m_hJob;


public:
	CRemoteShell(TCP_MODE iTcpMode, QWORD qwClientToken);
	~CRemoteShell();

protected:
	VOID OnReceiveWithDec(ITcpClient* pSender, CONNID dwConnID, QWORD qwClientToken, WORD wServiceType, WORD wSocketType, int iCommandID, DWORD dwParentSocketTag, MyBuffer mBuffer);

	EnHandleResult OnClose(ITcpClient* pSender, CONNID dwConnID, EnSocketOperation enOperation, int iErrorCode);

	VOID RunCmdProcess();

	static DWORD WINAPI RunCmdProcessThreadFunc(LPVOID lParam);
	static VOID WINAPI OnRecvExecCmd(LPVOID lParam);
	static VOID WINAPI LoopReadAndSendCommandReuslt(LPVOID lParam);

};


typedef struct _RECV_EXEC_CMD_THREAD_PARAM {
	CRemoteShell*	m_pThis;
	string			m_sCmd;
	_RECV_EXEC_CMD_THREAD_PARAM(CRemoteShell* pThis, string sCmd) {
		m_pThis = pThis;
		m_sCmd = sCmd;
	}
};