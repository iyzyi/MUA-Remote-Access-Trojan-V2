#pragma once

#include "../Include/msgpack.hpp"
#include "../../MyCrypto/MyCrypto.h"

#ifndef QWORD
#define QWORD unsigned long long
#endif


namespace nsGeneralSocket {

	// 服务类型
	enum SERVICE_TYPE {
		NULL_SERVICE,
		MAIN_SOCKET_SERVICE,
		REMOTE_SHELL_SERVICE,
		FILE_TRANSFER_SERVICE,
		IMAGE_CAPTURE_SERVICE,
		DESKTOP_MONITOR_SERVICE,
	};


	CRYPTO_ALG GetCryptoAlgByServiceType(SERVICE_TYPE wServiceType);


	enum SOCKET_TYPE {
		ROOT_SOCKET_WITH_DIALOG_TYPE,					// 各服务的主socket（此服务含对话框）
		ROOT_SOCKET_TYPE,								// 各服务的主socket（此服务不含对话框）
	};


	// 命令号
	enum COMMAND_ID {
		SET_CRYPTP_ALG_C2S = 0x01000000,
		SET_CRYPTP_ALG_S2C,

		CHANNEL_SUCCESS_C2S,							// 加密信道构建成功
		CHANNEL_SUCCESS_S2C
	};


	struct _SetCryptoAlg_C2S_Part1 {
		QWORD						qwClientToken;		// 应置0
		WORD						wServiceType;	// SERVICE_TYPE
		WORD						wSocketType;	// SOCKET_TYPE
		int							iCommandID;		// COMMAND_ID
		DWORD						dwParentSocketTag;	// 应置0
		MSGPACK_DEFINE(qwClientToken, wServiceType, wSocketType, iCommandID, dwParentSocketTag)
	};

	struct _SetCryptoAlg_C2S {
		string						sCipherText0;	// _SetCryptoAlg_C2S_Part1
		string						sCipherText1;	// Key Part1
		string						sCipherText2;	// Key Part2
		string						sCipherText3;	// IV Part1
		string						sCipherText4;	// IV Part2
		MSGPACK_DEFINE(sCipherText0, sCipherText1, sCipherText2, sCipherText3, sCipherText4)
	};


	struct _SetCryptoAlg_S2C {
		QWORD						qwClientToken;
		WORD						wServiceType;	// SERVICE_TYPE
		WORD						wSocketType;	// SOCKET_TYPE
		int							iCommandID;		// COMMAND_ID
		DWORD						dwParentSocketTag;
		int							iCryptoAlg;		// CRYPTO_ALG
		MSGPACK_DEFINE(qwClientToken, wServiceType, wSocketType, iCommandID, dwParentSocketTag, iCryptoAlg)
	};


	// 通用数据包格式
	struct _GeneralDataPacket {
		QWORD						qwClientToken;
		WORD						wServiceType;	// SERVICE_TYPE，表示此Socket所属服务的服务类型
		WORD						wSocketType;	// SOCKET_TYPE，表示此Socket的类型，比如文件传输中，有负责文件上传下载的socket，有负责列出文件的socket等等
		int							iCommandID;		// COMMAND_ID
		DWORD						dwParentSocketTag;
		msgpack::type::raw_ref		msData;
		MSGPACK_DEFINE(qwClientToken, wServiceType, wSocketType, iCommandID, dwParentSocketTag, msData)
	};


	struct _CreateChildSocket {
		DWORD						dwParentSocketTag;
		MSGPACK_DEFINE(dwParentSocketTag)
	};

}
