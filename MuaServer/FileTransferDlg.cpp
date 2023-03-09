#include "pch.h"
#include "FileTransferDlg.h"
#include "afxdialogex.h"

using namespace nsFileTransfer;


// CFileTransferDlg 对话框

IMPLEMENT_DYNAMIC(CFileTransferDlg, CDialogEx)

CFileTransferDlg::CFileTransferDlg(CClientSocket* pClientSocket, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_FILE_TRANSFER_DIALOG, pParent), CFileTransfer(pClientSocket, TRUE)
{
	m_pClientSocket = pClientSocket;

	this->Create(IDD_FILE_TRANSFER_DIALOG, GetDesktopWindow());
	this->ShowWindow(SW_SHOW);

	WCHAR pszTitle[64];
	wsprintf(pszTitle, L"文件传输    %s:%d\n", m_pClientSocket->m_wszIpAddress, m_pClientSocket->m_wPort);
	this->SetWindowText(pszTitle);

	// 修改CListCtrl的字体，使得等宽
	static CFont font;
	font.DeleteObject();
	font.CreatePointFont(100, _T("新宋体"));
	m_FileList.SetFont(&font);
	m_TaskList.SetFont(&font);

	// 文件列表
	CString head[] = { TEXT("名称"), TEXT("大小"), TEXT("类型"), TEXT("修改时间"), TEXT("属性"), TEXT("所有者") };		// TODO: 属性 和 所有者 是为Linux系统预留的
	m_FileList.InsertColumn(0, head[0], LVCFMT_LEFT, 300);
	m_FileList.InsertColumn(1, head[1], LVCFMT_LEFT, 200);
	m_FileList.InsertColumn(2, head[2], LVCFMT_LEFT, 100);
	m_FileList.InsertColumn(3, head[3], LVCFMT_LEFT, 200);
	m_FileList.InsertColumn(4, head[4], LVCFMT_LEFT, 100);
	m_FileList.InsertColumn(5, head[5], LVCFMT_LEFT, 100);
	m_FileList.SetExtendedStyle(m_FileList.GetExtendedStyle() | LVS_EX_FULLROWSELECT);

	// 文件传输列表
	CString head2[] = { TEXT("名称"), TEXT("状态"), TEXT("进度"), TEXT("大小"), TEXT("本地路径"), TEXT("远程路径"), TEXT("速度"),  TEXT("估计剩余时间"), TEXT("经过时间")};
	m_TaskList.InsertColumn(0, head2[0], LVCFMT_LEFT, 100);
	m_TaskList.InsertColumn(1, head2[1], LVCFMT_LEFT, 60);
	m_TaskList.InsertColumn(2, head2[2], LVCFMT_LEFT, 80);
	m_TaskList.InsertColumn(3, head2[3], LVCFMT_LEFT, 100);
	m_TaskList.InsertColumn(4, head2[4], LVCFMT_LEFT, 200);
	m_TaskList.InsertColumn(5, head2[5], LVCFMT_LEFT, 200);
	m_TaskList.InsertColumn(6, head2[6], LVCFMT_LEFT, 80);
	m_TaskList.InsertColumn(7, head2[7], LVCFMT_LEFT, 100);
	m_TaskList.InsertColumn(8, head2[8], LVCFMT_LEFT, 100);
	m_TaskList.SetExtendedStyle(m_TaskList.GetExtendedStyle() | LVS_EX_FULLROWSELECT);

	InitFileTransfer();
}


CFileTransferDlg::~CFileTransferDlg() {
	;
}


void CFileTransferDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_FILE_LIST, m_FileList);
	DDX_Control(pDX, IDC_EDIT_REMOTE_PATH, m_EditRemotePath);
	DDX_Control(pDX, IDC_TASK_LIST, m_TaskList);
}


// 解决回车键 ESC 默认关闭窗口
BOOL CFileTransferDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN) {
		if (GetFocus() == GetDlgItem(IDC_EDIT_REMOTE_PATH)) {
			WCHAR wszPath[MAX_PATH] = { 0 };
			m_EditRemotePath.GetWindowText(wszPath, MAX_PATH);
			SendListFilesCmd(wszPath);
		}
		return TRUE;
	}

	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_ESCAPE) {
		return TRUE;
	}

	return CDialog::PreTranslateMessage(pMsg);
}


