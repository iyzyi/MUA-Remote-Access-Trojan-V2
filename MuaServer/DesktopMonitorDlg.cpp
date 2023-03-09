#include "pch.h"
#include "DesktopMonitorDlg.h"

#include "afxdialogex.h"


using namespace nsDesktopMonitor;


// CImageCaptureDlg 对话框

IMPLEMENT_DYNAMIC(CDesktopMonitorDlg, CDialogEx)

CDesktopMonitorDlg::CDesktopMonitorDlg(CClientSocket* pClientSocket, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DESKTOP_MONITOR_DIALOG, pParent)
{
	m_pClientSocket = pClientSocket;

	this->Create(IDD_DESKTOP_MONITOR_DIALOG, GetDesktopWindow());
	this->ShowWindow(SW_SHOW);

	WCHAR pszTitle[64];
	wsprintf(pszTitle, L"桌面监控    %s:%d\n", m_pClientSocket->m_wszIpAddress, m_pClientSocket->m_wPort);
	this->SetWindowText(pszTitle);
	
	SetNullImage();

	GetServerSocket()->SendWithEnc(m_pClientSocket->m_dwConnID, LIST_SCREEN_S2C);

	InitDesktopMonitor();
}


void CDesktopMonitorDlg::OnClose()
{
	DestroySocket();

	CDialogEx::OnClose();
}


void CDesktopMonitorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_SCREEN, m_ComboScreen);
	DDX_Control(pDX, IDC_DESKTOP_MONITOR_DISPLAY, m_PicCtrl);
}


// 解决回车键 ESC 默认关闭窗口
BOOL CDesktopMonitorDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_ESCAPE)     return   TRUE;
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)   return   TRUE;
	else
		return CDialog::PreTranslateMessage(pMsg);
}


BEGIN_MESSAGE_MAP(CDesktopMonitorDlg, CDialogEx)
	ON_WM_CLOSE()
	ON_CBN_SELCHANGE(IDC_COMBO_SCREEN, &CDesktopMonitorDlg::OnCbnSelchangeComboScreen)
	ON_WM_SIZE()
END_MESSAGE_MAP()


// CImageCaptureDlg 消息处理程序



CDesktopMonitorDlg::~CDesktopMonitorDlg() {
	DestroySocket();
}


// 断开相关的Socket，同时退出死循环的线程
VOID CDesktopMonitorDlg::DestroySocket() {
	if (m_pClientSocket != nullptr) {
		GetClientManager()->DeleteClientSocket(m_pClientSocket);
		m_pClientSocket = nullptr;
	}
	if (m_pImageSocket != nullptr) {
		GetClientManager()->DeleteClientSocket(m_pImageSocket);
		m_pImageSocket = nullptr;
	}
	if (m_pAudioSocket != nullptr) {
		GetClientManager()->DeleteClientSocket(m_pAudioSocket);
		m_pAudioSocket = nullptr;
	}
}



VOID CDesktopMonitorDlg::OnReceiveWithDec(ITcpServer* pSender, CONNID dwConnID, QWORD qwClientToken, WORD wServiceType, WORD wSocketType, int iCommandID, DWORD dwParentSocketTag, MyBuffer mBuffer) {
	switch (iCommandID) {
	case LIST_SCREEN_C2S:
		OnRecv_ListScreenC2S(dwConnID, qwClientToken, wServiceType, wSocketType, iCommandID, mBuffer);
		break;

	case DISPLAY_SCREEN_C2S:
		OnRecv_DisplayScreenC2S(dwConnID, qwClientToken, wServiceType, wSocketType, iCommandID, mBuffer);
		break;
	}
}


