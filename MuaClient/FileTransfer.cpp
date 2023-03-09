#include "pch.h"
#include "FileTransfer.h"

using namespace nsFileTransfer;


CFileTransfer::CFileTransfer(TCP_MODE iTcpMode, WORD wSocketType, QWORD qwClientToken, DWORD dwParentSocketTag/* = 0 */) : CSocket(iTcpMode, nsGeneralSocket::FILE_TRANSFER_SERVICE, wSocketType, qwClientToken) {
	m_qwClientToken = qwClientToken;
    m_dwParentSocketTag = dwParentSocketTag;
}


CFileTransfer::~CFileTransfer(){
	;
}


VOID CFileTransfer::OnReceiveWithDec(ITcpClient* pSender, CONNID dwConnID, QWORD qwClientToken, WORD wServiceType, WORD wSocketType, int iCommandID, DWORD dwParentSocketTag, MyBuffer mBuffer)
{
	DebugPrintRecvParams(dwConnID, qwClientToken, wServiceType, wSocketType, iCommandID, dwParentSocketTag, mBuffer);

	switch (iCommandID) {
	case nsGeneralSocket::CHANNEL_SUCCESS_S2C:
		;
		break;

    case LIST_HARD_DISKS_S2C: {
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ListHardDisks, (LPVOID)this, 0, NULL);
        break;
    }

    case LIST_FILES_S2C: {
        _ListFiles_S2C mData = MsgUnpack<_ListFiles_S2C>(mBuffer.ptr(), mBuffer.size());
        _LIST_FILES_THREAD_PARAM* pThreadParam = new _LIST_FILES_THREAD_PARAM(this, mData.sPath);
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ListFiles, (LPVOID)pThreadParam, 0, NULL);
        break;
    }

    case FILE_DOWNLOAD_CMD_S2C: {
        _FileDownloadCmd_S2C mData = MsgUnpack<_FileDownloadCmd_S2C>(mBuffer.ptr(), mBuffer.size());
        _SEND_FILE_DATA_THREAD_PARAM* pThreadParam = new _SEND_FILE_DATA_THREAD_PARAM(this, mData.dwFileTag, mData.sClientPath);
        if (mData.bHaveFileSize) {    
            CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)SendFileData, (LPVOID)pThreadParam, 0, NULL);
        }
        else {
            CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)BeforeSendFileData, (LPVOID)pThreadParam, 0, NULL);
        }
        break;
    }

    case FILE_DOWNLOAD_S2C: {
        _FileDownload_S2C mData = MsgUnpack<_FileDownload_S2C>(mBuffer.ptr(), mBuffer.size());
        _SEND_FILE_DATA_THREAD_PARAM* pThreadParam = new _SEND_FILE_DATA_THREAD_PARAM(this, mData.dwFileTag, "");
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)SendFileData, (LPVOID)pThreadParam, 0, NULL);
        break;
    }

    case FILE_UPLOAD_CMD_S2C: {
        _FileUploadCmd_S2C mData = MsgUnpack<_FileUploadCmd_S2C>(mBuffer.ptr(), mBuffer.size());
        _RECV_FILE_DATA_THREAD_PARAM* pThreadParam = new _RECV_FILE_DATA_THREAD_PARAM(this, mData.dwFileTag, mData.sClientPath, mData.qwFileSize, mData.dwSectionNum);
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)BeforeRecvFileData, (LPVOID)pThreadParam, 0, NULL);
        break;
    }

    case FILE_UPLOAD_S2C: {
        //_FileUpload_S2C mData = MsgUnpack<_FileUpload_S2C>(mBuffer.ptr(), mBuffer.size());
        //_SEND_FILE_DATA_THREAD_PARAM* pThreadParam = new _SEND_FILE_DATA_THREAD_PARAM(this, mData.dwFileTag, mData.sClientPath);
        //CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)SendFileData, (LPVOID)pThreadParam, 0, NULL);
        RecvFileData(mBuffer);
        break;
    }


	}
}


EnHandleResult CFileTransfer::OnClose(ITcpClient* pSender, CONNID dwConnID, EnSocketOperation enOperation, int iErrorCode)
{
	DebugPrint("[Client %d] OnClose: \n", dwConnID);

	delete this;

	return HR_OK;
}




// **************************************** 列出盘符 ****************************************

struct _DriverUnit {
    string              sDriverName     = "";               // 盘符
    string              sDriverLable    = "";               // 标签
    BOOL                bIsHardDisk     = FALSE;            // 该Driver是否为硬盘
    QWORD               qwTotalBytes    = 0;                // 总空间大小
    QWORD               qwFreeBytes     = 0;                // 剩余空间大小
};


