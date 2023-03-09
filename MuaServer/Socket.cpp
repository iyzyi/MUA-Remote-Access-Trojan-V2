#include "pch.h"
#include "Socket.h"

#include "MuaServer.h"				// theApp
#include "MuaServerDlg.h"			// ClientManager

#include <inttypes.h>				// PRIx64


// 获取客户端管理器
CClientManager* GetClientManager()
{
	return ((CMuaServerDlg*)(theApp.m_pMainWnd))->m_pClientManager;
}


CSocket::CSocket(TCP_MODE iTcpMode) : m_pTcpPackServer(this), m_pTcpPushServer(this){
	m_iTcpMode = iTcpMode;
}


CSocket::~CSocket() {
	StopSocket();
}


VOID CSocket::InitSocket(LPWSTR pwszAddress, WORD wPort) {
	wcscpy_s(m_wszAddress, 32, pwszAddress);
	m_wPort = wPort;

	if (m_iTcpMode == TCP_PACK) {
		m_pTcpPackServer->SetLocalAddress(m_wszAddress, m_wPort);
	}
	else if (m_iTcpMode == TCP_PUSH) {
		m_pTcpPushServer->SetLocalAddress(m_wszAddress, m_wPort);
	}

	if (m_iTcpMode == TCP_PACK) {
		m_pTcpPackServer->InitSocket();
	}
	else if (m_iTcpMode == TCP_PUSH) {
		m_pTcpPushServer->InitSocket();
	}
}


BOOL CSocket::StartSocket() {
	BOOL bRet = FALSE;
	if (m_iTcpMode == TCP_PACK) {
		bRet = m_pTcpPackServer->StartSocket();
	}
	else if (m_iTcpMode == TCP_PUSH) {
		bRet = m_pTcpPushServer->StartSocket();
	}
	return bRet;
}


BOOL CSocket::StopSocket() {
	BOOL bRet = FALSE;
	if (m_iTcpMode == TCP_PACK) {
		bRet = m_pTcpPackServer->StopSocket();
	}
	else if (m_iTcpMode == TCP_PUSH) {
		bRet = m_pTcpPushServer->StopSocket();
	}
	return bRet;
}


BOOL CSocket::Disconnect(CONNID dwConnID)
{
	// 先检查是否存活
	BOOL bAlive = TRUE;
	if (m_iTcpMode == TCP_PACK) {
		bAlive = m_pTcpPackServer->m_pTcpPackServer->IsConnected(dwConnID);
	}
	else if (m_iTcpMode == TCP_PUSH) {
		bAlive = m_pTcpPushServer->m_pTcpPushServer->IsConnected(dwConnID);
	}
	
	// 如果原来就没存活，则返回真，表示连接已断开
	if (!bAlive) {
		return TRUE;
	}

	// 如果存活，则断开连接
	BOOL bRet = FALSE;
	if (m_iTcpMode == TCP_PACK) {
		bRet = m_pTcpPackServer->m_pTcpPackServer->Disconnect(dwConnID);
	}
	else if (m_iTcpMode == TCP_PUSH) {
		bRet = m_pTcpPushServer->m_pTcpPushServer->Disconnect(dwConnID);
	}
	return bRet;
}


BOOL CSocket::Send(CONNID dwConnID, PBYTE pBuffer, DWORD dwBufLen) {
	BOOL bRet = FALSE;
	if (m_iTcpMode == TCP_PACK) {
		bRet = m_pTcpPackServer->Send(dwConnID, pBuffer, dwBufLen);
	}
	else if (m_iTcpMode == TCP_PUSH) {
		bRet = m_pTcpPushServer->Send(dwConnID, pBuffer, dwBufLen);
	}
	return bRet;
}


