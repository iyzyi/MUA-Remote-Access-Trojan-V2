#pragma once


#ifdef _WIN64
    #ifdef _MT
        #ifdef _DLL
            #ifdef _DEBUG
                #pragma comment(lib, "../MyMsgPack/lib/msgpack_x64_MDd_U.lib")
            #else
                #pragma comment(lib, "../MyMsgPack/lib/msgpack_x64_MD_U.lib")
            #endif
        #else
            #ifdef _DEBUG
                #pragma comment(lib, "../MyMsgPack/lib/msgpack_x64_MTd_U.lib")
            #else
                #pragma comment(lib, "../MyMsgPack/lib/msgpack_x64_MT_U.lib")
            #endif
        #endif
    #endif
#elif _WIN32
    #ifdef _MT
        #ifdef _DLL
            #ifdef _DEBUG
                #pragma comment(lib, "../MyMsgPack/lib/msgpack_x86_MDd_U.lib")
            #else
                #pragma comment(lib, "../MyMsgPack/lib/msgpack_x86_MD_U.lib")
            #endif
        #else
            #ifdef _DEBUG
                #pragma comment(lib, "../MyMsgPack/lib/msgpack_x86_MTd_U.lib")
            #else
                #pragma comment(lib, "../MyMsgPack/lib/msgpack_x86_MT_U.lib")
            #endif
        #endif
    #endif
#endif


#include "Include/msgpack.hpp"

#include "MsgStruct/GeneralSocket.h"
#include "MsgStruct/MainSocket.h"
#include "MsgStruct/RemoteShell.h"
#include "MsgStruct/FileTransfer.h"
#include "MsgStruct/ImageCapture.h"
#include "MsgStruct/DesktopMonitor.h"


// 智能指针 缓冲区
class MyBuffer {
private:
    std::shared_ptr<BYTE> share_ptr;
    int share_ptr_size;

public:
    auto ptr() {
        return share_ptr.get();
    }
    int size() {
        return share_ptr_size;
    }
    MyBuffer(int size) {
        share_ptr = std::shared_ptr<BYTE>(new BYTE[size], [](PBYTE p) {delete[] p;});
        this->share_ptr_size = size;
    }
    MyBuffer(PBYTE src, int size) {
        share_ptr = std::shared_ptr<BYTE>(new BYTE[size], [](PBYTE p) {delete[] p;});
        this->share_ptr_size = size;
        memcpy(ptr(), src, size);
    }
};




/// <summary>
/// 将MsgData序列化。
/// </summary>
/// <typeparam name="T"></typeparam>
/// <param name="MsgData">自定义的struct</param>
/// <param name="pOutBuffer">输出缓冲区。注意使用完后要手动delete这个缓冲区</param>
/// <returns>输出缓冲区的大小</returns>
template <class T>
DWORD MsgPack(T MsgData, PBYTE& pOutBuffer) {
    msgpack::sbuffer sbuffer;
    msgpack::pack(sbuffer, MsgData);

    pOutBuffer = new BYTE[sbuffer.size()];
    memcpy(pOutBuffer, sbuffer.data(), sbuffer.size());

    return sbuffer.size();
}


template <class T>
MyBuffer MsgPack(T MsgData) {
    msgpack::sbuffer sbuffer;
    msgpack::pack(sbuffer, MsgData);
    MyBuffer mBuffer = MyBuffer((BYTE*)sbuffer.data(), sbuffer.size());
    return mBuffer;
}


/// <summary>
/// 反序列化成MsgData
/// </summary>
/// <typeparam name="T"></typeparam>
/// <param name="pBuffer">将要进行反序列化的数据缓冲区</param>
/// <param name="dwBufSize">数据缓冲区的大小</param>
/// <returns>MsgData，是自定义类型的struct</returns>
template <class T>
T MsgUnpack(PBYTE pBuffer, DWORD dwBufSize)
{
    // 反序列化
    msgpack::unpacked  unpack;
    msgpack::unpack(&unpack, (char*)pBuffer, dwBufSize);

    //msgpack::object obj = unpack.get();

    //T MsgData;
    //obj.convert(&MsgData);

    T MsgData = unpack.get().as<T>();

    return MsgData;
}



