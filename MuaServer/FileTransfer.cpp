#include "pch.h"
#include "FileTransfer.h"

using namespace nsFileTransfer;


CFileTransfer::CFileTransfer(CClientSocket* pClientSocket, BOOL bWithDialog) {
	m_pClientSocket = pClientSocket;
	m_bWithDialog = bWithDialog;

	// 手动重置信号
	m_hExitThreadEvent = CreateEvent(NULL, true, false, NULL);

	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)SendTransferCmdThreadFunc, (LPVOID)this, 0, NULL);
}


// 初始化
VOID CFileTransfer::InitFileTransfer() {
	CClientUnit* pClientUnit = GetClientManager()->GetClientUnitByToken(m_pClientSocket->m_qwClientToken);

	// 该socket用于传输文件列表
	GetServerSocket()->SendCreateChildSocket(pClientUnit->m_dwMainSocketConnID, CREATE_LIST_SOCKET_S2C, m_pClientSocket->m_dwSocketTag);

	// 该socket用于传输文件
	for (int i = 0; i < FILE_CONCURRENT_DOWNLOAD_NUMBER; i++) {
		GetServerSocket()->SendCreateChildSocket(pClientUnit->m_dwMainSocketConnID, CREATE_FILE_SOCKET_S2C, m_pClientSocket->m_dwSocketTag);
	}
}


CFileTransfer::~CFileTransfer() {
	DestroySocket();
}


// 断开相关的Socket，同时退出死循环的线程
VOID CFileTransfer::DestroySocket() {
	if (m_pClientSocket != nullptr) {
		GetClientManager()->DeleteClientSocket(m_pClientSocket);
	}
	if (m_pListSocket != nullptr) {
		GetClientManager()->DeleteClientSocket(m_pListSocket);
	}
	for (int i = 0; i < m_vDataSocket.size(); i++) {
		if (m_vDataSocket[i].pClientSocket != nullptr) {
			GetClientManager()->DeleteClientSocket(m_vDataSocket[i].pClientSocket);
		}
	}

	SetEvent(m_hExitThreadEvent);

	if (m_hExitThreadEvent != NULL) {
		CloseHandle(m_hExitThreadEvent);
		m_hExitThreadEvent = NULL;
	}
}


VOID CFileTransfer::OnReceiveWithDec(ITcpServer* pSender, CONNID dwConnID, QWORD qwClientToken, WORD wServiceType, WORD wSocketType, int iCommandID, DWORD dwParentSocketTag, MyBuffer mBuffer) {
	switch (iCommandID) {
	case LIST_HARD_DISKS_C2S:
		DisplayHardDisk(dwConnID, qwClientToken, wServiceType, wSocketType, iCommandID, mBuffer);
		break;

	case LIST_FILES_C2S:
		DisplayFiles(dwConnID, qwClientToken, wServiceType, wSocketType, iCommandID, mBuffer);
		break;

	case LIST_FILES_FAILED_C2S: {
		_ListFilesFailed_C2S mData = MsgUnpack<_ListFilesFailed_C2S>(mBuffer.ptr(), mBuffer.size());
		MessageBoxA(0, mData.sPath.c_str(), mData.sError.c_str(), 0);
		break;
	}

	case FILE_DOWNLOAD_CMD_C2S: {
		BeforeRecvFileData(dwConnID, mBuffer);
		break;
	}

	case FILE_DOWNLOAD_C2S: {
		RecvFileData(dwConnID, qwClientToken, wServiceType, wSocketType, iCommandID, mBuffer);
		break;
	}

	case FILE_DOWNLOAD_FAILED_C2S: {
		FileDownloadFailed(dwConnID, mBuffer);
		break;
	}

	case FILE_UPLOAD_CMD_C2S: {
		BeforeFileUpload(dwConnID, mBuffer);
		break;
	}

	case FILE_UPLOAD_C2S: {
		OnRecv_FileUpload_C2S(dwConnID, mBuffer);
		break;
	}

	case FILE_UPLOAD_FAILED_C2S: {
		break;
	}

	}
}