void CFileTransferDlg::OnClose() {
	DestroySocket();

	CDialogEx::OnClose();
}


BEGIN_MESSAGE_MAP(CFileTransferDlg, CDialogEx)
	ON_WM_CLOSE()
	ON_NOTIFY(NM_DBLCLK, IDC_FILE_LIST, &CFileTransferDlg::OnDoubleClickFileList)
	ON_BN_CLICKED(IDC_BUTTON_UPLOAD_FILE, &CFileTransferDlg::OnBnClickedButtonUploadFile)
END_MESSAGE_MAP()


// CFileTransferDlg 消息处理程序


// 将硬盘信息添加到文件列表中显示
VOID CFileTransferDlg::AddHardLists2List(vector<_HardDiskUnit_WIN_C2S>& vHardDiskUnits) {
	USES_CONVERSION;

	// 清空列表（因为这是列出硬盘盘符）
	m_FileList.DeleteAllItems();

	for (int i = 0; i < vHardDiskUnits.size(); i++) {
		WCHAR wszMemory[MAX_PATH] = { 0 };
		wsprintf(wszMemory, L"%s 可用，共 %s", A2W(FileSize2Str(vHardDiskUnits[i].qwFreeBytes).c_str()), A2W(FileSize2Str(vHardDiskUnits[i].qwTotalBytes).c_str()));

		WCHAR wszType[MAX_PATH] = { 0 };
		wsprintf(wszType, L"%s", A2W(vHardDiskUnits[i].sHardDiskLabel.c_str()));

		// 插入到列表尾部
		DWORD dwInsertIndex = m_FileList.GetItemCount();

		LV_ITEM   lvitemData = { 0 };
		lvitemData.mask = LVIF_PARAM;
		lvitemData.iItem = dwInsertIndex;
		lvitemData.lParam = TYPE_HARD_DISK;				// 文件类型

		m_FileList.InsertItem(&lvitemData);
		m_FileList.SetItemText(dwInsertIndex, 0, A2W(vHardDiskUnits[i].sHardDiskName.c_str()));				// 名称			
		m_FileList.SetItemText(dwInsertIndex, 1, wszMemory);												// 大小
		m_FileList.SetItemText(dwInsertIndex, 2, wszType);													// 类型
		m_FileList.SetItemText(dwInsertIndex, 3, L"-");														// 修改时间
		m_FileList.SetItemText(dwInsertIndex, 4, L"-");														// 属性
		m_FileList.SetItemText(dwInsertIndex, 5, L"-");														// 所有者
	}

	m_EditRemotePath.SetWindowText(L"");
}


// 将文件、文件夹信息添加到文件列表中显示
VOID CFileTransferDlg::AddFiles2List(vector<_FileUnit_WIN_C2S>& vFileUnits, string sPath, BOOL bFirst) {
	USES_CONVERSION;

	if (bFirst) {
		// 清空列表
		m_FileList.DeleteAllItems();

		// 更改路径
		m_EditRemotePath.SetWindowText(A2W(sPath.c_str()));

		AddPointRow("..");
		AddPointRow(".");
	}

	for (int i = 0; i < vFileUnits.size(); i++) {
		// 插入到列表尾部
		DWORD dwInsertIndex = m_FileList.GetItemCount();

		LV_ITEM   lvitemData = { 0 };
		lvitemData.mask = LVIF_PARAM;
		lvitemData.iItem = dwInsertIndex;
		lvitemData.lParam = vFileUnits[i].iFileType;			// 文件类型

		m_FileList.InsertItem(&lvitemData);
		m_FileList.SetItemText(dwInsertIndex, 0, A2W(vFileUnits[i].sFileName.c_str()));						// 名称		
		
		if (vFileUnits[i].iFileType == TYPE_FILE) {
			m_FileList.SetItemText(dwInsertIndex, 1, A2W(FileSize2Str(vFileUnits[i].qwFileBytes).c_str()));	// 大小
		}
		else {
			m_FileList.SetItemText(dwInsertIndex, 1, L"-");
		}

		if (vFileUnits[i].iFileType == TYPE_FOLDER) {
			m_FileList.SetItemText(dwInsertIndex, 2, L"文件夹");											// 类型
		}
		else {
			m_FileList.SetItemText(dwInsertIndex, 2, L"-");
		}

		m_FileList.SetItemText(dwInsertIndex, 3, A2W(vFileUnits[i].sModifiedDate.c_str()));					// 修改时间
		m_FileList.SetItemText(dwInsertIndex, 4, L"-");														// 属性
		m_FileList.SetItemText(dwInsertIndex, 5, L"-");														// 所有者
	}
}


