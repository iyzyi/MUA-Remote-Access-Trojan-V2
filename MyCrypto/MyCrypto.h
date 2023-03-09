#pragma once
#include "pch.h"
#include "Misc.h"
#include "AsymmetricCrypto.h"

#include "Include/aes.h"  
#include "Include/chacha.h"
#include "Include/hc128.h"
#include "Include/hc256.h"

#include "Include/hex.h"            // StreamTransformationFilter  
#include "Include/modes.h"          // CFB_Mode等
#include "Include/osrng.h"			// AutoSeededRandomPool


using namespace std;
using namespace CryptoPP;


#define MAX_KEY_LEN				512
#define MAX_IV_LEN				512


enum CRYPTO_ALG { 
	// 不加密
	PLAIN_TEXT, 

	// ChaCha系列
	CHACHA_20, 
	CHACHA_12,
	CHACHA_8,
	CHACHA_TLS,

	// HC系列
	HC_256,
	HC_128,

	// AES系列
	AES_ECB_256, 
	AES_ECB_192,
	AES_ECB_128,
	AES_CBC_256, 
	AES_CBC_192,
	AES_CBC_128,
	AES_CFB_256, 
	AES_CFB_192,
	AES_CFB_128,
	AES_OFB_256, 
	AES_OFB_192,
	AES_OFB_128,
	AES_CTR_256,
	AES_CTR_192,
	AES_CTR_128,
	//AES_CBC_CTS_256,
	//AES_CBC_CTS_192,
	//AES_CBC_CTS_128,
	AES_CFB_FIPS_256,
	AES_CFB_FIPS_192,
	AES_CFB_FIPS_128,

	// 
};


// ************************** 加密 **************************

class CEncrypt {
protected:
	PBYTE m_pbKey				= nullptr;
	PBYTE m_pbIv				= nullptr;

	// Encrypt()中使用此变量做密文中转
	string m_sCipherText;

	// 选取的密码算法
	CRYPTO_ALG m_iCryptoAlg;

	// 加密器
	StreamTransformationFilter* m_pEncryptor = nullptr;

	// 密码算法类，加密器需要用到
	PVOID m_pEncryption			= nullptr;
	
	// 某些密码算法可能会用到这个变量，如ChaCha_12
	AlgorithmParameters m_AlgParams;

public:
	CEncrypt();
	~CEncrypt();

	BOOL Init(CRYPTO_ALG iCryptoAlg, PBYTE pbKey = nullptr, PBYTE pbIv = nullptr);
	string Encrypt(PBYTE pbPlainText, DWORD dwPlainTextLength);
};


// ************************** 解密 **************************

class CDecrypt {
protected:
	PBYTE m_pbKey				= nullptr;
	PBYTE m_pbIv				= nullptr;

	// Decrypt()中使用此变量做明文中转
	string m_sPlainText;

	// 选取的密码算法
	CRYPTO_ALG m_iCryptoAlg;

	// 解密器
	StreamTransformationFilter* m_pDecryptor = nullptr;

	// 密码算法类，解密器需要用到
	PVOID m_pDecryption = nullptr;

	// 某些密码算法可能会用到这个变量，如ChaCha_12
	AlgorithmParameters m_AlgParams;

public:
	CDecrypt();
	~CDecrypt();

	BOOL Init(CRYPTO_ALG iCryptoAlg, PBYTE pbKey = nullptr, PBYTE pbIv = nullptr);
	string Decrypt(PBYTE pbCipherText, DWORD dwCipherTextLength);

private:
	// 解决"无法在要求对象展开的函数中使用__try"的问题
	VOID DecryptBody(PBYTE pbCipherText, DWORD dwCipherTextLength);
};