// 用于列出文件夹的Socket创建好了
VOID CFileTransfer::CreateListSocketSuccess(CClientSocket* pClientSocket) {
	//m_pListObject = new CFileTransfer(pClientSocket, m_bWithDialog);
	m_pListSocket = pClientSocket;

	// 向client发送 列出盘符 命令
	GetServerSocket()->SendWithEnc(m_pListSocket->m_dwConnID, LIST_HARD_DISKS_S2C);
}


// 用于传输文件的Socket创建好了
VOID CFileTransfer::CreateFileSocketSuccess(CClientSocket* pClientSocket) {
	if (m_vDataSocket.size() >= FILE_CONCURRENT_DOWNLOAD_NUMBER) {
		DebugPrint("[ERROR3] 创建的并发DataSocket过多，断开此Socket连接\n");
		GetServerSocket()->Disconnect(pClientSocket->m_dwConnID);
	}
	else {
		_DataSocket DataSocket;
		DataSocket.pClientSocket = pClientSocket;
		DataSocket.bOnTransfering = FALSE;
		InitializeCriticalSection(&DataSocket.WriteCS);
		m_vDataSocket.push_back(DataSocket);
	}
}


// 获取空闲可用的DataSocket
_DataSocket* CFileTransfer::GetFreeDataSocket() {
	int iIndex = -1;
	for (int i = 0; i < m_vDataSocket.size(); i++) {
		if (!m_vDataSocket[i].bOnTransfering) {
			iIndex = i;
			break;
		}
	}
	return (iIndex == -1) ? (nullptr) : (&m_vDataSocket[iIndex]);
}


// 将接收到的硬盘信息更新到文件列表中
VOID CFileTransfer::DisplayHardDisk(CONNID dwConnID, QWORD qwClientToken, WORD wServiceType, WORD wSocketType, int iCommandID, MyBuffer mBuffer) {
	_ListHardDisk_WIN_C2S mData = MsgUnpack<_ListHardDisk_WIN_C2S>((PBYTE)mBuffer.ptr(), mBuffer.size());
	
	// 防止伪造攻击
	if (mData.dwHardDiskNum > 0xff) {
		return;
	}

	vector<_HardDiskUnit_WIN_C2S> vHardDiskUnits = MsgGetUnitsFromBuf<_HardDiskUnit_WIN_C2S>((PBYTE)mData.msData.ptr, mData.dwHardDiskNum);

	if (m_bWithDialog) {
		AddHardLists2List(vHardDiskUnits);
	}
}


// 将接收到的文件、文件夹信息更新到文件列表中
VOID CFileTransfer::DisplayFiles(CONNID dwConnID, QWORD qwClientToken, WORD wServiceType, WORD wSocketType, int iCommandID, MyBuffer mBuffer) {
	_ListFiles_WIN_C2S mData = MsgUnpack<_ListFiles_WIN_C2S>((PBYTE)mBuffer.ptr(), mBuffer.size());

	vector<_FileUnit_WIN_C2S> vFileUnits = MsgGetUnitsFromBuf<_FileUnit_WIN_C2S>((PBYTE)mData.msData.ptr, mData.dwFileNum);

	AddFiles2List(vFileUnits, mData.sPath, (mData.dwSectionNum == 1 || mData.dwSectionIndex == 1));
}


// 向client发出 列出目录下的文件 命令
VOID CFileTransfer::SendListFilesCmd(PWCHAR pwszRemotePath) {
	_ListFiles_S2C mData;
	mData.sPath = Wchars2Str(pwszRemotePath);

	MyBuffer mBuffer = MsgPack<_ListFiles_S2C>(mData);

	GetServerSocket()->SendWithEnc(m_pListSocket->m_dwConnID, LIST_FILES_S2C, mBuffer.ptr(), mBuffer.size());
}