VOID CFileTransfer::ListHardDisks(LPVOID lParam) {
	CFileTransfer* pThis = (CFileTransfer*)lParam;

    // 获取盘符字符串
    DWORD dwStrLen = GetLogicalDriveStrings(0, NULL);
    MyBuffer mDriverBuffer = MyBuffer(dwStrLen);
    GetLogicalDriveStringsA(dwStrLen, (LPSTR)mDriverBuffer.ptr());
    PCHAR pszDriver = (PCHAR)mDriverBuffer.ptr();

    vector<_DriverUnit> vDriver;
    DWORD dwHardDiskNum = 0;

    while (*pszDriver != '\0')
    {
        _DriverUnit DriverData;
        DriverData.sDriverName = pszDriver;

        int iDriverType;
        BOOL bResult;
        QWORD qwFreeBytesToCaller;
        QWORD qwTotalBytes;
        QWORD qwFreeBytes;

        // 获取驱动器类型
        iDriverType = GetDriveTypeA(pszDriver);

        if (iDriverType == DRIVE_FIXED) {
            DriverData.bIsHardDisk = TRUE;
            dwHardDiskNum++;

            // 获取硬盘空间
            bResult = GetDiskFreeSpaceExA(pszDriver, (PULARGE_INTEGER)&qwFreeBytesToCaller, (PULARGE_INTEGER)&qwTotalBytes, (PULARGE_INTEGER)&qwFreeBytes);

            DriverData.qwTotalBytes = qwTotalBytes;
            DriverData.qwFreeBytes = qwFreeBytes;

            // 获取硬盘Label（就是通俗意义上的的硬盘名称）
            CHAR szLabel[MAX_PATH];
            DWORD dwDontNeedThis;
            if (!GetVolumeInformationA(pszDriver, szLabel, sizeof(szLabel), NULL, &dwDontNeedThis, &dwDontNeedThis, NULL, 0)) {
                DriverData.sDriverLable = "";
            }
            else {
                DriverData.sDriverLable = szLabel;
            }
        }

        vDriver.push_back(DriverData);
        pszDriver += strlen(pszDriver) + 1;			    //定位到下一个字符串.加一是为了跳过'\0'字符串.
    }
    
    vector<_PackedUnit> vPackedUnits;
    
    for (int i = 0; i < vDriver.size(); i++)
    {
        if (vDriver[i].bIsHardDisk) {

            _HardDiskUnit_WIN_C2S mData;
            mData.sHardDiskName = vDriver[i].sDriverName;
            mData.sHardDiskLabel = vDriver[i].sDriverLable;
            mData.qwTotalBytes = vDriver[i].qwTotalBytes;
            mData.qwFreeBytes = vDriver[i].qwFreeBytes;

            MsgAddUnitToVec<_HardDiskUnit_WIN_C2S>(vPackedUnits, mData);
        }
    }

    PBYTE pBuffer = nullptr;
    DWORD dwBufLen = MsgMergeUnitsToBuf<_HardDiskUnit_WIN_C2S>(vPackedUnits, pBuffer);

    // 向server发送盘符信息
    _ListHardDisk_WIN_C2S mData;
    mData.dwHardDiskNum = dwHardDiskNum;
    mData.msData.ptr = (PCHAR)pBuffer;
    mData.msData.size = dwBufLen;

    MyBuffer mFinalBuffer = MsgPack<_ListHardDisk_WIN_C2S>(mData);
    delete[] pBuffer;

    pThis->SendWithEnc(LIST_HARD_DISKS_C2S, mFinalBuffer.ptr(), mFinalBuffer.size());
}


// **************************************** 列出文件 ****************************************

