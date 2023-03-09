#pragma once
#include "Service.h"
#include "FileTransfer.h"

// CFileTransferDlg 对话框

class CFileTransferDlg : public CDialogEx, public CFileTransfer
{
	DECLARE_DYNAMIC(CFileTransferDlg)

public:
	CFileTransferDlg(CClientSocket* pClientSocket, CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CFileTransferDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_FILE_TRANSFER_DIALOG };
#endif

public:
	CClientSocket* m_pClientSocket = nullptr;
	string sCurrentRemotePath;
	vector<HANDLE> m_vhFiles;
	CListCtrl m_FileList;
	CEdit m_EditRemotePath;


protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnClose();
	afx_msg void OnDoubleClickFileList(NMHDR* pNMHDR, LRESULT* pResult);

	VOID AddHardLists2List(vector<nsFileTransfer::_HardDiskUnit_WIN_C2S>& vHardDiskUnits);
	VOID AddFiles2List(vector<nsFileTransfer::_FileUnit_WIN_C2S>& vFileUnits, string sPath, BOOL bFirst=TRUE);
	string FileSize2Str(QWORD qwMemoryBytes);
	VOID AddPointRow(string sPoint);
	VOID AddTask2List(_TransferFileTaskInfo info);
	CListCtrl m_TaskList;
	VOID RemoteFileTaskFromList(DWORD dwFileTag);
	BOOL ChooseLocalDir(PWCHAR pwszLocalDir);
	afx_msg void OnBnClickedButtonUpload();
	afx_msg void OnBnClickedButtonUploadFile();

	VOID UpdateFileDownloadInfo(_TransferFileTaskInfo FileInfo);

	VOID FileDownloadFailed(DWORD dwConnID, MyBuffer mBuffer);

	VOID UpdateFileUploadInfo(_TransferFileTaskInfo FileInfo);
};