// 加密并发送（如果pBuffer = nullptr, dwBufLen = 0，那就相当于这个封包只含有包头，不含有包体）
BOOL CSocket::SendWithEnc(CONNID dwConnID, int iCommandID, PBYTE pBuffer, DWORD dwBufLen) {
	// 客户端管理器
	CClientManager* pClientManager = GetClientManager();

	// 找到ConnID对应的ClientSocket
	CClientSocket* pClientSocket = pClientManager->GetClientSocketByConnId(dwConnID);
	if (pClientSocket == nullptr) {
		return FALSE;
	}

	// pBuffer是包体，此处封装上包头
	nsGeneralSocket::_GeneralDataPacket mData;
	mData.qwClientToken = pClientSocket->m_qwClientToken;
	mData.wServiceType = pClientSocket->m_wServiceType;
	mData.wSocketType = pClientSocket->m_wSocketType;
	mData.iCommandID = iCommandID;
	mData.dwParentSocketTag = pClientSocket->m_dwParentSocketTag;
	mData.msData.ptr = (PCHAR)pBuffer;	
	mData.msData.size = dwBufLen;

	MyBuffer mBuffer2 = MsgPack<nsGeneralSocket::_GeneralDataPacket>(mData);

	// 加密数据并发送
	string sCipherText = pClientSocket->m_pEnc->Encrypt(mBuffer2.ptr(), mBuffer2.size());

	BOOL bRet = FALSE;
	if (m_iTcpMode == TCP_PACK) {
		bRet = m_pTcpPackServer->Send(dwConnID, (PBYTE)sCipherText.c_str(), sCipherText.length());
	}
	else if (m_iTcpMode == TCP_PUSH) {
		bRet = m_pTcpPushServer->Send(dwConnID, (PBYTE)sCipherText.c_str(), sCipherText.length());
	}
	return bRet;
}


// 如果dwParentSocketTag = 0的话，不需要调用此函数，直接调用SendWithEnc即可
BOOL CSocket::SendCreateChildSocket(CONNID dwConnID, int iCommandID, DWORD dwParentSocketTag) {
	// 客户端管理器
	CClientManager* pClientManager = GetClientManager();

	// 找到ConnID对应的ClientSocket
	CClientSocket* pClientSocket = pClientManager->GetClientSocketByConnId(dwConnID);
	if (pClientSocket == nullptr) {
		return FALSE;
	}

	nsGeneralSocket::_CreateChildSocket mData;
	mData.dwParentSocketTag = dwParentSocketTag;

	MyBuffer mBuffer = MsgPack<nsGeneralSocket::_CreateChildSocket>(mData);
	BOOL bRet = SendWithEnc(dwConnID, iCommandID, mBuffer.ptr(), mBuffer.size());

	return bRet;
}




VOID CSocket::DebugPrintRecvParams(CONNID dwConnID, QWORD qwClientToken, WORD wServiceType, WORD wSocketType, int iCommandID, DWORD dwParentSocketTag, MyBuffer mBuffer) {
	if (mBuffer.size() != 0) {
		DebugPrint("[Client %d] OnReceiveWithDec: TOKEN: 0x%" PRIx64 ", SERVICE_TYPE: 0x%x, SOCKET_TYPE: 0x%x, COMMAND_ID: 0x%x, PARENT_SOCKET_TAG: 0x%x, 包体:\n\n", dwConnID, qwClientToken, wServiceType, wSocketType, iCommandID, dwParentSocketTag);
		PrintData((PBYTE)mBuffer.ptr(), mBuffer.size());
	}
	else {
		DebugPrint("[Client %d] OnReceiveWithDec: TOKEN: 0x%" PRIx64 ", SERVICE_TYPE: 0x%x, SOCKET_TYPE: 0x%x, COMMAND_ID: 0x%x, PARENT_SOCKET_TAG: 0x%x, 包体为空\n\n", dwConnID, qwClientToken, wServiceType, wSocketType, iCommandID, dwParentSocketTag);
	}
}


BOOL _IsNewClientSocket(CONNID dwConnID, const BYTE* pData, int iLength) {
	BOOL bRet = FALSE;
	nsGeneralSocket::_SetCryptoAlg_C2S mData = MsgUnpack<nsGeneralSocket::_SetCryptoAlg_C2S>((PBYTE)pData, iLength);

	bRet = (
		(mData.sCipherText0.length() == iRsaKeyBits / 8) &&
		(mData.sCipherText1.length() == iRsaKeyBits / 8) &&
		(mData.sCipherText2.length() == iRsaKeyBits / 8) &&
		(mData.sCipherText3.length() == iRsaKeyBits / 8) &&
		(mData.sCipherText4.length() == iRsaKeyBits / 8)
		);

	if (bRet) {
		string sDataPart1 = RsaDecryptUsePrivateKey(sRsaPrivateKey, mData.sCipherText0);
		nsGeneralSocket::_SetCryptoAlg_C2S_Part1 mDataPart1 = MsgUnpack<nsGeneralSocket::_SetCryptoAlg_C2S_Part1>((PBYTE)sDataPart1.c_str(), sDataPart1.length());
		bRet = (mDataPart1.iCommandID == nsGeneralSocket::SET_CRYPTP_ALG_C2S);
	}

	return bRet;
}

