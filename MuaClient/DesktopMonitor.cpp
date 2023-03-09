#include "pch.h"
#include "DesktopMonitor.h"

using namespace nsDesktopMonitor;


CDesktopMonitor::CDesktopMonitor(TCP_MODE iTcpMode, WORD wSocketType, QWORD qwClientToken, DWORD dwParentSocketTag/* = 0 */) : CSocket(iTcpMode, nsGeneralSocket::DESKTOP_MONITOR_SERVICE, wSocketType, qwClientToken) {
    m_qwClientToken = qwClientToken;
    m_hExitThreadEvent = CreateEvent(NULL, true, false, NULL);
    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)PrintScreenThreadFunc, (LPVOID)this, 0, NULL);
}


CDesktopMonitor::~CDesktopMonitor() {
    if (m_hExitThreadEvent != NULL) {
        CloseHandle(m_hExitThreadEvent);
        m_hExitThreadEvent = NULL;
    }

    SetEvent(m_hExitThreadEvent);
}


VOID CDesktopMonitor::OnReceiveWithDec(ITcpClient* pSender, CONNID dwConnID, QWORD qwClientToken, WORD wServiceType, WORD wSocketType, int iCommandID, DWORD dwParentSocketTag, MyBuffer mBuffer) {
    DebugPrintRecvParams(dwConnID, qwClientToken, wServiceType, wSocketType, iCommandID, dwParentSocketTag, mBuffer);

    switch (iCommandID) {
    case nsGeneralSocket::CHANNEL_SUCCESS_S2C:
        ;
        break;

    case LIST_SCREEN_S2C:
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ListScreen, (LPVOID)this, 0, NULL);
        break;

    case DISPLAY_SCREEN_S2C:
        _DisplayScreen_S2C mData = MsgUnpack<_DisplayScreen_S2C>(mBuffer.ptr(), mBuffer.size());
        _DisplayScreenThreadParam* pThreadParam = new _DisplayScreenThreadParam(this, mData.sScreenName);
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)DisplayScreen, (LPVOID)pThreadParam, 0, NULL);
        break;
    }
}


EnHandleResult CDesktopMonitor::OnClose(ITcpClient* pSender, CONNID dwConnID, EnSocketOperation enOperation, int iErrorCode) {
    DebugPrint("[Client %d] OnClose: \n", dwConnID);

    delete this;

    return HR_OK;
}





// *********************************** 屏幕截图 ***********************************

BOOL CALLBACK CallbackEnumMonitor2(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lp_monitor, LPARAM data)
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


VOID CDesktopMonitor::ListScreen(LPVOID lParam) {
    CDesktopMonitor* pThis = (CDesktopMonitor*)lParam;

    // 枚举所有的显示器
    std::vector<_MonitorProperty> vScreenUnits;
    EnumDisplayMonitors(NULL, NULL, CallbackEnumMonitor2, (LPARAM)&vScreenUnits);

    vector<_PackedUnit> vPackedUnits;
    for (auto& monitor : vScreenUnits) {
        _ScreenUnit_WIN_C2S mData;
        mData.sScreenName = Wchars2Str((PWCHAR)monitor.monitor_name.c_str());
        mData.dwWidth = monitor.width;
        mData.dwHeight = monitor.height;
        mData.dwX = monitor.x;
        mData.dwY = monitor.y;
        mData.bIsMainScreen = monitor.flag;
        MsgAddUnitToVec<_ScreenUnit_WIN_C2S>(vPackedUnits, mData);
    }

    PBYTE pBuffer = nullptr;
    DWORD dwBufLen = MsgMergeUnitsToBuf<_ScreenUnit_WIN_C2S>(vPackedUnits, pBuffer);

    _ListScreen_C2S mData2;
    mData2.dwScreenNum = vScreenUnits.size();
    mData2.msData.ptr = (PCHAR)pBuffer;
    mData2.msData.size = dwBufLen;
    
    MyBuffer mFinalBuffer = MsgPack<_ListScreen_C2S>(mData2);

    pThis->SendWithEnc(LIST_SCREEN_C2S, mFinalBuffer.ptr(), mFinalBuffer.size());
    delete pBuffer;
}


