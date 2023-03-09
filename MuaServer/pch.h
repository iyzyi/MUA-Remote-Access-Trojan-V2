// pch.h: 这是预编译标头文件。
// 下方列出的文件仅编译一次，提高了将来生成的生成性能。
// 这还将影响 IntelliSense 性能，包括代码完成和许多代码浏览功能。
// 但是，如果此处列出的文件中的任何一个在生成之间有更新，它们全部都将被重新编译。
// 请勿在此处添加要频繁更新的文件，这将使得性能优势无效。

#ifndef PCH_H
#define PCH_H

// 添加要在此处预编译的标头
#include "framework.h"

#include <inttypes.h>				// PRIx64
#include <sstream>					// stringstream
#include <fstream>					// ofstream
#include <io.h>						// _access
#include <direct.h>					// _mkdir


#include "../MyHpSocket/MyHpSocket.h"
#include "../MyCrypto/MyCrypto.h"
#include "../MyMsgPack/MyMsgPack.h"


//#define ROOT_DIR	(_getcwd(NULL, 0))
#define ROOT_DIR "D:\\桌面"

#include <string>
const int iRsaKeyBits = 3072;
const std::string sRsaPrivateKey =
"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"
"UhsNQt/+zA==";






#endif //PCH_H 