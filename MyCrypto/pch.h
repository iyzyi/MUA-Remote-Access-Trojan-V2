#pragma once

#include <stdio.h>
#include <windows.h>
#include <iostream>
#include <string>


#ifdef _WIN64
	#ifdef _MT
		#ifdef _DLL
			#ifdef _DEBUG
				#pragma comment(lib, "../MyCrypto/Lib/cryptlib_x64_MDd_U.lib")
			#else
				#pragma comment(lib, "../MyCrypto/Lib/cryptlib_x64_MD_U.lib")
			#endif
		#else
			#ifdef _DEBUG
				#pragma comment(lib, "../MyCrypto/Lib/cryptlib_x64_MTd_U.lib")
			#else
				#pragma comment(lib, "../MyCrypto/Lib/cryptlib_x64_MT_U.lib")
			#endif
		#endif
	#endif
#elif _WIN32
	#ifdef _MT
		#ifdef _DLL
			#ifdef _DEBUG
				#pragma comment(lib, "../MyCrypto/Lib/cryptlib_x86_MDd_U.lib")
			#else
				#pragma comment(lib, "../MyCrypto/Lib/cryptlib_x86_MD_U.lib")
			#endif
			#else
			#ifdef _DEBUG
				#pragma comment(lib, "../MyCrypto/Lib/cryptlib_x86_MTd_U.lib")
			#else
				#pragma comment(lib, "../MyCrypto/Lib/cryptlib_x86_MT_U.lib")
			#endif
		#endif
	#endif
#endif


#ifdef _DEBUG
	#define DebugPrint(...) printf(__VA_ARGS__)
#else
	#define DebugPrint(...)
#endif