// 添加文件传输任务
VOID CFileTransfer::AddTransferFileTask(FILE_TRANSFER_TYPE iTransferType, PWCHAR wszRemotePath, PWCHAR wszLocalPath) {
	string sFileName;
	string sRemotePath = Wchars2Str(wszRemotePath);
	string sLocalPath = Wchars2Str(wszLocalPath);

	if (iTransferType == DOWNLOAD) {
		string::size_type iPos = sRemotePath.find_last_of('\\') + 1;
		sFileName = sRemotePath.substr(iPos, sRemotePath.length() - iPos);
	}
	else {
		string::size_type iPos = sLocalPath.find_last_of('\\') + 1;
		sFileName = sLocalPath.substr(iPos, sLocalPath.length() - iPos);
	}
	
	m_dwGlobalFileTag++;
	_TransferFileTaskInfo info(m_dwGlobalFileTag, iTransferType, sFileName, sRemotePath, sLocalPath);
	m_vFileTaskOnWaiting.push_back(info);

	AddTask2List(info);
}


// 添加文件夹传输任务
VOID CFileTransfer::AddTransferFolderTask(FILE_TRANSFER_TYPE iTransferType, PWCHAR wszRemotePath, PWCHAR wszLocalPath) {
	;
}


_DataSocket* CFileTransfer::SearchDataSocket(CONNID dwConnID) {
	for (int i = 0; i < m_vDataSocket.size(); i++) {
		if (m_vDataSocket[i].pClientSocket->m_dwConnID == dwConnID) {
			return &m_vDataSocket[i];
		}
	}
	return nullptr;
}


// 文件传输完成后，从m_vFileTaskOnStarting和UI列表中删除这个文件相关的信息
VOID CFileTransfer::DeleteFileInfoAfterTransferOver(_DataSocket* pDataSocket) {
	int iIndex = -1;
	for (int i = 0; i < m_vFileTaskOnStarting.size(); i++) {
		if (pDataSocket->FileTransferInfo.dwFileTag == m_vFileTaskOnStarting[i].dwFileTag) {
			iIndex = i;
			break;
		}
	}
	ASSERT(iIndex != -1);
	m_vFileTaskOnStarting.erase(m_vFileTaskOnStarting.begin() + iIndex);

	RemoteFileTaskFromList(pDataSocket->FileTransferInfo.dwFileTag);

	pDataSocket->bOnTransfering = FALSE;
	pDataSocket->FileTransferInfo = _TransferFileTaskInfo();
}


VOID CFileTransfer::BeforeRecvFileData(CONNID dwConnID, MyBuffer mBuffer) {
	_FileDownloadCmd_C2S mData = MsgUnpack<_FileDownloadCmd_C2S>((PBYTE)mBuffer.ptr(), mBuffer.size());
	
	if (mData.bOpenFile) {

		// 通过CONNID获取对应的DataSocket
		_DataSocket* pDataSocket = SearchDataSocket(dwConnID);
		ASSERT(pDataSocket != nullptr);
		ASSERT(pDataSocket->FileTransferInfo.dwFileTag == mData.dwFileTag);

		HANDLE hFile = CreateFileA(pDataSocket->FileTransferInfo.sLocalPath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE) {
			MessageBox(0, L"文件句柄创建失败", L"文件句柄创建失败", 0);
			// TODO 向Client发送取消此文件传输的命令
			return;
		}

		pDataSocket->FileTransferInfo.hFile = hFile;
		pDataSocket->FileTransferInfo.qwFileSize = mData.qwFileSize;
		pDataSocket->FileTransferInfo.dwSectionNum = mData.dwSectionNum;
		// TODO check dwSectionNum from client

		// 知道client的文件的大小后，再次向client发送_FileDownloadCmd_S2C
		_FileDownloadCmd_S2C mData2;
		mData2.dwFileTag = mData.dwFileTag;
		mData2.sClientPath = "";
		mData2.bHaveFileSize = TRUE;
		MyBuffer mBuffer = MsgPack<_FileDownloadCmd_S2C>(mData2);
		GetServerSocket()->SendWithEnc(pDataSocket->pClientSocket->m_dwConnID, FILE_DOWNLOAD_CMD_S2C, mBuffer.ptr(), mBuffer.size());
	}
}