// 将QWORD格式的文件大小转换为可观字符串
string CFileTransferDlg::FileSize2Str(QWORD qwMemoryBytes) {
	CHAR szMemorySize[MAX_PATH] = { 0 };
	string sMemorySize = "";

	double fA, fB, fC, fD;

	if (qwMemoryBytes < 1024) {
		sprintf_s(szMemorySize, "%d B", (int)qwMemoryBytes);
	}
	else {
		fA = qwMemoryBytes / 1024;
		if (fA < 1024) {
			sprintf_s(szMemorySize, "%.2f KB", fA);
		}
		else {
			fB = fA / 1024;
			if (fB < 1024) {
				sprintf_s(szMemorySize, "%.2f MB", fB);
			}
			else {
				fC = fB / 1024;
				if (fC < 1024) {
					sprintf_s(szMemorySize, "%.2f GB", fC);
				}
				else {
					fD = fC / 1024;
					sprintf_s(szMemorySize, "%.2f TB", fD);
				}
			}
		}
	}

	sMemorySize = szMemorySize;
	return sMemorySize;
}


// 双击文件列表中的文件、文件夹
void CFileTransferDlg::OnDoubleClickFileList(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	
	int nItem = pNMItemActivate->iItem;

	LV_ITEM  lvitemData = { 0 };
	lvitemData.mask = LVIF_PARAM;
	lvitemData.iItem = nItem;
	m_FileList.GetItem(&lvitemData);
	FILE_TYPE iFileType = (FILE_TYPE)lvitemData.lParam;

	switch(iFileType) {
	case TYPE_POINT: {
		// 当前路径下的文件夹（相对路径）
		WCHAR wszDirectory[MAX_PATH] = { 0 };
		m_FileList.GetItemText(nItem, 0, wszDirectory, MAX_PATH);

		// 当前路径
		WCHAR wszCurrentPath[MAX_PATH] = { 0 };
		m_EditRemotePath.GetWindowText(wszCurrentPath, MAX_PATH);

		PWCHAR pszLastPointer = NULL;
		if (wcscmp(wszDirectory, L"..") == 0) {
			if (wszCurrentPath[wcslen(wszCurrentPath)-1] == L'\\' && wcslen(wszCurrentPath) == 3) {
				GetServerSocket()->SendWithEnc(m_pClientSocket->m_dwConnID, LIST_HARD_DISKS_S2C);
			}
			else {
				pszLastPointer = wcsrchr(wszCurrentPath, L'\\');
				if (wcscmp(pszLastPointer, L"\\") != 0) {
					*pszLastPointer = NULL;
					m_EditRemotePath.SetWindowTextW(wszCurrentPath);
				}
				if (wcslen(wszCurrentPath) < 3) {
					*pszLastPointer = L'\\';
					*(pszLastPointer + 1) = NULL;
				}
				SendListFilesCmd(wszCurrentPath);
			}
		}

		if (wcscmp(wszDirectory, L".") == 0) {
			// 当前路径
			WCHAR wszCurrentPath[MAX_PATH] = { 0 };
			m_EditRemotePath.GetWindowText(wszCurrentPath, MAX_PATH);

			SendListFilesCmd(wszCurrentPath);
		}

		break;
	}

	case TYPE_HARD_DISK:
	case TYPE_FOLDER: {
		// 当前路径下的文件夹（相对路径）
		WCHAR wszDirectory[MAX_PATH] = { 0 };
		m_FileList.GetItemText(nItem, 0, wszDirectory, MAX_PATH);

		// 当前路径
		WCHAR wszCurrentPath[MAX_PATH] = { 0 };
		m_EditRemotePath.GetWindowText(wszCurrentPath, MAX_PATH);

		// 拼接
		WCHAR wszRemotePath[MAX_PATH] = { 0 };
		if (wcslen(wszCurrentPath) == 0) {
			wcscpy_s(wszRemotePath, wszDirectory);
		}
		else {
			if (wszCurrentPath[wcslen(wszCurrentPath) - 1] == L'\\') {
				wsprintf(wszRemotePath, L"%s%s", wszCurrentPath, wszDirectory);
			}
			else {
				wsprintf(wszRemotePath, L"%s\\%s", wszCurrentPath, wszDirectory);
			}
		}

		// 发送命令
		SendListFilesCmd(wszRemotePath);
		break;
	}

	case TYPE_FILE:
		WCHAR wszLocalDir[MAX_PATH] = { 0 };
		BOOL bRet = ChooseLocalDir(wszLocalDir);
		if (bRet) {
			// 当前路径下的文件（相对路径）
			WCHAR wszFileName[MAX_PATH] = { 0 };
			m_FileList.GetItemText(nItem, 0, wszFileName, MAX_PATH);

			// 当前路径
			WCHAR wszCurrentPath[MAX_PATH] = { 0 };
			m_EditRemotePath.GetWindowText(wszCurrentPath, MAX_PATH);

			// 拼接
			WCHAR wszClientPath[MAX_PATH] = { 0 };
			if (wcslen(wszCurrentPath) == 0) {
				wcscpy_s(wszClientPath, wszFileName);
			}
			else {
				if (wszCurrentPath[wcslen(wszCurrentPath) - 1] == L'\\') {
					wsprintf(wszClientPath, L"%s%s", wszCurrentPath, wszFileName);
				}
				else {
					wsprintf(wszClientPath, L"%s\\%s", wszCurrentPath, wszFileName);
				}
			}

			WCHAR wszServerPath[MAX_PATH];
			wsprintf(wszServerPath, L"%s\\%s", wszLocalDir, wszFileName);

			AddTransferFileTask(DOWNLOAD, wszClientPath, wszServerPath);
		}

		break;
	}

	*pResult = 0;
}


