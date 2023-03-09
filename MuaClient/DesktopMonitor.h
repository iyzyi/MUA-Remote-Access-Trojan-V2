#pragma once
#include "Socket.h"


struct _MonitorProperty {
	int                         width;
	int                         height;
	int                         x;
	int                         y;
	HDC                         hdc;
	HMONITOR                    hmonitor;
	std::basic_string<TCHAR>    monitor_name;
	bool                        flag;
};


class CDesktopMonitor : public CSocket {
public:
	QWORD					m_qwClientToken;
	CDesktopMonitor* m_pImageSocket = nullptr;


public:
	CDesktopMonitor(TCP_MODE iTcpMode, WORD wSocketType, QWORD qwClientToken, DWORD dwParentSocketTag = 0);
	~CDesktopMonitor();

protected:
	HANDLE			m_hExitThreadEvent;

	VOID OnReceiveWithDec(ITcpClient* pSender, CONNID dwConnID, QWORD qwClientToken, WORD wServiceType, WORD wSocketType, int iCommandID, DWORD dwParentSocketTag, MyBuffer mBuffer);

	EnHandleResult OnClose(ITcpClient* pSender, CONNID dwConnID, EnSocketOperation enOperation, int iErrorCode);

	
	static VOID ListScreen(LPVOID lParam);

	static VOID DisplayScreen(LPVOID lParam);

	static VOID PrintScreenThreadFunc(LPVOID lParam);

	VOID PrintScreen(_MonitorProperty monitor);

	VOID SendImage(PBYTE pbImageData, DWORD dwImageLength);

	BOOL				m_bChooseMonitor = FALSE;
	_MonitorProperty	m_ChooseMonitor;

	//static VOID SendImage(LPVOID lParam, PBYTE pbImageData, DWORD dwImageLength);
	//static VOID PrintScreen(LPVOID lParam);
};


struct _DisplayScreenThreadParam {
	CDesktopMonitor*			pThis;
	string						sScreenName;
	_DisplayScreenThreadParam(CDesktopMonitor* pThis, string sScreenName) : pThis(pThis), sScreenName(sScreenName) { }
};