// 判断此Socket是否为新的ClientSocket
// (如果知道了IP:PORT, 这个TCP谁都可以连，但只有遵循我们MUA的通信协议的才是ClientSocket)
BOOL CSocket::IsNewClientSocket(CONNID dwConnID, const BYTE* pData, int iLength)
{
	BOOL bRet = FALSE;

	// 能正常将数据反序列化成_SetCryptoAlg_C2S格式而不报错、且5个密文长度都是RSA密钥长度的数据包，就是正确的传递Key和IV的数据包
	__try {
		bRet = _IsNewClientSocket(dwConnID, pData, iLength);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		bRet = FALSE;
	}

	return bRet;
}


// 初始化这个新的ClientSocket
BOOL CSocket::InitNewClientSocket(CONNID dwConnID, const BYTE* pData, int iLength)
{	
	nsGeneralSocket::_SetCryptoAlg_C2S mData = MsgUnpack<nsGeneralSocket::_SetCryptoAlg_C2S>((PBYTE)pData, iLength);

	string sKey1, sKey2, sIv1, sIv2;
	sKey1 = RsaDecryptUsePrivateKey(sRsaPrivateKey, mData.sCipherText1);
	sKey2 = RsaDecryptUsePrivateKey(sRsaPrivateKey, mData.sCipherText2);
	sIv1 = RsaDecryptUsePrivateKey(sRsaPrivateKey, mData.sCipherText3);
	sIv2 = RsaDecryptUsePrivateKey(sRsaPrivateKey, mData.sCipherText4);

	string sKey = sKey1 + sKey2;
	string sIv = sIv1 + sIv2;

	string sDataPart1 = RsaDecryptUsePrivateKey(sRsaPrivateKey, mData.sCipherText0);
	nsGeneralSocket::_SetCryptoAlg_C2S_Part1 mDataPart1 = MsgUnpack<nsGeneralSocket::_SetCryptoAlg_C2S_Part1>((PBYTE)sDataPart1.c_str(), sDataPart1.length());


	CClientUnit* pClientUnit;
	CClientSocket* pClientSocket;

	// 不仅此ClientSocket是新的，所属的ClientUnit也是新的
	if (mDataPart1.qwClientToken == 0) {
		// 创建新的ClientUnit，并将该ClientUnit录入ClientManager的map中
		pClientUnit = new CClientUnit(dwConnID);
		GetClientManager()->AddClientUtil(pClientUnit->m_qwClientToken, pClientUnit);
		DebugPrint("[Client %d] 创建新的ClientUnit, CLIENT_TOKEN: 0x%" PRIx64 "\n", dwConnID, pClientUnit->m_qwClientToken);

		// 创建新的ClientSocket，并将该ClientSocket分别录入ClientManager和所属的ClientUnit的map中
		pClientSocket = new CClientSocket(pClientUnit->m_qwClientToken, mDataPart1.wServiceType, mDataPart1.wSocketType, dwConnID, mDataPart1.dwParentSocketTag);
		GetClientManager()->AddClientSocket(dwConnID, pClientSocket);
		pClientUnit->AddClientSocket(dwConnID, pClientSocket);
		DebugPrint("[Client %d] 创建新的ClientSocket, CONNID: %d\n", dwConnID, dwConnID);
	}

	// 此ClientSocket是新的，但所属的ClientUnit已存在
	else {
		// 获取该ClientSocket所属的ClientUnit
		pClientUnit = GetClientManager()->GetClientUnitByToken(mDataPart1.qwClientToken);

		if (pClientUnit == nullptr) {
			DebugPrint("[Client %d] 客户端伪造TOKEN进行攻击，中断连接", dwConnID);
			Disconnect(dwConnID);
			return FALSE;
		}

		// 创建新的ClientSocket，并将该ClientSocket分别录入ClientManager和所属的ClientUnit的map中
		pClientSocket = new CClientSocket(pClientUnit->m_qwClientToken, mDataPart1.wServiceType, mDataPart1.wSocketType, dwConnID, mDataPart1.dwParentSocketTag);
		GetClientManager()->AddClientSocket(dwConnID, pClientSocket);
		pClientUnit->AddClientSocket(dwConnID, pClientSocket);
		DebugPrint("[Client %d] 创建新的ClientSocket, CONNID: %d\n", dwConnID, dwConnID);
	}

	// 向Client发送SET_CRYPTO_ALG数据包
	nsGeneralSocket::_SetCryptoAlg_S2C mData2;
	mData2.qwClientToken = pClientUnit->m_qwClientToken;
	mData2.wServiceType = pClientSocket->m_wServiceType;
	mData2.wSocketType = pClientSocket->m_wSocketType;
	mData2.dwParentSocketTag = pClientSocket->m_dwParentSocketTag;
	mData2.iCommandID = nsGeneralSocket::SET_CRYPTP_ALG_S2C;
	mData2.iCryptoAlg = nsGeneralSocket::GetCryptoAlgByServiceType((nsGeneralSocket::SERVICE_TYPE)mDataPart1.wServiceType);

	MyBuffer mBuffer = MsgPack<nsGeneralSocket::_SetCryptoAlg_S2C>(mData2);
	//string sBuffer = Bytes2Str(pBuffer, dwBufLen);
	//delete[] pBuffer;

	string sCipherTextReturn = RsaEncryptUsePrivateKey(sRsaPrivateKey, Bytes2Str(mBuffer.ptr(), mBuffer.size()));
	Send(dwConnID, (PBYTE)sCipherTextReturn.c_str(), sCipherTextReturn.length());

	// 设置本Socket的通信Key和IV
	pClientSocket->SetCrypto((CRYPTO_ALG)mData2.iCryptoAlg, (PBYTE)sKey.c_str(), (PBYTE)sIv.c_str());

	return TRUE;
}