// 初始化
VOID CDesktopMonitorDlg::InitDesktopMonitor() {
	CClientUnit* pClientUnit = GetClientManager()->GetClientUnitByToken(m_pClientSocket->m_qwClientToken);

	// 该socket用于传输图像
	GetServerSocket()->SendCreateChildSocket(pClientUnit->m_dwMainSocketConnID, CREATE_IMAGE_SOCKET_S2C, m_pClientSocket->m_dwSocketTag);

	//// 该socket用于传输音频
	//GetServerSocket()->SendCreateChildSocket(pClientUnit->m_dwMainSocketConnID, CREATE_AUDIO_SOCKET_S2C, m_pClientSocket->m_dwSocketTag);

	InitializeCriticalSection(&m_ImageSectionCS);
	InitializeCriticalSection(&m_LoadImageCS);
}



// 调整窗口大小时触发
void CDesktopMonitorDlg::OnSize(UINT nType, int cx, int cy) {
	CDialogEx::OnSize(nType, cx, cy);

	ChangeWidget(cx, cy);
}


// 动态调整控件大小
void CDesktopMonitorDlg::ChangeWidget(int cx, int cy) {
	// 图像区域
	CRect rt1;
	CStatic* pPicCtrl = (CStatic*)GetDlgItem(IDC_DESKTOP_MONITOR_DISPLAY);
	if (pPicCtrl == NULL)
		return;
	pPicCtrl->GetWindowRect(rt1);
	ScreenToClient(rt1);
	rt1.right = cx;
	rt1.bottom = cy  - 50;
	pPicCtrl->MoveWindow(rt1);

	// 下拉框
	CRect rt2;
	CComboBox* pComboBox = (CComboBox*)GetDlgItem(IDC_COMBO_SCREEN);
	if (pComboBox == NULL)
		return;
	pComboBox->GetWindowRect(rt2);
	ScreenToClient(rt2);
	rt2.top = cy - 30;
	rt2.bottom = cy;
	pComboBox->MoveWindow(rt2);

	// 文字
	CRect rt3;
	CStatic* pStatic = (CStatic*)GetDlgItem(IDC_STATIC_SCREEN_NAME);
	if (pStatic == NULL)
		return;
	pStatic->GetWindowRect(rt3);
	ScreenToClient(rt3);
	rt3.left = 0;
	rt3.top = cy - 30;
	rt3.bottom = cy;
	pStatic->MoveWindow(rt3);

	if (m_bLastImageOK == TRUE) {
		LoadImage(TRUE);
	}
}


// 下拉框中列出各个显示器（屏幕）
VOID CDesktopMonitorDlg::OnRecv_ListScreenC2S(CONNID dwConnID, QWORD qwClientToken, WORD wServiceType, WORD wSocketType, int iCommandID, MyBuffer mBuffer) {

	m_vScreenUnits.clear();
	m_ComboScreen.ResetContent();

	_ListScreen_C2S mData = MsgUnpack<_ListScreen_C2S>((PBYTE)mBuffer.ptr(), mBuffer.size());
	vector<_ScreenUnit_WIN_C2S> vScreenUnits = MsgGetUnitsFromBuf<_ScreenUnit_WIN_C2S>((PBYTE)mData.msData.ptr, mData.dwScreenNum);

	USES_CONVERSION;
	int iIndex = -1;
	for (int i = 0; i < vScreenUnits.size(); i++) {
		_ScreenUnit ScreenUnit(vScreenUnits[i].sScreenName, vScreenUnits[i].dwWidth, vScreenUnits[i].dwHeight, vScreenUnits[i].dwX, vScreenUnits[i].dwY, vScreenUnits[i].bIsMainScreen);
		m_vScreenUnits.push_back(ScreenUnit);

		string sScreenName;
		if (vScreenUnits[i].sScreenName.find("\\\\.\\" == 0)) {
			sScreenName = vScreenUnits[i].sScreenName.substr(4);
		}
		else {
			sScreenName = vScreenUnits[i].sScreenName;
		}

		WCHAR wszScreenInfo[128];
		if (vScreenUnits[i].bIsMainScreen) {
			iIndex = i;
			wsprintf(wszScreenInfo, L"%s (%dx%d) (主)", A2W(sScreenName.c_str()), vScreenUnits[i].dwWidth, vScreenUnits[i].dwHeight);
		}
		else {
			wsprintf(wszScreenInfo, L"%s (%dx%d)", A2W(sScreenName.c_str()), vScreenUnits[i].dwWidth, vScreenUnits[i].dwHeight);
		}
		m_ComboScreen.AddString(wszScreenInfo);
	}

	// 默认选择 主显示器
	if (iIndex != -1) {
		m_ComboScreen.SetCurSel(iIndex);
		OnCbnSelchangeComboScreen();
	}
}


