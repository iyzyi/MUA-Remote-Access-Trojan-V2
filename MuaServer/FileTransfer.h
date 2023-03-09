#pragma once
#include "Service.h"


enum FILE_TRANSFER_TYPE { UNKNOWN, DOWNLOAD, UPLOAD };


struct _TransferFileTaskInfo {
	DWORD					dwFileTag = 0;
	FILE_TRANSFER_TYPE		iTransferType = UNKNOWN;
	string					sFileName = "";
	string					sRemotePath = "";
	string					sLocalPath = "";

	DWORD					dwLastIndex = 0;
	HANDLE					hFile = nullptr;
	QWORD					qwFileSize = 0;
	DWORD					dwSectionNum = 0;
	QWORD					qwStartTime = 0;

	_TransferFileTaskInfo(DWORD dwFileTag = 0, FILE_TRANSFER_TYPE iTransferType = UNKNOWN, string sFileName = "", string sRemotePath = "", string sLocalPath = "") {
		this->dwFileTag = dwFileTag;
		this->iTransferType = iTransferType;
		this->sFileName = sFileName;
		this->sRemotePath = sRemotePath;
		this->sLocalPath = sLocalPath;
	};
};


struct _DataSocket {
	CClientSocket*			pClientSocket	= nullptr;
	BOOL					bOnTransfering	= FALSE;		// 正在传输文件与否
	CRITICAL_SECTION		WriteCS;
	_TransferFileTaskInfo	FileTransferInfo = _TransferFileTaskInfo();
};


class CFileTransfer : public CService{
protected:
	vector<_TransferFileTaskInfo>   m_vFileTaskOnWaiting;
	vector<_TransferFileTaskInfo>	m_vFileTaskOnStarting;
	
	vector<_DataSocket> m_vDataSocket;
	BOOL				m_bWithDialog;

	HANDLE				m_hExitThreadEvent;


	DWORD				m_dwGlobalFileTag = 0;


public:
	CClientSocket* m_pClientSocket = nullptr;

	CClientSocket* m_pListSocket = nullptr;

public:
	CFileTransfer(CClientSocket* pClientSocket, BOOL bWithDialog);

	VOID InitFileTransfer();


	VOID OnReceiveWithDec(ITcpServer* pSender, CONNID dwConnID, QWORD qwClientToken, WORD wServiceType, WORD wSocketType, int iCommandID, DWORD dwParentSocketTag, MyBuffer mBuffer);

	~CFileTransfer();

	VOID DestroySocket();

	VOID CreateListSocketSuccess(CClientSocket* pClientSocket);

	VOID CreateFileSocketSuccess(CClientSocket* pClientSocket);

	_DataSocket* GetFreeDataSocket();


	VOID DisplayHardDisk(CONNID dwConnID, QWORD qwClientToken, WORD wServiceType, WORD wSocketType, int iCommandID, MyBuffer mBuffer);
	VOID DisplayFiles(CONNID dwConnID, QWORD qwClientToken, WORD wServiceType, WORD wSocketType, int iCommandID, MyBuffer mBuffer);



	VOID SendListFilesCmd(PWCHAR pwszRemotePath);

	VOID SendFileDownloadCmd(string sRemotePath, string sLocalPath);

	VOID AddTransferFileTask(FILE_TRANSFER_TYPE iTransferType, PWCHAR wszRemotePath, PWCHAR wszLocalPath);

	VOID AddTransferFolderTask(FILE_TRANSFER_TYPE iTransferType, PWCHAR wszRemotePath, PWCHAR wszLocalPath);

	_DataSocket* SearchDataSocket(CONNID dwConnID);

	VOID DeleteFileInfoAfterTransferOver(_DataSocket* pDataSocket);

	VOID BeforeRecvFileData(CONNID dwConnID, MyBuffer mBuffer);

	VOID RecvFileData(CONNID dwConnID, QWORD qwClientToken, WORD wServiceType, WORD wSocketType, int iCommandID, MyBuffer mBuffer);

	virtual VOID BeforeFileUpload(CONNID dwConnID, MyBuffer mBuffer);

	VOID SendFileData(_DataSocket* pDataSocket);

	VOID OnRecv_FileUpload_C2S(DWORD dwConnID, MyBuffer mBuffer);


	static VOID WINAPI SendTransferCmdThreadFunc(LPVOID lParam);

	virtual VOID AddHardLists2List(vector<nsFileTransfer::_HardDiskUnit_WIN_C2S>& vHardDiskUnits);
	virtual VOID AddFiles2List(vector<nsFileTransfer::_FileUnit_WIN_C2S>& vFileUnits, string sPath, BOOL bFirst = TRUE);
	virtual VOID AddTask2List(_TransferFileTaskInfo info);

	//VOID RecvFileData(CONNID dwConnID, QWORD qwClientToken, WORD wServiceType, WORD wSocketType, int iCommandID, msgpack::type::raw_ref msData);

	virtual VOID RemoteFileTaskFromList(DWORD dwFileTag);

	virtual VOID UpdateFileDownloadInfo(_TransferFileTaskInfo FileInfo);

	virtual VOID FileDownloadFailed(DWORD dwConnID, MyBuffer mBuffer);
	virtual VOID UpdateFileUploadInfo(_TransferFileTaskInfo FileInfo);
};