VOID CFileTransfer::ListFiles(LPVOID lParam) {
    _LIST_FILES_THREAD_PARAM* pThreadParam = (_LIST_FILES_THREAD_PARAM*)lParam;
    CFileTransfer* pThis = pThreadParam->m_pThis;
    string sPath = pThreadParam->m_sPath;
    delete pThreadParam;

    vector<_PackedUnit> vPackedUnits;

    char szSearchFilter[MAX_PATH];
    WIN32_FIND_DATAA FileData = { 0 };

    // 构造搜索文件类型字符串, *.*表示搜索所有文件类型
    sprintf_s(szSearchFilter, "%s\\*.*", sPath.c_str());

    // 搜索第一个文件
    HANDLE hFile = ::FindFirstFileA(szSearchFilter, &FileData);
    if (INVALID_HANDLE_VALUE != hFile)
    {
        do
        {
            PCHAR pszFileName = FileData.cFileName;

            if (strcmp(pszFileName, ".") == 0 || strcmp(pszFileName, "..") == 0) {
                continue;
            }

            QWORD qwFileBytes = ((QWORD)FileData.nFileSizeHigh << 32) + FileData.nFileSizeLow;
            FILE_TYPE iFileType = TYPE_UNKNOWN;
            CHAR szLastWriteTime[64] = { 0 };

            // 修改时间
            SYSTEMTIME sysTime;
            FileTimeToSystemTime(&FileData.ftLastWriteTime, &sysTime);
            sprintf_s(szLastWriteTime, "%02d-%02d-%02d %02d:%02d:%02d", sysTime.wYear, sysTime.wMonth, sysTime.wDay, sysTime.wHour, sysTime.wMinute, sysTime.wSecond);

            // 是目录
            if (FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                iFileType = TYPE_FOLDER;
            }
            // 其他的属性先默认是文件，https://learn.microsoft.com/zh-cn/windows/win32/fileio/file-attribute-constants
            else {
                iFileType = TYPE_FILE;
            }

            // 添加到vPackedUnits中
            _FileUnit_WIN_C2S mData;
            mData.iFileType = iFileType;
            mData.sFileName = pszFileName;
            mData.qwFileBytes = qwFileBytes;
            mData.sModifiedDate = szLastWriteTime;
            
            MsgAddUnitToVec<_FileUnit_WIN_C2S>(vPackedUnits, mData);

            // 搜索下一个文件
        } while (::FindNextFileA(hFile, &FileData));
    }
    else {
        _ListFilesFailed_C2S mData;
        mData.sPath = sPath;
        mData.sError = "此目录的句柄打开失败。";

        MyBuffer mBuffer = MsgPack<_ListFilesFailed_C2S>(mData);
        pThis->SendWithEnc(LIST_FILES_FAILED_C2S, mBuffer.ptr(), mBuffer.size());
        return;
    }

    // 关闭文件句柄
    ::FindClose(hFile);


    if (vPackedUnits.size() == 0) {
        _ListFiles_WIN_C2S mData;
        mData.dwSectionIndex = 0;
        mData.dwSectionNum = 1;
        mData.dwFileNum = 0;
        mData.sPath = sPath;
        mData.msData.ptr = NULL;
        mData.msData.size = 0;

        MyBuffer mBuffer = MsgPack<_ListFiles_WIN_C2S>(mData);
        pThis->SendWithEnc(LIST_FILES_C2S, mBuffer.ptr(), mBuffer.size());
        return;
    }

    // 计算 本目录下的文件信息 需要 分多少次传输
    DWORD dwSectionNumber = (vPackedUnits.size() % FILES_NUM_PER_GROUP) ? (vPackedUnits.size() / FILES_NUM_PER_GROUP + 1) : (vPackedUnits.size() / FILES_NUM_PER_GROUP);

    for (int i = 1; i <= dwSectionNumber; i++) {
        PBYTE pBuffer = nullptr;
        
        int iStart = (i - 1) * FILES_NUM_PER_GROUP;
        int iEnd = (i == dwSectionNumber) ? (vPackedUnits.size()) : (i * FILES_NUM_PER_GROUP);

        DWORD dwBufLen = MsgMergeUnitsToBuf<_FileUnit_WIN_C2S>(vPackedUnits, pBuffer, iStart, iEnd);
        
        // 向server发送文件列表
        _ListFiles_WIN_C2S mData;
        mData.dwSectionIndex = (i == dwSectionNumber) ? (0) : (i);
        mData.dwSectionNum = dwSectionNumber;
        mData.dwFileNum = iEnd - iStart;
        mData.sPath = sPath;
        mData.msData.ptr = (PCHAR)pBuffer;
        mData.msData.size = dwBufLen;

        MyBuffer mFinalBuffer = MsgPack<_ListFiles_WIN_C2S>(mData);
        delete[] pBuffer;

        pThis->SendWithEnc(LIST_FILES_C2S, mFinalBuffer.ptr(), mFinalBuffer.size());
    }
}


// **************************************** 发送文件 ****************************************