// 直到子socket创建成功后，才向客户端发送 DISPLAY_SCREEN_S2C
VOID CDesktopMonitorDlg::UntilCreateChildSocketSuccess(LPARAM lParam) {
	_UntilCreateChildSocketSuccessThreadParam* pThreadParam = (_UntilCreateChildSocketSuccessThreadParam*)lParam;
	CDesktopMonitorDlg* pThis = pThreadParam->pThis;
	string sScreenName = pThreadParam->sScreenName;
	delete pThreadParam;

	while (pThis->m_pImageSocket == nullptr) {		// TODO 检查音频Socket是否创建成功
		// TODO 计时器
		Sleep(100);
	}

	_DisplayScreen_S2C mData;
	mData.sScreenName = sScreenName;
	PBYTE pBuffer;
	MyBuffer mBuffer = MsgPack<_DisplayScreen_S2C>(mData);
	pThis->GetServerSocket()->SendWithEnc(pThis->m_pClientSocket->m_dwConnID, DISPLAY_SCREEN_S2C, mBuffer.ptr(), mBuffer.size());
}


// 下拉框更改显示器
void CDesktopMonitorDlg::OnCbnSelchangeComboScreen() {
	int iIndex = m_ComboScreen.GetCurSel();
	if (iIndex != CB_ERR) {
		_UntilCreateChildSocketSuccessThreadParam* pThreadParam = new _UntilCreateChildSocketSuccessThreadParam(this, m_vScreenUnits[iIndex].sScreenName);
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)UntilCreateChildSocketSuccess, (LPVOID)pThreadParam, 0, NULL);
	}
}


// 用于传输屏幕截图的Socket创建好了
VOID CDesktopMonitorDlg::CreateImageSocketSuccess(CClientSocket* pClientSocket) {
	m_pImageSocket = pClientSocket;
}


VOID CDesktopMonitorDlg::OnRecv_DisplayScreenC2S(CONNID dwConnID, QWORD qwClientToken, WORD wServiceType, WORD wSocketType, int iCommandID, MyBuffer mBuffer) {
	
	EnterCriticalSection(&m_ImageSectionCS);				// 加锁

	_DisplayScreen_C2S mData = MsgUnpack<_DisplayScreen_C2S>((PBYTE)mBuffer.ptr(), mBuffer.size());

	// 此种情况必为Client端在伪造报文，试图消耗Server的资源。
	if (mData.dwSectionNum != m_dwImageSectionIndex + 1 && mData.dwSectionIndex != m_dwImageSectionIndex + 1) {
		// TODO: 断开连接
		LeaveCriticalSection(&m_ImageSectionCS);			// 解锁
		return;
	}

	// 图像的首个切片
	if (mData.dwSectionIndex == 1) {
		m_bLastImageOK = FALSE;
		ASSERT(m_qwImageTag == 0);
		m_qwImageTag = mData.qwTime;
	}
	
	if (m_qwImageTag != mData.qwTime) {
		// TODO: 断开连接
		LeaveCriticalSection(&m_ImageSectionCS);			// 解锁
		return;
	}

	else {

		// 拷贝切片
		memcpy(m_pbImageData + m_dwImageRecvSize, mData.msImageSection.ptr, mData.msImageSection.size);

		// 图像的最后一个切片
		if (mData.dwSectionIndex == 0) {
			LoadImage();
			m_bLastImageOK = TRUE;
			m_dwImageSectionIndex = 0;
			m_dwImageRecvSize = 0;
			m_qwImageTag = 0;
		}
		// 不是最后一个切片
		else {
			m_dwImageSectionIndex++;
			m_dwImageRecvSize += mData.msImageSection.size;
		}
	}

	LeaveCriticalSection(&m_ImageSectionCS);				// 解锁
}