VOID CFileTransfer::RecvFileData(CONNID dwConnID, QWORD qwClientToken, WORD wServiceType, WORD wSocketType, int iCommandID, MyBuffer mBuffer) {
	USES_CONVERSION;
	_FileDownload_C2S mData = MsgUnpack<_FileDownload_C2S>((PBYTE)mBuffer.ptr(), mBuffer.size());

	// 通过CONNID获取对应的DataSocket
	_DataSocket* pDataSocket = SearchDataSocket(dwConnID);
	ASSERT(pDataSocket != nullptr);

	if (! (pDataSocket->FileTransferInfo.dwLastIndex + 1 == mData.dwSectionIndex ||
		pDataSocket->FileTransferInfo.dwLastIndex + 1 == pDataSocket->FileTransferInfo.dwSectionNum) ) {
		DebugPrint("[ERROR5] 文件分片失序，请判断是否为客户端伪造非法文件分片");
		return;
	}
	
	EnterCriticalSection(&pDataSocket->WriteCS);			// 加锁
	DWORD dwBytesWritten = 0;
	BOOL bRet = WriteFile(pDataSocket->FileTransferInfo.hFile, mData.msData.ptr, mData.msData.size, &dwBytesWritten, NULL);
	if (!bRet) {
		MessageBox(0, L"写入失败", L"写入失败", 0);
	}
	else {
		pDataSocket->FileTransferInfo.dwLastIndex++;
	}
	LeaveCriticalSection(&pDataSocket->WriteCS);			// 解锁

	// 告知客户端，本服务端已接收到的文件切片的index
	_FileUpload_C2S mData2;
	mData2.dwFileTag = pDataSocket->FileTransferInfo.dwFileTag;
	mData2.dwLastIndex = pDataSocket->FileTransferInfo.dwLastIndex;
	MyBuffer mBuffer2 = MsgPack<_FileUpload_C2S>(mData2);
	GetServerSocket()->SendWithEnc(dwConnID, FILE_DOWNLOAD_S2C, mBuffer2.ptr(), mBuffer2.size());

	// 在UI中更新进度
	DWORD dwSectionIndex = (mData.dwSectionIndex == 0) ? (pDataSocket->FileTransferInfo.dwSectionNum) : (mData.dwSectionIndex);
	UpdateFileDownloadInfo(pDataSocket->FileTransferInfo);

	// 最后一个文件切片，传输完关闭文件句柄
	if (mData.dwSectionIndex == 0) {
		CloseHandle(pDataSocket->FileTransferInfo.hFile);
		DeleteFileInfoAfterTransferOver(pDataSocket);
	}
}


