#pragma once

#include "../Include/msgpack.hpp"
#include "../../MyCrypto/MyCrypto.h"

#ifndef QWORD
#define QWORD unsigned long long
#endif


namespace nsRemoteShell {

	enum CHILD_SOCKET_TYPE {
		NOPE
	};

	// ÃüÁîºÅ
	enum COMMAND_ID {
		REMOTE_SHELL_S2C = 0x03000000, 
		REMOTE_SHELL_C2S,

		EXEC_CMD_S2C,
		EXEC_CMD_C2S
	};

	struct _ExecCmd_S2C {
		string						sCmd;		// Ö÷»úÃû
		MSGPACK_DEFINE(sCmd)
	};

	struct _ExecCmd_C2S {
		string						sResult;
		MSGPACK_DEFINE(sResult);
	};
}