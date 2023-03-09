#pragma once
#include "Service.h"

// CImageCaptureDlg 对话框

class CImageCaptureDlg : public CDialogEx, public CService
{
	DECLARE_DYNAMIC(CImageCaptureDlg)

public:
	CImageCaptureDlg(CClientSocket* pClientSocket, CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CImageCaptureDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_IMAGE_CAPTURE_DIALOG };
#endif

public:
	CClientSocket* m_pClientSocket = nullptr;

	CHAR m_szLastImageTime[32] = { 0 };			// 上一个文件的时间
	DWORD m_dwCurrentTimeImageNum = 1;			// 同一时间内的文件数量

	CHAR m_szImagePath[MAX_PATH] = { 0 };		// 当前文件的文件名
	DWORD m_dwImageSectionIndex = 0;			// 已接收到的当前文件的最新的序号


protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	DECLARE_MESSAGE_MAP()


public:
	afx_msg void OnClose();

	VOID OnReceiveWithDec(ITcpServer* pSender, CONNID dwConnID, QWORD qwClientToken, WORD wServiceType, WORD wSocketType, int iCommandID, DWORD dwParentSocketTag, MyBuffer mBuffer);
	afx_msg void OnBnClickedPrintScreen();


	VOID RecvImage(CONNID dwConnID, QWORD qwClientToken, WORD wServiceType, WORD wSocketType, int iCommandID, MyBuffer mBuffer);
	void DisplayPrintScreen(CONNID dwConnID, QWORD qwClientToken, WORD wServiceType, WORD wSocketType, int iCommandID, MyBuffer mBuffer);
	
	VOID LoadImage(PCHAR csImagePath);

};