BOOL CFileTransferDlg::ChooseLocalDir(PWCHAR pwszLocalDir) {
	BOOL bRet = FALSE;

	//// 获取特定文件夹的LPITEMIDLIST
	//LPITEMIDLIST rootLoation;
	//SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOP, &rootLoation);
	//if (rootLoation == NULL) {
	//	return bRet;
	//}

	//// 配置对话框  
	//BROWSEINFO bi;
	//ZeroMemory(&bi, sizeof(bi));
	//bi.pidlRoot = rootLoation;			// 文件夹对话框之根目录，不指定的话则为我的电脑  
	//bi.lpszTitle = _T("将文件下载到"); 
	//bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_DONTGOBELOWDOMAIN | BIF_NEWDIALOGSTYLE;

	//// 打开对话框
	//LPITEMIDLIST targetLocation = SHBrowseForFolder(&bi);
	//if (targetLocation != NULL) {
	//	if (SHGetPathFromIDList(targetLocation, pwszLocalDir)) {
	//		bRet = TRUE;
	//	}
	//}

	CFolderPickerDialog FolderDlg;
	FolderDlg.m_ofn.lpstrTitle = _T("将文件下载到");
	//FolderDlg.m_ofn.lpstrInitialDir = _T("C:\\");
	if (FolderDlg.DoModal() == IDOK) {
		CString csFolderPath = FolderDlg.GetPathName();
		wcscpy_s(pwszLocalDir, MAX_PATH, csFolderPath.AllocSysString());
		bRet = TRUE;
	}

	return bRet;
}


