#include "pch.h"
#include "MainSocket.h"


typedef struct _REBORN_THREAD_PARAM {
	WCHAR m_pwszAddress[32];
	WORD m_wPort;
	_REBORN_THREAD_PARAM(wchar_t* pwszAddress, wchar_t* pwszPort) {
		wcscpy_s(m_pwszAddress, pwszAddress);
		char pszPort[10];
		WideCharToMultiByte(CP_ACP, NULL, pwszPort, -1, pszPort, 10, NULL, NULL);
		m_wPort = atoi(pszPort);
	}
}REBORN_THREAD_PARAM;


void WINAPI StartClientThreadFunc(REBORN_THREAD_PARAM* pThreadParam);
CMainSocket* StartClientFuncBody(CMainSocket* pMainSocket, LPWSTR pszAddress, WORD wPort);


// 用VS调试的话请在 "MuaClient -> 配置属性 -> 调试 -> 命令参数" 中修改IP地址和端口
int wmain(int argc, wchar_t* argv[]) {
	if (argc < 3)
	{
		CHAR pszPath[MAX_PATH];
		WideCharToMultiByte(CP_ACP, NULL, argv[0], -1, pszPath, MAX_PATH, NULL, NULL);
		printf("Usage:\n %s <Host> <Port>\n", pszPath);
		system("pause");
		return -1;
	}

	REBORN_THREAD_PARAM* pThreadParam = new REBORN_THREAD_PARAM(argv[1], argv[2]);
	HANDLE hRebornThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)StartClientThreadFunc, pThreadParam, 0, NULL);

	// 之前hRebornThread进程里面的new CSocketClient()一直很奇怪地失败，我还以为是CSocketClient的构造函数的问题，
	// 没想到只是我的主线程退出了而已。小丑竟是我自己。
	WaitForSingleObject(hRebornThread, INFINITE);
}


void WINAPI StartClientThreadFunc(REBORN_THREAD_PARAM* pThreadParam) {
	WCHAR wszAddress[32];
	wcscpy_s(wszAddress, pThreadParam->m_pwszAddress);		// TODO 检查 pThreadParam->m_pwszAddress 长度
	WORD wPort = pThreadParam->m_wPort;
	delete pThreadParam;

	CMainSocket* pMainSocket = nullptr;
	while (true) {
		__try {
			pMainSocket = StartClientFuncBody(pMainSocket, wszAddress, wPort);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			pMainSocket = nullptr;
			DebugPrint("无视异常，重连服务端\n");
		}
	}
}


// SEH所在函数不能有对象展开，所以单独放到一个函数里
CMainSocket* StartClientFuncBody(CMainSocket* pMainSocketTemp, LPWSTR pwszAddress, WORD wPort) {
	CMainSocket* pMainSocket = pMainSocketTemp;

	if (pMainSocket == nullptr) {
		pMainSocket = new CMainSocket(TCP_PACK);
		pMainSocket->InitSocket(pwszAddress, wPort);
		pMainSocket->StartSocket();
	}

	// 未连接时新建一个连接
	if (!pMainSocket->IsConnected()) {
		delete pMainSocket;
		pMainSocket = new CMainSocket(TCP_PACK);
		pMainSocket->InitSocket(pwszAddress, wPort);
		DebugPrint("正在重连服务端.....\n");
		pMainSocket->StartSocket();
	}

	// 5秒重试一次
	Sleep(5000);

	return pMainSocket;
}