// 从传输列表中获取要传输的文件信息，并向client发送文件传输命令
VOID WINAPI CFileTransfer::SendTransferCmdThreadFunc(LPVOID lParam) {
	CFileTransfer* pThis = (CFileTransfer*)lParam;

	while (TRUE) {

		// 触发关闭事件时跳出循环，结束线程。
		if (WAIT_OBJECT_0 == WaitForSingleObject(pThis->m_hExitThreadEvent, 0)) {
			break;
		}

		// 正在传输的任务数小于预设的文件传输的并发值，且存在正在等待中的文件传输任务
		if (pThis->m_vFileTaskOnStarting.size() < FILE_CONCURRENT_DOWNLOAD_NUMBER && 
			pThis->m_vFileTaskOnWaiting.size() > 0) {

			_TransferFileTaskInfo FileInfo = pThis->m_vFileTaskOnWaiting[0];
			pThis->m_vFileTaskOnWaiting.erase(pThis->m_vFileTaskOnWaiting.begin());
			pThis->m_vFileTaskOnStarting.push_back(FileInfo);
			
			_DataSocket* pDataSocket = pThis->GetFreeDataSocket();
			ASSERT(pDataSocket != nullptr);

			// 下载
			if (FileInfo.iTransferType == DOWNLOAD) {
				_FileDownloadCmd_S2C mData;
				mData.dwFileTag = FileInfo.dwFileTag;
				mData.sClientPath = FileInfo.sRemotePath;
				mData.bHaveFileSize = FALSE;
				MyBuffer mBuffer = MsgPack<_FileDownloadCmd_S2C>(mData);
				BOOL bRet = pThis->GetServerSocket()->SendWithEnc(pDataSocket->pClientSocket->m_dwConnID, FILE_DOWNLOAD_CMD_S2C, mBuffer.ptr(), mBuffer.size());

				if (bRet) {
					pDataSocket->bOnTransfering = TRUE;
					FileInfo.qwStartTime = GetCurrentTimeStamp();
					pDataSocket->FileTransferInfo = FileInfo;
				}
			}

			// 上传
			else {

				// 打开文件
				HANDLE hFile = CreateFileA(FileInfo.sLocalPath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

				// 打开失败
				if (hFile == INVALID_HANDLE_VALUE) {
					// TODO
					return;
				}

				// 获取文件大小
				QWORD qwFileSize = 0;
				DWORD dwFileSizeLowDword = 0;
				DWORD dwFileSizeHighDword = 0;
				dwFileSizeLowDword = GetFileSize(hFile, &dwFileSizeHighDword);
				qwFileSize = (((QWORD)dwFileSizeHighDword) << 32) + dwFileSizeLowDword;


				_FileUploadCmd_S2C mData;
				mData.dwFileTag = FileInfo.dwFileTag;
				mData.sClientPath = FileInfo.sRemotePath;
				mData.qwFileSize = qwFileSize;
				mData.dwSectionNum = (qwFileSize % FILE_SECTION_MAX_LENGTH) ? (qwFileSize / FILE_SECTION_MAX_LENGTH + 1) : (qwFileSize / FILE_SECTION_MAX_LENGTH);

				MyBuffer mBuffer = MsgPack<_FileUploadCmd_S2C>(mData);
				BOOL bRet = pThis->GetServerSocket()->SendWithEnc(pDataSocket->pClientSocket->m_dwConnID, FILE_UPLOAD_CMD_S2C, mBuffer.ptr(), mBuffer.size());

				if (bRet) {
					pDataSocket->bOnTransfering = TRUE;
					FileInfo.hFile = hFile;
					FileInfo.qwFileSize = qwFileSize;
					FileInfo.dwSectionNum = mData.dwSectionNum;
					FileInfo.qwStartTime = GetCurrentTimeStamp();
					pDataSocket->FileTransferInfo = FileInfo;
				}
			}
		}
	}
}


VOID CFileTransfer::BeforeFileUpload(CONNID dwConnID, MyBuffer mBuffer) {
	USES_CONVERSION;
	_FileUploadCmd_C2S mData = MsgUnpack<_FileUploadCmd_C2S>((PBYTE)mBuffer.ptr(), mBuffer.size());

	_DataSocket* pDataSocket = SearchDataSocket(dwConnID);
	ASSERT(pDataSocket != nullptr);

	if (!mData.bOpenFile) {
		// TODO
		//MessageBoxA(0, mData.sClientPath.c_str(), "文件创建失败", 0);
		DeleteFileInfoAfterTransferOver(pDataSocket);
		return;
	}

	SendFileData(pDataSocket);
}


VOID CFileTransfer::SendFileData(_DataSocket* pDataSocket) {
	_TransferFileTaskInfo& FileInfo = pDataSocket->FileTransferInfo;
	HANDLE hFile = FileInfo.hFile;
	QWORD qwFileSize = FileInfo.qwFileSize;
	DWORD dwFileSectionNum = FileInfo.dwSectionNum;
	DWORD dwFileSectionIndex = FileInfo.dwLastIndex + 1;

	auto mFileBuffer = MyBuffer(FILE_SECTION_MAX_LENGTH);
	DWORD dwBytesReadTemp = 0;

	// 不是最后一个分片
	if (dwFileSectionIndex != dwFileSectionNum) {
		ReadFile(hFile, mFileBuffer.ptr(), FILE_SECTION_MAX_LENGTH, &dwBytesReadTemp, NULL);
	}
	// 最后一个分片
	else {
		DWORD dwReadBytes = qwFileSize % FILE_SECTION_MAX_LENGTH ? qwFileSize % FILE_SECTION_MAX_LENGTH : FILE_SECTION_MAX_LENGTH;
		ReadFile(hFile, mFileBuffer.ptr(), dwReadBytes, &dwBytesReadTemp, NULL);
	}

	_FileDownload_C2S mData;
	mData.dwFileTag = pDataSocket->FileTransferInfo.dwFileTag;
	mData.dwSectionIndex = (dwFileSectionIndex == dwFileSectionNum) ? (0) : (dwFileSectionIndex);
	mData.msData.ptr = (PCHAR)mFileBuffer.ptr();
	mData.msData.size = dwBytesReadTemp;

	dwBytesReadTemp = 0;

	MyBuffer mBuffer = MsgPack<_FileDownload_C2S>(mData);
	GetServerSocket()->SendWithEnc(pDataSocket->pClientSocket->m_dwConnID, FILE_UPLOAD_S2C, mBuffer.ptr(), mBuffer.size());
}


VOID CFileTransfer::OnRecv_FileUpload_C2S(DWORD dwConnID, MyBuffer mBuffer) {
	USES_CONVERSION;
	_FileUpload_C2S mData = MsgUnpack<_FileUpload_C2S>((PBYTE)mBuffer.ptr(), mBuffer.size());

	// 通过CONNID获取对应的DataSocket
	_DataSocket* pDataSocket = SearchDataSocket(dwConnID);
	ASSERT(pDataSocket != nullptr);

	if (!(pDataSocket->FileTransferInfo.dwLastIndex + 1 == mData.dwLastIndex ||
		pDataSocket->FileTransferInfo.dwLastIndex + 1 == pDataSocket->FileTransferInfo.dwSectionNum)) {
		DebugPrint("[ERROR6] 文件分片失序，请判断是否为客户端伪造非法文件分片");
		return;
	}

	pDataSocket->FileTransferInfo.dwLastIndex++;

	// 在UI中更新进度
	UpdateFileUploadInfo(pDataSocket->FileTransferInfo);

	// 最后一个文件切片，传输完关闭文件句柄
	if (pDataSocket->FileTransferInfo.dwLastIndex == pDataSocket->FileTransferInfo.dwSectionNum) {
		CloseHandle(pDataSocket->FileTransferInfo.hFile);
		DeleteFileInfoAfterTransferOver(pDataSocket);
	}
	// 不是最后一个文件切片，则传输下一个文件切片
	else {
		SendFileData(pDataSocket);
	}
}



// 虚函数，需要FileTransferDlg重载

VOID CFileTransfer::AddHardLists2List(vector<nsFileTransfer::_HardDiskUnit_WIN_C2S>& vHardDiskUnits) {
	return;
}

VOID CFileTransfer::AddFiles2List(vector<nsFileTransfer::_FileUnit_WIN_C2S>& vFileUnits, string sPath, BOOL bFirst) {
	return;
}

VOID CFileTransfer::AddTask2List(_TransferFileTaskInfo info) {
	return;
}

// 从UI中删除已完成的这个文件传输任务（如果继承子类含有对话框的话需要重载此函数
VOID CFileTransfer::RemoteFileTaskFromList(DWORD dwFileTag) {
	;
}

VOID CFileTransfer::UpdateFileDownloadInfo(_TransferFileTaskInfo FileInfo) {
	;
}

VOID CFileTransfer::FileDownloadFailed(DWORD dwConnID, MyBuffer mBuffer) {
	USES_CONVERSION;
	_FileDownloadFailed_C2S mData = MsgUnpack<_FileDownloadFailed_C2S>((PBYTE)mBuffer.ptr(), mBuffer.size());
	MessageBox(0, A2W(mData.sClientPath.c_str()), A2W(mData.sErrorInfo.c_str()), 0);

	_DataSocket* pDataSocket = SearchDataSocket(dwConnID);
	ASSERT(pDataSocket != nullptr);

	DeleteFileInfoAfterTransferOver(pDataSocket);
}


VOID CFileTransfer::UpdateFileUploadInfo(_TransferFileTaskInfo FileInfo) {
	;
}