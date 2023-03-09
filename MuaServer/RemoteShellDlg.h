 #pragma once
#include "Service.h"

// CRemoteShellDlg 对话框

class CRemoteShellDlg : public CDialogEx, public CService
{
	DECLARE_DYNAMIC(CRemoteShellDlg)

public:
	CRemoteShellDlg(CClientSocket* pClientSocket, CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CRemoteShellDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_REMOTE_SHELL_DIALOG };
#endif


public:
	CClientSocket* m_pClientSocket = nullptr;

	CEdit m_EditCmd;
	CEdit m_EditResult;

	DWORD m_dwBufferTail = 0;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	DECLARE_MESSAGE_MAP()

	
public:
	afx_msg void OnClose();
	afx_msg void OnBnClickedExecCmd();
	VOID OnReceiveWithDec(ITcpServer* pSender, CONNID dwConnID, QWORD qwClientToken, WORD wServiceType, WORD wSocketType, int iCommandID, DWORD dwParentSocketTag, MyBuffer mBuffer);
	VOID RecvExecCmdResult(MyBuffer mBuffer);
};
