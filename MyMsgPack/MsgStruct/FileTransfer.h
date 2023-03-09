#pragma once

#include "../Include/msgpack.hpp"
#include "../../MyCrypto/MyCrypto.h"

#ifndef QWORD
#define QWORD unsigned long long
#endif


namespace nsFileTransfer {

	#define FILES_NUM_PER_GROUP 100									// 一个目录下的文件信息要分n次传输，此值表示几个文件信息为一组

	#define FILE_SECTION_MAX_LENGTH (PACKET_MAX_LENGTH - 512)		// 文件的一个分片的大小

	#define FILE_CONCURRENT_DOWNLOAD_NUMBER	3						// 文件并发下载数量


	enum CHILD_SOCKET_TYPE {
		LIST_FILES_SOCKET = 0x0400,
		FILE_TRANSFER_SOCKET
	};


	// 文件类型
	enum FILE_TYPE {
		TYPE_UNKNOWN,						// 未知
		TYPE_POINT,							// 特指 . 和 ..
		TYPE_HARD_DISK,						// 硬盘，仅表示Windows系统的盘符
		TYPE_FOLDER,						// 文件夹
		TYPE_FILE,							// 文件
		TYPE_LINK,							// 链接
		// TODO: some type else
	};


	// 命令号
	enum COMMAND_ID {

		// **************** 主socket ****************

		FILE_TRANSFER_S2C = 0x04000000,
		FILE_TRANSFER_C2S,
		
		FILE_TRANSFER_WITHOUT_DIALOG_S2C,
		FILE_TRANSFER_WITHOUT_DIALOG_C2S,


		// ************** 列出硬盘/文件 **************

		CREATE_LIST_SOCKET_S2C,
		CREATE_LIST_SOCKET_C2S,
		
		LIST_HARD_DISKS_S2C,
		LIST_HARD_DISKS_C2S,

		LIST_FILES_S2C,
		LIST_FILES_C2S,
		LIST_FILES_FAILED_C2S,


		// ************** 文件上传下载 **************

		CREATE_FILE_SOCKET_S2C,
		CREATE_FILE_SOCKET_C2S,

		FILE_UPLOAD_CMD_S2C,					// 上传是指S2C
		FILE_UPLOAD_CMD_C2S,
		FILE_UPLOAD_C2S,
		FILE_UPLOAD_S2C,
		FILE_UPLOAD_FAILED_C2S,

		FILE_DOWNLOAD_CMD_S2C,					// 下载是指C2S
		FILE_DOWNLOAD_CMD_C2S,
		FILE_DOWNLOAD_S2C,
		FILE_DOWNLOAD_C2S,
		FILE_DOWNLOAD_FAILED_C2S,
	};


	// ***************************** 列出硬盘 *****************************
	struct _ListHardDisk_WIN_C2S {
		DWORD						dwHardDiskNum;
		msgpack::type::raw_ref		msData;				// 内含多个 _HardDiskUnit_WIN_C2S 数据
		MSGPACK_DEFINE(dwHardDiskNum, msData)
	};

	struct _HardDiskUnit_WIN_C2S {
		string						sHardDiskName;
		string						sHardDiskLabel;
		QWORD						qwTotalBytes;
		QWORD						qwFreeBytes;
		MSGPACK_DEFINE(sHardDiskName, sHardDiskLabel, qwTotalBytes, qwFreeBytes)
	};


	// ***************************** 列出文件 *****************************

	struct _ListFiles_S2C {
		string						sPath;
		MSGPACK_DEFINE(sPath)
	};

	struct _ListFiles_WIN_C2S {
		DWORD						dwSectionIndex;		// 每次列出的文件个数是有限的，故一个目录要封装多个_ListFiles_WIN_C2S。dwIndex从1开始自增，最后一个_ListFiles_WIN_C2S的dwIndex = 0
		DWORD						dwSectionNum;		// 总共要多少个_ListFiles_WIN_C2S
		DWORD						dwFileNum;
		string						sPath;
		msgpack::type::raw_ref		msData;				// 内含多个 _FileUnit_WIN_C2S 数据
		MSGPACK_DEFINE(dwSectionIndex, dwSectionNum, dwFileNum, sPath, msData)
	};