// 向文件列表中添加 . 和 ..
VOID CFileTransferDlg::AddPointRow(string sPoint) {
	USES_CONVERSION;

	int dwInsertIndex = 0;

	LV_ITEM  lvitemData = { 0 };
	lvitemData.mask = LVIF_PARAM;
	lvitemData.iItem = dwInsertIndex;
	lvitemData.lParam = TYPE_POINT;

	m_FileList.InsertItem(&lvitemData);
	m_FileList.SetItemText(dwInsertIndex, 0, A2W(sPoint.c_str()));	// 名称		
	m_FileList.SetItemText(dwInsertIndex, 1, L"-");					// 大小
	m_FileList.SetItemText(dwInsertIndex, 2, L"文件夹");			// 类型
	m_FileList.SetItemText(dwInsertIndex, 3, L"-");					// 修改时间
	m_FileList.SetItemText(dwInsertIndex, 4, L"-");					// 属性
	m_FileList.SetItemText(dwInsertIndex, 5, L"-");					// 所有者
}


// 将传输文件的信息添加到传输列表中
VOID CFileTransferDlg::AddTask2List(_TransferFileTaskInfo info) {
	USES_CONVERSION;

	// 插入到列表尾部
	DWORD dwInsertIndex = m_TaskList.GetItemCount();

	LV_ITEM   lvitemData = { 0 };
	lvitemData.mask = LVIF_PARAM;
	lvitemData.iItem = dwInsertIndex;
	lvitemData.lParam = (LPARAM)info.dwFileTag;											// 用于标识文件的tag

	WCHAR wszTransferStatus[32] = L"未知状态";
	if (info.iTransferType == DOWNLOAD) {
		wcscpy_s(wszTransferStatus, L"等待下载");
	}
	else {
		wcscpy_s(wszTransferStatus, L"等待上传");
	}

	m_TaskList.InsertItem(&lvitemData);
	m_TaskList.SetItemText(dwInsertIndex, 0, A2W(info.sFileName.c_str()));				// 名称			
	m_TaskList.SetItemText(dwInsertIndex, 1, wszTransferStatus);						// 状态
	m_TaskList.SetItemText(dwInsertIndex, 2, L"-");										// 进度
	m_TaskList.SetItemText(dwInsertIndex, 3, L"-");										// 大小
	m_TaskList.SetItemText(dwInsertIndex, 4, A2W(info.sLocalPath.c_str()));				// 本地路径
	m_TaskList.SetItemText(dwInsertIndex, 5, A2W(info.sRemotePath.c_str()));			// 远程路径
	m_TaskList.SetItemText(dwInsertIndex, 6, L"-");										// 速度
	m_TaskList.SetItemText(dwInsertIndex, 7, L"-");										// 估计剩余时间
	m_TaskList.SetItemText(dwInsertIndex, 8, L"-");										// 经过时间
}


VOID CFileTransferDlg::RemoteFileTaskFromList(DWORD dwFileTag) {
	for (int i = 0; i < m_TaskList.GetItemCount(); i++)
	{
		LV_ITEM  lvitemData = { 0 };
		lvitemData.mask = LVIF_PARAM;
		lvitemData.iItem = i;
		m_TaskList.GetItem(&lvitemData);
		DWORD dwFileTagTemp = lvitemData.lParam;

		if (dwFileTagTemp == dwFileTag) {
			m_TaskList.DeleteItem(i);
			break;
		}
	}
}


// 上传文件
void CFileTransferDlg::OnBnClickedButtonUploadFile()
{
	CFileDialog FileDlg(true, _T(""), _T(""), OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("*.* ||"), NULL);
	FileDlg.m_ofn.lpstrTitle = L"请选择需要上传的文件";

	if (FileDlg.DoModal() == IDOK)    //弹出对话框  
	{
		// 要上传的文件路径
		CString csFilePath = FileDlg.GetPathName();
		WCHAR wszServerPath[MAX_PATH] = { 0 };
		wcscpy_s(wszServerPath, csFilePath.AllocSysString());

		// 文件名
		CString csFileName = FileDlg.GetFileName();
			
		// 当前文件列表所处路径
		WCHAR wszClientPath[MAX_PATH] = { 0 };
		m_EditRemotePath.GetWindowText(wszClientPath, MAX_PATH);
		wsprintf(wszClientPath, L"%s\\%s", wszClientPath, csFileName.AllocSysString());

		AddTransferFileTask(UPLOAD, wszClientPath, wszServerPath);
	}
}



