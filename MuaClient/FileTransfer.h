#pragma once
#include "Socket.h"


// 本结构用于文件S2C
struct _FileInfo {
	DWORD					dwFileTag = 0;
	string					sLocalPath = "";
	HANDLE					hFile = nullptr;
	QWORD					qwFileSize = 0;
	DWORD					dwSectionNum = 0;
	QWORD					qwStartTime = 0;

	DWORD					dwLastIndex = 0;
	CRITICAL_SECTION		WriteCS;

	_FileInfo(DWORD dwFileTag, string sLocalPath, HANDLE hFile, QWORD qwFileSize, DWORD dwSectionNum) {
		this->dwFileTag = dwFileTag;
		this->sLocalPath = sLocalPath;
		this->hFile = hFile;
		this->qwFileSize = qwFileSize;
		this->dwSectionNum = dwSectionNum;
		this->qwStartTime = GetCurrentTimeStamp();
		InitializeCriticalSection(&WriteCS);
	};
};



class CFileTransfer : public CSocket{
public:
	QWORD m_qwClientToken;

	vector<_FileInfo>	m_vFileTask;


public:
	CFileTransfer(TCP_MODE iTcpMode, WORD wSocketType, QWORD qwClientToken, DWORD dwParentSocketTag = 0);
	~CFileTransfer();

protected:
	VOID OnReceiveWithDec(ITcpClient* pSender, CONNID dwConnID, QWORD qwClientToken, WORD wServiceType, WORD wSocketType, int iCommandID, DWORD dwParentSocketTag, MyBuffer mBuffer);

	EnHandleResult OnClose(ITcpClient* pSender, CONNID dwConnID, EnSocketOperation enOperation, int iErrorCode);

	static VOID ListHardDisks(LPVOID lParam);
	static VOID ListFiles(LPVOID lParam);
	static VOID BeforeSendFileData(LPVOID lParam);
	static VOID SendFileData(LPVOID lParam);
	static VOID BeforeRecvFileData(LPVOID lParam);
	VOID RecvFileData(MyBuffer mBuffer);
};



typedef struct _LIST_FILES_THREAD_PARAM {
	CFileTransfer*	m_pThis;
	string			m_sPath;

	_LIST_FILES_THREAD_PARAM(CFileTransfer* pThis, string sPath) {
		m_pThis = pThis;
		m_sPath = sPath;
	}
};


typedef struct _RECV_FILE_DATA_THREAD_PARAM {
	CFileTransfer*	pThis;
	DWORD			dwFileTag;
	string			sClientPath;
	QWORD			qwFileSize;
	DWORD			dwSectionNum;

	_RECV_FILE_DATA_THREAD_PARAM(CFileTransfer* pThis, DWORD dwFileTag, string sClientPath, QWORD qwFileSize, DWORD dwSectionNum) {
		this->pThis = pThis;
		this->dwFileTag = dwFileTag;
		this->sClientPath = sClientPath;
		this->qwFileSize = qwFileSize;
		this->dwSectionNum = dwSectionNum;
	}
};


typedef struct _SEND_FILE_DATA_THREAD_PARAM {
	CFileTransfer*	pThis;
	DWORD			dwFileTag;
	string			sClientPath;

	_SEND_FILE_DATA_THREAD_PARAM(CFileTransfer* pThis, DWORD dwFileTag, string sClientPath) {
		this->pThis = pThis;
		this->dwFileTag = dwFileTag;
		this->sClientPath = sClientPath;
	}
};