struct _PackedUnit {
    PBYTE           pBuffer;
    DWORD           dwBufLen;
    _PackedUnit(PBYTE pBuffer, DWORD dwBufLen) {
        this->pBuffer = pBuffer;
        this->dwBufLen = dwBufLen;
    }
};


template <class _Unit>
VOID MsgAddUnitToVec(vector<_PackedUnit>& vPackedUnits, _Unit Unit) {
    PBYTE pBuffer = nullptr;
    DWORD dwBufLen = MsgPack<_Unit>(Unit, pBuffer);
    
    _PackedUnit PackedUnit(pBuffer, dwBufLen);
    vPackedUnits.push_back(PackedUnit);
}


template <class _Unit>
DWORD MsgMergeUnitsToBuf(vector<_PackedUnit>& vPackedUnits, PBYTE& pTotalBuffer, int iStart=0, int iEnd=-1){
    if (iEnd == -1) {
        iEnd = vPackedUnits.size();
    }

    DWORD dwTotalBufLen = 0;
    for (int i = iStart; i < iEnd; i++) {
        dwTotalBufLen += sizeof(DWORD) + vPackedUnits[i].dwBufLen;
    }

    pTotalBuffer = new BYTE[dwTotalBufLen];
    DWORD dwPointer = 0;

    for (int i = iStart; i < iEnd; i++) {
        WriteDwordToBuffer(pTotalBuffer, vPackedUnits[i].dwBufLen, dwPointer);
        dwPointer += sizeof(DWORD);

        memcpy(pTotalBuffer + dwPointer, vPackedUnits[i].pBuffer, vPackedUnits[i].dwBufLen);
        dwPointer += vPackedUnits[i].dwBufLen;

        delete vPackedUnits[i].pBuffer;
    }

    return dwPointer;
}


template <class _Unit>
auto MsgMergeUnitsToBuf(vector<_PackedUnit>& vPackedUnits, int iStart = 0, int iEnd = -1) {
    if (iEnd == -1) {
        iEnd = vPackedUnits.size();
    }

    DWORD dwTotalBufLen = 0;
    for (int i = iStart; i < iEnd; i++) {
        dwTotalBufLen += sizeof(DWORD) + vPackedUnits[i].dwBufLen;
    }

    MyBuffer mTotalBuffer = MyBuffer(dwTotalBufLen);
    DWORD dwPointer = 0;

    for (int i = iStart; i < iEnd; i++) {
        WriteDwordToBuffer(mTotalBuffer.ptr(), vPackedUnits[i].dwBufLen, dwPointer);
        dwPointer += sizeof(DWORD);

        memcpy(mTotalBuffer.ptr() + dwPointer, vPackedUnits[i].pBuffer, vPackedUnits[i].dwBufLen);
        dwPointer += vPackedUnits[i].dwBufLen;

        delete vPackedUnits[i].pBuffer;
    }

    return mTotalBuffer;
}



template <class _Unit>
vector<_Unit> MsgGetUnitsFromBuf(PBYTE pTotalBuffer, DWORD dwUnitsNum) {
    vector<_Unit> vUnits;
    DWORD dwPointer = 0;

    for (int i = 0; i < dwUnitsNum; i++) {
        DWORD dwUnitBufLen = GetDwordFromBuffer(pTotalBuffer, dwPointer);
        dwPointer += sizeof(DWORD);

        PBYTE pBuffer = pTotalBuffer + dwPointer;
        _Unit mData = MsgUnpack<_Unit>(pBuffer, dwUnitBufLen);
        dwPointer += dwUnitBufLen;

        vUnits.push_back(mData);
    }

    return vUnits;
}