VOID CDesktopMonitor::DisplayScreen(LPVOID lParam) {
    _DisplayScreenThreadParam* pThreadParam = (_DisplayScreenThreadParam*)lParam;
    CDesktopMonitor* pThis = pThreadParam->pThis;
    string sScreenName = pThreadParam->sScreenName;
    delete pThreadParam;

    // 枚举所有的显示器
    std::vector<_MonitorProperty> vScreenUnits;
    EnumDisplayMonitors(NULL, NULL, CallbackEnumMonitor2, (LPARAM)&vScreenUnits);
    
    BOOL bRet = FALSE;
    for (auto& monitor : vScreenUnits) {
        if (sScreenName == Wchars2Str((PWCHAR)monitor.monitor_name.c_str())) {
            pThis->m_bChooseMonitor = TRUE;
            pThis->m_ChooseMonitor = monitor;
            bRet = TRUE;
            break;
        }
    }

    if (!bRet) {
        _DisplayScreenFailed_C2S mData;
        CHAR szErrorInfo[64];
        sprintf_s(szErrorInfo, "未找到显示器[%s]", sScreenName.c_str());
        mData.sErrorInfo = szErrorInfo;
        MyBuffer mBuffer = MsgPack<_DisplayScreenFailed_C2S>(mData);
        pThis->SendWithEnc(DISPLAY_SCREEN_FAILED_C2S, mBuffer.ptr(), mBuffer.size());
    }
}


VOID CDesktopMonitor::PrintScreenThreadFunc(LPVOID lParam) {
    CDesktopMonitor* pThis = (CDesktopMonitor*)lParam;

    while (1) {
        // 触发关闭事件时跳出循环，结束线程
        if (WAIT_OBJECT_0 == WaitForSingleObject(pThis->m_hExitThreadEvent, 0)) {
            break;
        }

        if (pThis->m_bChooseMonitor) {
            pThis->PrintScreen(pThis->m_ChooseMonitor);
        }

        Sleep(100);
    }

    MessageBoxA(0, "123", "123", 0);
}


VOID CDesktopMonitor::PrintScreen(_MonitorProperty monitor)
{
    HDC hdc = ::GetDC(GetDesktopWindow());
    HDC com_hdc = CreateCompatibleDC(hdc);

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

    // TODO 压缩BMP图片

    SendImage(mImageData.ptr(), dwImageLength);
}




// 图片切片发送（受HpSocket TCP_PACK 的数据包最大长度限制）
#define IMAGE_SECTION_LENGTH  (PACKET_MAX_LENGTH - 0xff)
VOID CDesktopMonitor::SendImage(PBYTE pbImageData, DWORD dwImageLength) {

    DWORD dwSectionNumber = (dwImageLength % PACKET_MAX_LENGTH) ? (dwImageLength / PACKET_MAX_LENGTH + 1) : (dwImageLength / PACKET_MAX_LENGTH);

    QWORD qwImageTime = GetCurrentTimeStampMs();        // 毫秒级时间戳，用作图像的唯一标识

    for (int i = 1; i <= dwSectionNumber; i++) {
        _DisplayScreen_C2S mData;
        mData.qwTime = qwImageTime;
        mData.dwImageSize = dwImageLength;
        mData.dwSectionNum = dwSectionNumber;
        mData.dwSectionIndex = (i == dwSectionNumber) ? 0 : i;
        mData.msImageSection.ptr = (PCHAR)(pbImageData + (i - 1) * IMAGE_SECTION_LENGTH);
        mData.msImageSection.size = (i == dwSectionNumber) ? (dwImageLength - (i - 1) * IMAGE_SECTION_LENGTH) : IMAGE_SECTION_LENGTH;

        PBYTE pBuffer = nullptr;
        MyBuffer mBuffer = MsgPack<_DisplayScreen_C2S>(mData);

        m_pImageSocket->SendWithEnc(DISPLAY_SCREEN_C2S, mBuffer.ptr(), mBuffer.size());
        delete[] pBuffer;
    }
}