	struct _FileUnit_WIN_C2S {
		int							iFileType;
		string						sFileName;
		QWORD						qwFileBytes;
		string						sModifiedDate;
		MSGPACK_DEFINE(iFileType, sFileName, qwFileBytes, sModifiedDate)
	};


	struct _ListFilesFailed_C2S {
		string						sPath;
		string						sError;
		MSGPACK_DEFINE(sPath, sError)
	};


	// ***************************** 文件下载 *****************************

	struct _FileDownloadCmd_S2C {
		DWORD						dwFileTag;
		string						sClientPath;
		BOOL						bHaveFileSize;
		MSGPACK_DEFINE(dwFileTag, sClientPath, bHaveFileSize)
	};


	struct _FileDownloadCmd_C2S {
		DWORD						dwFileTag;
		BOOL						bOpenFile;
		QWORD						qwFileSize;
		DWORD						dwSectionNum;
		MSGPACK_DEFINE(dwFileTag, bOpenFile, qwFileSize, dwSectionNum)
	};

	
	struct _FileDownload_C2S {
		DWORD						dwFileTag;			// 每个文件传输任务都有唯一的tag用以标识
		DWORD						dwSectionIndex;		// dwIndex从1开始自增，最后一个_FileDownload_C2S的dwIndex = 0
		msgpack::type::raw_ref		msData;			
		MSGPACK_DEFINE(dwFileTag, dwSectionIndex, msData)
	};


	struct _FileDownload_S2C {
		DWORD						dwFileTag;
		DWORD						dwLastIndex;// 已接收的切片的最新index
		MSGPACK_DEFINE(dwFileTag, dwLastIndex)
	};


	struct _FileDownloadFailed_C2S {
		DWORD						dwFileTag;
		string						sClientPath;
		string						sErrorInfo;
		MSGPACK_DEFINE(dwFileTag, sClientPath, sErrorInfo)
	};


	struct _FileDownloadStop_C2S {
		DWORD						dwFileTag;
		MSGPACK_DEFINE(dwFileTag)
	};


	// ***************************** 文件上传 *****************************

	struct _FileUploadCmd_S2C {
		DWORD						dwFileTag;
		string						sClientPath;
		QWORD						qwFileSize;
		DWORD						dwSectionNum;
		MSGPACK_DEFINE(dwFileTag, sClientPath, qwFileSize, dwSectionNum)
	};


	struct _FileUploadCmd_C2S {
		DWORD						dwFileTag;
		BOOL						bOpenFile;
		MSGPACK_DEFINE(dwFileTag, bOpenFile)
	};

	
	struct _FileUpload_S2C {
		DWORD						dwFileTag;			// 每个文件传输任务都有唯一的tag用以标识
		DWORD						dwSectionIndex;		// dwIndex从1开始自增，最后一个_FileUpload_S2C的dwIndex = 0
		msgpack::type::raw_ref		msData;
		MSGPACK_DEFINE(dwFileTag, dwSectionIndex, msData)
	};


	struct _FileUpload_C2S {
		DWORD						dwFileTag;
		DWORD						dwLastIndex;		// 已接收的切片的最新index
		MSGPACK_DEFINE(dwFileTag, dwLastIndex)
	};


	struct _FileUploadFailed_C2S {
		DWORD						dwFileTag;
		string						sClientPath;
		string						sErrorInfo;
		MSGPACK_DEFINE(dwFileTag, sClientPath, sErrorInfo)
	};

	struct _FileUploadStop_S2C {
		DWORD						dwFileTag;
		MSGPACK_DEFINE(dwFileTag)
	};
}