EnHandleResult CSocket::OnReceive(ITcpServer* pSender, CONNID dwConnID, const BYTE* pData, int iLength) {
	DebugPrint("[Client %d] OnReceive: %d Bytes \n", dwConnID, iLength);
	
	// 找到ConnID对应的ClientSocket
	CClientSocket* pClientSocket = GetClientManager()->GetClientSocketByConnId(dwConnID);

	// 新的Socket
	if (pClientSocket == nullptr) {

		// 判断这个新的Socket是否是ClientSocket(如果知道了IP:PORT, 这个TCP谁都可以连，但只有遵循我们MUA的通信协议的才是ClientSocket)
		if (IsNewClientSocket(dwConnID, pData, iLength)) {
			// 初始化这个新的ClientSocket
			InitNewClientSocket(dwConnID, pData, iLength);
		}

		// 不是ClientSocket，断开连接
		else {
			DebugPrint("[Client %d] 不符合本项目的通信协议，断开连接\n", dwConnID);
			Disconnect(dwConnID);
		}
	}

	// 不是新的Socket，正常解密
	else {
		string sPlainText = pClientSocket->m_pDec->Decrypt((PBYTE)pData, iLength);

		if (sPlainText == "") {
			DebugPrint("[Client %d] 解密接收到的数据时出现异常\n", dwConnID);
		}
		else {
			nsGeneralSocket::_GeneralDataPacket mData = MsgUnpack<nsGeneralSocket::_GeneralDataPacket>((PBYTE)sPlainText.c_str(), sPlainText.length());

			MyBuffer mBuffer = MyBuffer((PBYTE)mData.msData.ptr, mData.msData.size);
			OnReceiveWithDec(pSender, dwConnID, mData.qwClientToken, mData.wServiceType, mData.wSocketType, mData.iCommandID, mData.dwParentSocketTag, mBuffer);
		}
	}
	
	return HR_OK;
}


EnHandleResult CSocket::OnClose(ITcpServer* pSender, CONNID dwConnID, EnSocketOperation enOperation, int iErrorCode){
	DebugPrint("[Client %d] OnClose: \n", dwConnID);

	CClientSocket* pClientSocket = GetClientManager()->GetClientSocketByConnId(dwConnID);
	
	// 如果是在DeleteClientSocket中断开的socket连接，则GetClientSocketByConnId必为nullptr，就不用在这里再次处理了，不然就陷入循环处理了。
	if (pClientSocket != nullptr) {
		CClientUnit* pClientUnit = GetClientManager()->GetClientUnitByToken(pClientSocket->m_qwClientToken);

		// 当前断开的socket连接是Client的主socket，则删掉整个ClientUnit
		if (pClientUnit->m_dwMainSocketConnID == dwConnID) {
			GetClientManager()->DeleteClientUnit(pClientUnit);
		}

		// 当前断开的socket连接不是主socket，则只删掉这个ClientSocket
		else {
			GetClientManager()->DeleteClientSocket(pClientSocket);
		}
	}

	return HR_OK;
}