VOID CDesktopMonitorDlg::LoadImage(BOOL bResize/* = FALSE*/) {

	EnterCriticalSection(&m_LoadImageCS);		// 加锁

	BYTE*				pBmpData;				// 图像数据  
	BITMAPINFO*			pBmpInfo;				// 图像信息  
	BITMAPFILEHEADER	BmpHeader;				// 文件头  
	BITMAPINFOHEADER	BmpInfo;				// 信息头  

	memcpy(&BmpHeader, m_pbImageData, sizeof(BITMAPFILEHEADER));
	memcpy(&BmpInfo, m_pbImageData + sizeof(BITMAPFILEHEADER), sizeof(BITMAPINFOHEADER));
	pBmpData = m_pbImageData + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

	MyBuffer mBmpInfo = MyBuffer(sizeof(BITMAPINFOHEADER));
	memcpy(mBmpInfo.ptr(), &BmpInfo, sizeof(BITMAPINFOHEADER));

	// 显示图像相关
	CRect rect;
	m_PicCtrl.GetClientRect(&rect);					// 获得图像控件所在的矩形区域  
	CDC* pDC = m_PicCtrl.GetDC();					// 获得图像控件的DC
	pDC->SetStretchBltMode(COLORONCOLOR);

	// 计算 居中缩放图像 所用的坐标数据
	int nImgW = BmpInfo.biWidth, nImgH = BmpInfo.biHeight;		// 图像宽 高
	int nCtlW = rect.Width(), nCtlH = rect.Height();			// 控件宽 高
	int nW, nH;													// 计算后的控件宽 高
	int nTopX, nTopY;											// 计算后的控件顶点
	if (nImgW * nCtlH > nCtlW * nImgH) {  
		nW = nCtlW;
		nH = MulDiv(nW, nImgH, nImgW);
		nTopX = 0;
		nTopY = (nCtlH - nH) / 2;
	}
	else {
		nH = nCtlH;
		nW = MulDiv(nH, nImgW, nImgH);
		nTopX = (nCtlW - nW) / 2;
		nTopY = 0;
	}

	if (bResize) {
		SetNullImage();
	}

	// 居中缩放
	StretchDIBits(pDC->GetSafeHdc(), nTopX, nTopY, nW, nH, 0, 0, BmpInfo.biWidth, BmpInfo.biHeight, pBmpData, (BITMAPINFO*)mBmpInfo.ptr(), DIB_RGB_COLORS, SRCCOPY);

	// 拉伸
	//StretchDIBits(pDC->GetSafeHdc(), 0, 0, iWidth, iHeight, 0, 0, BmpInfo.biWidth, BmpInfo.biHeight, pBmpData, pBmpInfo, DIB_RGB_COLORS, SRCCOPY);

	LeaveCriticalSection(&m_LoadImageCS);			// 解锁
}


// 图像区域置空
VOID CDesktopMonitorDlg::SetNullImage() {
	CRect rect;
	m_PicCtrl.GetClientRect(&rect);					// 获得图像控件所在的矩形区域
	CDC* pDC = m_PicCtrl.GetDC();						// 获得图像控件的DC
	pDC->SetStretchBltMode(COLORONCOLOR);

	StretchDIBits(pDC->GetSafeHdc(), 0, 0, rect.Width(), rect.Height(), 0, 0, 0, 0, NULL, NULL, DIB_RGB_COLORS, BLACKNESS);
}