VOID CFileTransfer::BeforeSendFileData(LPVOID lParam) {
    _SEND_FILE_DATA_THREAD_PARAM* pThreadParam = (_SEND_FILE_DATA_THREAD_PARAM*)lParam;
    CFileTransfer* pThis = pThreadParam->pThis;
    DWORD dwFileTag = pThreadParam->dwFileTag;
    string sClientPath = pThreadParam->sClientPath;
    delete pThreadParam;

    // 打开文件
    HANDLE hFile = CreateFileA(sClientPath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    // 打开失败
    if (hFile == INVALID_HANDLE_VALUE) {
        _FileDownloadCmd_C2S mData;
        mData.dwFileTag = dwFileTag;
        mData.bOpenFile = FALSE;
        mData.qwFileSize = 0;
        mData.dwSectionNum = 0;

        PBYTE pBuffer = nullptr;
        MyBuffer mBuffer = MsgPack<_FileDownloadCmd_C2S>(mData);
        pThis->SendWithEnc(FILE_DOWNLOAD_CMD_C2S, mBuffer.ptr(), mBuffer.size());
        delete[] pBuffer;
        return;
    }

    // 获取文件大小，并计算切片数
    QWORD qwFileSize = 0;
    DWORD dwFileSizeLowDword = 0;
    DWORD dwFileSizeHighDword = 0;
    dwFileSizeLowDword = GetFileSize(hFile, &dwFileSizeHighDword);
    qwFileSize = (((QWORD)dwFileSizeHighDword) << 32) + dwFileSizeLowDword;
    DWORD dwSectionNum = (qwFileSize % FILE_SECTION_MAX_LENGTH) ? (qwFileSize / FILE_SECTION_MAX_LENGTH + 1) : (qwFileSize / FILE_SECTION_MAX_LENGTH);

    // 文件相关信息 存入m_vFileTask中
    _FileInfo FileInfo(dwFileTag, sClientPath, hFile, qwFileSize, dwSectionNum);
    pThis->m_vFileTask.push_back(FileInfo);
    
    // 组装_FileDownloadCmd_C2S
    _FileDownloadCmd_C2S mData;
    mData.dwFileTag = dwFileTag;
    mData.bOpenFile = TRUE;
    mData.qwFileSize = qwFileSize;
    mData.dwSectionNum = dwSectionNum;
    
    PBYTE pBuffer = nullptr;
    MyBuffer mBuffer = MsgPack<_FileDownloadCmd_C2S>(mData);
    pThis->SendWithEnc(FILE_DOWNLOAD_CMD_C2S, mBuffer.ptr(), mBuffer.size());
    delete[] pBuffer;
}


VOID CFileTransfer::SendFileData(LPVOID lParam) {
    _SEND_FILE_DATA_THREAD_PARAM* pThreadParam = (_SEND_FILE_DATA_THREAD_PARAM*)lParam;
    CFileTransfer* pThis = pThreadParam->pThis;
    DWORD dwFileTag = pThreadParam->dwFileTag;
    delete pThreadParam;

    // 根据FileTag从m_vFileTask中找到对应的FileInfo
    int iIndex = -1;
    for (int i = 0; i < pThis->m_vFileTask.size(); i++) {
        if (pThis->m_vFileTask[i].dwFileTag == dwFileTag) {
            iIndex = i;
            break;
        }
    }
    ASSERT(iIndex != -1);
    _FileInfo& FileInfo = pThis->m_vFileTask[iIndex];

    if (FileInfo.dwLastIndex == FileInfo.dwSectionNum) {
        return;
    }
    
    MyBuffer mFileBuffer = MyBuffer(FILE_SECTION_MAX_LENGTH);
    DWORD dwBytesReadTemp = 0;

    DWORD dwFileSectionIndex = FileInfo.dwLastIndex + 1;

    // 不是最后一个分片
    if (dwFileSectionIndex != FileInfo.dwSectionNum) {
        ReadFile(FileInfo.hFile, mFileBuffer.ptr(), FILE_SECTION_MAX_LENGTH, &dwBytesReadTemp, NULL);
    }
    // 最后一个分片
    else {
        DWORD dwReadBytes = FileInfo.qwFileSize % FILE_SECTION_MAX_LENGTH ? FileInfo.qwFileSize % FILE_SECTION_MAX_LENGTH : FILE_SECTION_MAX_LENGTH;
        ReadFile(FileInfo.hFile, mFileBuffer.ptr(), dwReadBytes, &dwBytesReadTemp, NULL);
    }
        
    _FileDownload_C2S mData;
    mData.dwFileTag = dwFileTag;
    mData.dwSectionIndex = (dwFileSectionIndex == FileInfo.dwSectionNum) ? (0) : (dwFileSectionIndex);
    mData.msData.ptr = (PCHAR)mFileBuffer.ptr();
    mData.msData.size = dwBytesReadTemp;

    dwBytesReadTemp = 0;

    MyBuffer mBuffer = MsgPack<_FileDownload_C2S>(mData);
    pThis->SendWithEnc(FILE_DOWNLOAD_C2S, mBuffer.ptr(), mBuffer.size());

    FileInfo.dwLastIndex++;
}


// **************************************** 接收文件 ****************************************

VOID CFileTransfer::BeforeRecvFileData(LPVOID lParam) {
    _RECV_FILE_DATA_THREAD_PARAM* pThreadParam = (_RECV_FILE_DATA_THREAD_PARAM*)lParam;
    CFileTransfer* pThis = pThreadParam->pThis;
    DWORD dwFileTag = pThreadParam->dwFileTag;
    string sClientPath = pThreadParam->sClientPath;
    QWORD qwFileSize = pThreadParam->qwFileSize;
    DWORD dwSectionNum = pThreadParam->dwSectionNum;    
    delete pThreadParam;

    HANDLE hFile = CreateFileA(sClientPath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    _FileUploadCmd_C2S mData;
    mData.dwFileTag = dwFileTag;
    mData.bOpenFile = (hFile == INVALID_HANDLE_VALUE) ? FALSE : TRUE;

    if (mData.bOpenFile) {
        _FileInfo FileInfo(dwFileTag, sClientPath, hFile, qwFileSize, dwSectionNum);
        pThis->m_vFileTask.push_back(FileInfo);
    }

    PBYTE pBuffer = nullptr;
    MyBuffer mBuffer = MsgPack<_FileUploadCmd_C2S>(mData);
    pThis->SendWithEnc(FILE_UPLOAD_CMD_C2S, mBuffer.ptr(), mBuffer.size());
    delete[] pBuffer;
}



VOID CFileTransfer::RecvFileData(MyBuffer mBuffer) {
    USES_CONVERSION;
    _FileDownload_C2S mData = MsgUnpack<_FileDownload_C2S>(mBuffer.ptr(), mBuffer.size());

    // 根据FileTag从m_vFileTask中找到对应的FileInfo
    int iIndex = -1;
    for (int i = 0; i < m_vFileTask.size(); i++) {
        if (m_vFileTask[i].dwFileTag == mData.dwFileTag) {
            iIndex = i;
            break;
        }
    }
    ASSERT(iIndex != -1);
    _FileInfo& FileInfo = m_vFileTask[iIndex];

    // 判断文件切片是否失序
    if (!(FileInfo.dwLastIndex + 1 == mData.dwSectionIndex ||
        FileInfo.dwLastIndex + 1 == FileInfo.dwSectionNum)) {
        DebugPrint("[ERROR5] 文件分片失序");
        return;
    }

    EnterCriticalSection(&FileInfo.WriteCS);			// 加锁
    DWORD dwBytesWritten = 0;
    BOOL bRet = WriteFile(FileInfo.hFile, mData.msData.ptr, mData.msData.size, &dwBytesWritten, NULL);
    if (!bRet) {
        _FileUploadFailed_C2S mData;
        mData.dwFileTag = FileInfo.dwFileTag;
        mData.sClientPath = FileInfo.sLocalPath;
        mData.sErrorInfo = "文件写入失败";
    }
    else {
        FileInfo.dwLastIndex++;
    }
    LeaveCriticalSection(&FileInfo.WriteCS);			// 解锁

    _FileUpload_C2S mData2;
    mData2.dwFileTag = FileInfo.dwFileTag;
    mData2.dwLastIndex = FileInfo.dwLastIndex;
    MyBuffer mBuffer2 = MsgPack<_FileUpload_C2S>(mData2);
    SendWithEnc(FILE_UPLOAD_C2S, mBuffer2.ptr(), mBuffer2.size());

    // 最后一个文件切片，传输完关闭文件句柄，并从m_vFileTask中删除对应FileInfo
    if (mData.dwSectionIndex == 0) {
        CloseHandle(FileInfo.hFile);
        
        int iIndex = -1;
        for (int i = 0; i < m_vFileTask.size(); i++) {
            if (FileInfo.dwFileTag == m_vFileTask[i].dwFileTag) {
                iIndex = i;
                break;
            }
        }
        ASSERT(iIndex != -1);
        m_vFileTask.erase(m_vFileTask.begin() + iIndex);
    }
}