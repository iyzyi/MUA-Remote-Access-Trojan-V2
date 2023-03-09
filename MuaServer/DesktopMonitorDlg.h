#pragma once
#include "Service.h"


struct _ScreenUnit {
	string							sScreenName;
	DWORD							dwWidth;
	DWORD							dwHeight;
	DWORD							dwX;
	DWORD							dwY;
	BOOL							bIsMainScreen;
	
	_ScreenUnit(string sScreenName, DWORD dwWidth, DWORD dwHeight, DWORD dwX, DWORD dwY, BOOL bIsMainScreen) : sScreenName(sScreenName), dwWidth(dwWidth), dwHeight(dwHeight), dwX(dwX), dwY(dwY), bIsMainScreen(bIsMainScreen){
		;
	}
};


// CImageCaptureDlg 对话框

class CDesktopMonitorDlg : public CDialogEx, public CService
{
	DECLARE_DYNAMIC(CDesktopMonitorDlg)

public:
	CDesktopMonitorDlg(CClientSocket* pClientSocket, CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CDesktopMonitorDlg();

	// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DESKTOP_MONITOR_DIALOG };
#endif

public:
	CClientSocket* m_pClientSocket = nullptr;

	//CHAR m_szLastImageTime[32] = { 0 };			// 上一个文件的时间
	//DWORD m_dwCurrentTimeImageNum = 1;			// 同一时间内的文件数量

	//CHAR m_szImagePath[MAX_PATH] = { 0 };		// 当前文件的文件名
	//DWORD m_dwImageSectionIndex = 0;			// 已接收到的当前文件的最新的序号


protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	DECLARE_MESSAGE_MAP()


public:
	afx_msg void OnClose();


	VOID OnReceiveWithDec(ITcpServer* pSender, CONNID dwConnID, QWORD qwClientToken, WORD wServiceType, WORD wSocketType, int iCommandID, DWORD dwParentSocketTag, MyBuffer mBuffer);
	VOID InitDesktopMonitor();
	VOID OnRecv_ListScreenC2S(CONNID dwConnID, QWORD qwClientToken, WORD wServiceType, WORD wSocketType, int iCommandID, MyBuffer mBuffer);
	//afx_msg void OnBnClickedPrintScreen();


	//VOID RecvImage(CONNID dwConnID, QWORD qwClientToken, WORD wServiceType, WORD wSocketType, int iCommandID, msgpack::type::raw_ref msData);
	//void DisplayPrintScreen(CONNID dwConnID, QWORD qwClientToken, WORD wServiceType, WORD wSocketType, int iCommandID, msgpack::type::raw_ref msData);

	//VOID LoadImage(PCHAR csImagePath);

	CComboBox m_ComboScreen;
	CStatic m_PicCtrl;
	vector<_ScreenUnit> m_vScreenUnits;
	afx_msg void OnCbnSelchangeComboScreen();

	VOID CreateImageSocketSuccess(CClientSocket* pClientSocket);

	afx_msg void OnSize(UINT nType, int cx, int cy);
	VOID DestroySocket();

	void ChangeWidget(int cx, int cy);

	CClientSocket* m_pImageSocket = nullptr;
	CClientSocket* m_pAudioSocket = nullptr;

	static VOID UntilCreateChildSocketSuccess(LPARAM lParam);

	VOID OnRecv_DisplayScreenC2S(CONNID dwConnID, QWORD qwClientToken, WORD wServiceType, WORD wSocketType, int iCommandID, MyBuffer mBuffer);

	DWORD m_dwImageSectionIndex = 0;

	QWORD m_qwImageTag = 0;									// 使用毫秒级时间戳来标识图像
	BOOL m_bLastImageOK = TRUE;
	BYTE m_pbImageData[4096 * 2160 * 4 + 2048] = { 0 };		// 目前暂时预设缓冲区为 32色深 4K分辨率
	DWORD m_dwImageRecvSize = 0;							// 已接收的图像的大小
	CRITICAL_SECTION		m_ImageSectionCS;
	CRITICAL_SECTION		m_LoadImageCS;

	VOID LoadImage(BOOL bResize = FALSE);
	VOID SetNullImage();
};


struct _UntilCreateChildSocketSuccessThreadParam {
	CDesktopMonitorDlg* pThis;
	string				sScreenName;
	_UntilCreateChildSocketSuccessThreadParam(CDesktopMonitorDlg* pThis, string sScreenName) : pThis(pThis), sScreenName(sScreenName) {	};
};