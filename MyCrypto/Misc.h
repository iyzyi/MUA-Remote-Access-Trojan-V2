#pragma once

//#include <random>
#include <windows.h>


#ifndef QWORD
#define QWORD unsigned long long
#endif


//VOID RandomBytes(LPBYTE pbData, DWORD dwDataLen);

VOID PrintBytes(LPBYTE pbPrintData, DWORD dwDataLen);
VOID PrintChars(CHAR *pbPrintData, DWORD dwDataLen);
VOID PrintData(LPBYTE pbPrintData, DWORD dwDataLen);

BOOL IsLittleEndding();

QWORD GetQwordFromBuffer(PBYTE pbData, DWORD dwPos);
DWORD GetDwordFromBuffer(PBYTE pbData, DWORD dwPos = 0);
WORD GetWordFromBuffer(PBYTE pbData, DWORD dwPos = 0);
BYTE GetByteFromBuffer(PBYTE pbData, DWORD dwPos = 0);

VOID WriteQwordToBuffer(PBYTE pbData, QWORD qwNum, DWORD dwPos);
VOID WriteDwordToBuffer(PBYTE pbData, DWORD dwNum, DWORD dwPos = 0);
VOID WriteWordToBuffer(PBYTE pbData, WORD wNum, DWORD dwPos = 0);
VOID WriteByteToBuffer(PBYTE pbData, BYTE byNum, DWORD dwPos = 0);

std::string Bytes2Str(PBYTE pBuffer, DWORD dwBufLen, DWORD dwOffset = 0);
std::string Wchars2Str(PWCHAR pwszBuffer);

QWORD GetCurrentTimeStamp();
QWORD GetCurrentTimeStampMs();