VOID CFileTransferDlg::UpdateFileDownloadInfo(_TransferFileTaskInfo FileInfo) {
	USES_CONVERSION;
	int iIndex = -1;
	for (int i = 0; i < m_TaskList.GetItemCount(); i++)
	{
		LV_ITEM  lvitemData = { 0 };
		lvitemData.mask = LVIF_PARAM;
		lvitemData.iItem = i;
		m_TaskList.GetItem(&lvitemData);
		DWORD dwFileTagTemp = lvitemData.lParam;

		if (dwFileTagTemp == FileInfo.dwFileTag) {
			iIndex = i;
			break;
		}
	}
	ASSERT(iIndex != -1);

	// 是否下载完成
	BOOL bOk = FileInfo.dwLastIndex * FILE_SECTION_MAX_LENGTH >= FileInfo.qwFileSize;

	double fProcess = (bOk) ? 1 : ((QWORD)FileInfo.dwLastIndex * FILE_SECTION_MAX_LENGTH * 1.0 / FileInfo.qwFileSize);
	CHAR szProcess[32] = { 0 };
	sprintf_s(szProcess, "%.2f %%", fProcess * 100);

	CHAR szFileSize[64] = { 0 };
	QWORD qwTransferSize = (bOk) ? (FileInfo.qwFileSize) : ((QWORD)FileInfo.dwLastIndex * FILE_SECTION_MAX_LENGTH);
	sprintf_s(szFileSize, "%s / %s", FileSize2Str(qwTransferSize).c_str(), FileSize2Str(FileInfo.qwFileSize).c_str());

	CHAR szRate[32] = { 0 };
	QWORD qwElapsedTime = (GetCurrentTimeStamp() - FileInfo.qwStartTime);
	if (qwElapsedTime == 0) qwElapsedTime = 1;
	sprintf_s(szRate, "%s/s", FileSize2Str(qwTransferSize / qwElapsedTime).c_str());
	
	CHAR szEstimateRemainingTime[32] = { 0 };
	QWORD qwEstimateRemainingTime = 1.0 * (FileInfo.qwFileSize - qwTransferSize) * qwElapsedTime / qwTransferSize;
	sprintf_s(szEstimateRemainingTime, "%d s", qwEstimateRemainingTime);

	CHAR szElapsedTime[32] = { 0 };
	sprintf_s(szElapsedTime, "%d s", qwElapsedTime);
	
	m_TaskList.SetItemText(iIndex, 2, A2W(szProcess));					// 进度
	m_TaskList.SetItemText(iIndex, 3, A2W(szFileSize));					// 大小
	m_TaskList.SetItemText(iIndex, 6, A2W(szRate));						// 平均速度
	m_TaskList.SetItemText(iIndex, 7, A2W(szEstimateRemainingTime));	// 预估剩余时间
	m_TaskList.SetItemText(iIndex, 8, A2W(szElapsedTime));				// 经过时间。TODO: 这里应该另起一个线程，每秒更新一次
}


VOID CFileTransferDlg::FileDownloadFailed(DWORD dwConnID, MyBuffer mBuffer) {
	USES_CONVERSION;
	_DataSocket* pDataSocket = SearchDataSocket(dwConnID);
	ASSERT(pDataSocket != nullptr);

	int iIndex = -1;
	for (int i = 0; i < m_TaskList.GetItemCount(); i++)
	{
		LV_ITEM  lvitemData = { 0 };
		lvitemData.mask = LVIF_PARAM;
		lvitemData.iItem = i;
		m_TaskList.GetItem(&lvitemData);
		DWORD dwFileTagTemp = lvitemData.lParam;

		if (dwFileTagTemp == pDataSocket->FileTransferInfo.dwFileTag) {
			iIndex = i;
			break;
		}
	}
	ASSERT(iIndex != -1);

	_FileDownloadFailed_C2S mData = MsgUnpack<_FileDownloadFailed_C2S>(mBuffer.ptr(), mBuffer.size());

	CHAR szErrorText[128] = { 0 };
	sprintf_s(szErrorText, "下载失败：%s", mData.sErrorInfo.c_str());

	m_TaskList.SetItemText(iIndex, 1, A2W(szErrorText));
}


