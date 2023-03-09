#pragma once

#include "../Include/msgpack.hpp"
#include "../../MyCrypto/MyCrypto.h"

#ifndef QWORD
#define QWORD unsigned long long
#endif


// 为了未来可以兼容Linux系统的Client，涉及字符串的数据不要使用Unicode进行传输，最好用string。

namespace nsMainSocket {

	enum CHILD_SOCKET_TYPE {
		NOPE
	};

	// 命令号
	enum COMMAND_ID {
		LOGIN_PACKET_C2S = 0x02000000,
		LOGIN_PACKET_S2C
	};
	
	struct _LoginPacket_C2S {
		string						sHostName;		// 主机名
		string						sOsVersion;		// 系统版本
		string						sCpuType;		// CPU类型
		string						sMemoryInfo;	// 内存信息
		DWORD						dwCameraNum;	// 摄像头数量
		MSGPACK_DEFINE(sHostName, sOsVersion, sCpuType, sMemoryInfo, dwCameraNum)
	};
}