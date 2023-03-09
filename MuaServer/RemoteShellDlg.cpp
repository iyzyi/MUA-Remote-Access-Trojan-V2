#include "pch.h"
#include "RemoteShellDlg.h"
#include "afxdialogex.h"


using namespace nsRemoteShell;


#define CMD_RESULT_BUFFER_LEN		8192


// CRemoteShellDlg 对话框

IMPLEMENT_DYNAMIC(CRemoteShellDlg, CDialogEx)

CRemoteShellDlg::CRemoteShellDlg(CClientSocket* pClientSocket, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_REMOTE_SHELL_DIALOG, pParent)
{
	m_pClientSocket = pClientSocket;

	this->Create(IDD_REMOTE_SHELL_DIALOG, GetDesktopWindow());
	this->ShowWindow(SW_SHOW);

	WCHAR pszTitle[64];
	wsprintf(pszTitle, L"远程SHELL    %s:%d\n", m_pClientSocket->m_wszIpAddress, m_pClientSocket->m_wPort);
	this->SetWindowText(pszTitle);

	// 修改焦点到编辑框上
	GetDlgItem(IDC_EXEC_CMD)->SetFocus();

	//设置显示最大字符数
	m_EditResult.SetLimitText(UINT_MAX);

	// 修改字体，使得等宽
	static CFont font;
	font.DeleteObject();
	font.CreatePointFont(100, _T("新宋体"));
	m_EditResult.SetFont(&font);
}


CRemoteShellDlg::~CRemoteShellDlg()
{
	if (m_pClientSocket != nullptr) {
		GetClientManager()->DeleteClientSocket(m_pClientSocket);
		m_pClientSocket = nullptr;
	}
}


void CRemoteShellDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_INPUT_CMD, m_EditCmd);
	DDX_Control(pDX, IDC_CMD_RESULT, m_EditResult);
}


// 解决回车键 ESC 默认关闭窗口
BOOL CRemoteShellDlg::PreTranslateMessage(MSG* pMsg)
{
	// 按下回车键并且焦点在输入执行命令的那个编辑框上 等于 按下执行命令按钮
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN) {
		if (GetFocus() == GetDlgItem(IDC_INPUT_CMD)) {
			OnBnClickedExecCmd();
		}
		return TRUE;
	}

	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_ESCAPE) {
		return TRUE;
	}

	return CDialog::PreTranslateMessage(pMsg);
}


BEGIN_MESSAGE_MAP(CRemoteShellDlg, CDialogEx)
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_EXEC_CMD, &CRemoteShellDlg::OnBnClickedExecCmd)
END_MESSAGE_MAP()


// CRemoteShellDlg 消息处理程序


void CRemoteShellDlg::OnClose()
{
	if (m_pClientSocket != nullptr) {
		GetClientManager()->DeleteClientSocket(m_pClientSocket);
		m_pClientSocket = nullptr;
	}

	CDialogEx::OnClose();
}


void CRemoteShellDlg::OnBnClickedExecCmd()
{
	WCHAR wszCmd[256];
	m_EditCmd.GetWindowText(wszCmd, 256);

	string sCmd = Wchars2Str(wszCmd);

	BOOL bClear = (sCmd == "cls") || (sCmd == "clear");

	_ExecCmd_S2C mData;
	mData.sCmd = bClear ? "" : sCmd;
	
	if (bClear) {
		m_EditResult.SetWindowText(L"");
	}

	MyBuffer mBuffer = MsgPack<_ExecCmd_S2C>(mData);

	GetServerSocket()->SendWithEnc(m_pClientSocket->m_dwConnID, EXEC_CMD_S2C, mBuffer.ptr(), mBuffer.size());

	m_EditCmd.SetWindowText(L"");
}


VOID CRemoteShellDlg::OnReceiveWithDec(ITcpServer* pSender, CONNID dwConnID, QWORD qwClientToken, WORD wServiceType, WORD wSocketType, int iCommandID, DWORD dwParentSocketTag, MyBuffer mBuffer) {
	switch (iCommandID) {
	case EXEC_CMD_C2S:
		RecvExecCmdResult(mBuffer);
		break;
	}
}


VOID CRemoteShellDlg::RecvExecCmdResult(MyBuffer mBuffer) {
	_ExecCmd_C2S mData = MsgUnpack<_ExecCmd_C2S>((PBYTE)mBuffer.ptr(), mBuffer.size());

	DWORD dwWideCharLength = MultiByteToWideChar(CP_ACP, 0, mData.sResult.c_str(), mData.sResult.length(), NULL, 0);
	PWCHAR pszWideCharTemp = new WCHAR[dwWideCharLength + 1];
	MultiByteToWideChar(CP_ACP, 0, mData.sResult.c_str(), mData.sResult.length(), pszWideCharTemp, dwWideCharLength);
	pszWideCharTemp[dwWideCharLength] = NULL;

	if (dwWideCharLength > CMD_RESULT_BUFFER_LEN) {
		m_EditResult.SetWindowText(pszWideCharTemp + dwWideCharLength - CMD_RESULT_BUFFER_LEN);
		m_dwBufferTail = CMD_RESULT_BUFFER_LEN;
	}
	else if (m_dwBufferTail + dwWideCharLength <= CMD_RESULT_BUFFER_LEN) {
		m_EditResult.SetSel(-1);
		m_EditResult.ReplaceSel(pszWideCharTemp);
		m_dwBufferTail += dwWideCharLength;
	}
	else {
		m_EditResult.SetSel(0, m_dwBufferTail + dwWideCharLength - CMD_RESULT_BUFFER_LEN, FALSE);
		m_EditResult.ReplaceSel(L"");

		m_EditResult.SetSel(-1);
		m_EditResult.ReplaceSel(pszWideCharTemp);

		m_dwBufferTail = CMD_RESULT_BUFFER_LEN;
	}

	delete[] pszWideCharTemp;
}