//VOID CFileTransferDlg::BeforeFileUpload(CONNID dwConnID, msgpack::type::raw_ref msData) {
//	USES_CONVERSION;
//	_DataSocket* pDataSocket = SearchDataSocket(dwConnID);
//	ASSERT(pDataSocket != nullptr);
//
//	int iIndex = -1;
//	for (int i = 0; i < m_TaskList.GetItemCount(); i++)
//	{
//		LV_ITEM  lvitemData = { 0 };
//		lvitemData.mask = LVIF_PARAM;
//		lvitemData.iItem = i;
//		m_TaskList.GetItem(&lvitemData);
//		DWORD dwFileTagTemp = lvitemData.lParam;
//
//		if (dwFileTagTemp == pDataSocket->FileTransferInfo.dwFileTag) {
//			iIndex = i;
//			break;
//		}
//	}
//	ASSERT(iIndex != -1);
//
//	_FileUploadCmd_C2S mData = MsgUnpack<_FileUploadCmd_C2S>((PBYTE)msData.ptr, msData.size);
//
//	if (!mData.bOpenFile) {
//		m_TaskList.SetItemText(iIndex, 1, L"上传失败：文件创建失败");
//
//		// TODO
//		return;
//	}
//
//	SendFileData(pDataSocket);
//
//}


VOID CFileTransferDlg::UpdateFileUploadInfo(_TransferFileTaskInfo FileInfo) {
	USES_CONVERSION;
	int iIndex = -1;
	for (int i = 0; i < m_TaskList.GetItemCount(); i++)
	{
		LV_ITEM  lvitemData = { 0 };
		lvitemData.mask = LVIF_PARAM;
		lvitemData.iItem = i;
		m_TaskList.GetItem(&lvitemData);
		DWORD dwFileTagTemp = lvitemData.lParam;

		if (dwFileTagTemp == FileInfo.dwFileTag) {
			iIndex = i;
			break;
		}
	}
	ASSERT(iIndex != -1);

	// 是否上传完成
	BOOL bOk = FileInfo.dwSectionNum == FileInfo.dwLastIndex;

	double fProcess = (bOk) ? 1 : (((QWORD)FileInfo.dwLastIndex * FILE_SECTION_MAX_LENGTH * 1.0) / FileInfo.qwFileSize);
	CHAR szProcess[32] = { 0 };
	sprintf_s(szProcess, "%.2f %%", fProcess * 100);

	CHAR szFileSize[64] = { 0 };
	QWORD qwTransferSize = (bOk) ? (FileInfo.qwFileSize) : ((QWORD)FileInfo.dwLastIndex * FILE_SECTION_MAX_LENGTH);
	sprintf_s(szFileSize, "%s / %s", FileSize2Str(qwTransferSize).c_str(), FileSize2Str(FileInfo.qwFileSize).c_str());

	CHAR szRate[32] = { 0 };
	QWORD qwElapsedTime = (GetCurrentTimeStamp() - FileInfo.qwStartTime);
	if (qwElapsedTime == 0) qwElapsedTime = 1;
	sprintf_s(szRate, "%s/s", FileSize2Str(qwTransferSize / qwElapsedTime).c_str());

	CHAR szEstimateRemainingTime[32] = { 0 };
	QWORD qwEstimateRemainingTime = 1.0 * (FileInfo.qwFileSize - qwTransferSize) * qwElapsedTime / qwTransferSize;
	sprintf_s(szEstimateRemainingTime, "%d s", qwEstimateRemainingTime);

	CHAR szElapsedTime[32] = { 0 };
	sprintf_s(szElapsedTime, "%d s", qwElapsedTime);

	m_TaskList.SetItemText(iIndex, 2, A2W(szProcess));					// 进度
	m_TaskList.SetItemText(iIndex, 3, A2W(szFileSize));					// 大小
	m_TaskList.SetItemText(iIndex, 6, A2W(szRate));						// 平均速度
	m_TaskList.SetItemText(iIndex, 7, A2W(szEstimateRemainingTime));	// 预估剩余时间
	m_TaskList.SetItemText(iIndex, 8, A2W(szElapsedTime));				// 经过时间。TODO: 这里应该另起一个线程，每秒更新一次
}