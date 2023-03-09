#include "pch.h"
#include "MyCrypto.h"

 
// ************************** 加密 **************************

CEncrypt::CEncrypt() {
    ;
}


CEncrypt::~CEncrypt() {
    if (m_pbKey != nullptr) {
        delete[] m_pbKey;
        m_pbKey = nullptr;
    }

    if (m_pbIv != nullptr) {
        delete[] m_pbIv;
        m_pbIv = nullptr;
    }

    if (m_pEncryptor != nullptr) {
        delete m_pEncryptor;
        m_pEncryptor = nullptr;
    }

    if (m_pEncryption != nullptr) {
        delete m_pEncryption;
        m_pEncryption = nullptr;
    }
}


/// @brief Init()中会截取pbKey和pbIv，所以不用担心传入的Key和Iv过长，但需要保证Key和Iv不能短于算法所需长度。
BOOL CEncrypt::Init(CRYPTO_ALG iCryptoAlg, PBYTE pbKey, PBYTE pbIv) {
    m_iCryptoAlg = iCryptoAlg;

    m_pbKey = new BYTE[MAX_KEY_LEN];
    m_pbIv = new BYTE[MAX_IV_LEN];
    memset(m_pbKey, 0, MAX_KEY_LEN);
    memset(m_pbIv, 0, MAX_IV_LEN);

    switch (m_iCryptoAlg) {

    // 不加密
    case PLAIN_TEXT: {
        break;
    }


    #pragma region ChaCha系列
    // ChaCha系列
    case CHACHA_20: {
        memcpy(m_pbKey, pbKey, ChaCha::MAX_KEYLENGTH);
        memcpy(m_pbIv, pbIv, 8);
        m_pEncryption = new ChaCha::Encryption();
        ((ChaCha::Encryption*)m_pEncryption)->SetKeyWithIV(m_pbKey, ChaCha::MAX_KEYLENGTH, m_pbIv, 8);
        m_pEncryptor = new StreamTransformationFilter(*((ChaCha::Encryption*)m_pEncryption), new StringSink(m_sCipherText));
        break;
    }

    case CHACHA_12: {
        memcpy(m_pbKey, pbKey, ChaCha::MAX_KEYLENGTH);
        memcpy(m_pbIv, pbIv, 8);
        string sIv((PCHAR)m_pbIv, 8);
        m_AlgParams = MakeParameters (Name::Rounds(), 12) (Name::IV(), ConstByteArrayParameter(sIv, 8));
        m_pEncryption = new ChaCha::Encryption();
        ((ChaCha::Encryption*)m_pEncryption)->SetKey(m_pbKey, ChaCha::MAX_KEYLENGTH, m_AlgParams);
        m_pEncryptor = new StreamTransformationFilter(*((ChaCha::Encryption*)m_pEncryption), new StringSink(m_sCipherText));
        break;
    }

    case CHACHA_8: {
        memcpy(m_pbKey, pbKey, ChaCha::MAX_KEYLENGTH);
        memcpy(m_pbIv, pbIv, 8);
        string sIv((PCHAR)m_pbIv, 8);
        m_AlgParams = MakeParameters(Name::Rounds(), 8) (Name::IV(), ConstByteArrayParameter(sIv, 8));
        m_pEncryption = new ChaCha::Encryption();
        ((ChaCha::Encryption*)m_pEncryption)->SetKey(m_pbKey, ChaCha::MAX_KEYLENGTH, m_AlgParams);
        m_pEncryptor = new StreamTransformationFilter(*((ChaCha::Encryption*)m_pEncryption), new StringSink(m_sCipherText));
        break;
    }

    case CHACHA_TLS: {
        memcpy(m_pbKey, pbKey, ChaChaTLS::MAX_KEYLENGTH);
        memcpy(m_pbIv, pbIv, 12);
        m_pEncryption = new ChaChaTLS::Encryption();
        ((ChaChaTLS::Encryption*)m_pEncryption)->SetKeyWithIV(m_pbKey, ChaChaTLS::MAX_KEYLENGTH, m_pbIv, 12);
        m_pEncryptor = new StreamTransformationFilter(*((ChaChaTLS::Encryption*)m_pEncryption), new StringSink(m_sCipherText));
        break;
    }
    #pragma endregion


    case HC_256: {
        memcpy(m_pbKey, pbKey, HC256::MAX_KEYLENGTH);
        memcpy(m_pbIv, pbIv, 32);
        m_pEncryption = new HC256::Encryption();
        ((HC256::Encryption*)m_pEncryption)->SetKeyWithIV(m_pbKey, HC256::MAX_KEYLENGTH, m_pbIv, 32);
        m_pEncryptor = new StreamTransformationFilter(*((HC256::Encryption*)m_pEncryption), new StringSink(m_sCipherText));
        break;
    }

    case HC_128: {
        memcpy(m_pbKey, pbKey, HC128::MAX_KEYLENGTH);
        memcpy(m_pbIv, pbIv, 16);
        m_pEncryption = new HC128::Encryption();
        ((HC128::Encryption*)m_pEncryption)->SetKeyWithIV(m_pbKey, HC128::MAX_KEYLENGTH, m_pbIv, 16);
        m_pEncryptor = new StreamTransformationFilter(*((HC128::Encryption*)m_pEncryption), new StringSink(m_sCipherText));
        break;
    }


    #pragma region AES系列
    // AES系列
    case AES_ECB_256: {
        memcpy(m_pbKey, pbKey, AES::MAX_KEYLENGTH);
        m_pEncryption = new ECB_Mode<AES>::Encryption(m_pbKey, AES::MAX_KEYLENGTH);
        m_pEncryptor = new StreamTransformationFilter(*((ECB_Mode<AES>::Encryption*)m_pEncryption), new StringSink(m_sCipherText));
        break;
    }

    case AES_ECB_192: {
        memcpy(m_pbKey, pbKey, 24);
        m_pEncryption = new ECB_Mode<AES>::Encryption(m_pbKey, 24);
        m_pEncryptor = new StreamTransformationFilter(*((ECB_Mode<AES>::Encryption*)m_pEncryption), new StringSink(m_sCipherText));
        break;
    }

    case AES_ECB_128: {
        memcpy(m_pbKey, pbKey, AES::MIN_KEYLENGTH);
        m_pEncryption = new ECB_Mode<AES>::Encryption(m_pbKey, AES::MIN_KEYLENGTH);
        m_pEncryptor = new StreamTransformationFilter(*((ECB_Mode<AES>::Encryption*)m_pEncryption), new StringSink(m_sCipherText));
        break;
    }

    case AES_CBC_256: {
        memcpy(m_pbKey, pbKey, AES::MAX_KEYLENGTH);
        memcpy(m_pbIv, pbIv, AES::BLOCKSIZE);
        m_pEncryption = new CBC_Mode<AES>::Encryption(m_pbKey, AES::MAX_KEYLENGTH, m_pbIv);
        m_pEncryptor = new StreamTransformationFilter(*((CBC_Mode<AES>::Encryption*)m_pEncryption), new StringSink(m_sCipherText));
        break;
    }

    case AES_CBC_192: {
        memcpy(m_pbKey, pbKey, 24);
        memcpy(m_pbIv, pbIv, AES::BLOCKSIZE);
        m_pEncryption = new CBC_Mode<AES>::Encryption(m_pbKey, 24, m_pbIv);
        m_pEncryptor = new StreamTransformationFilter(*((CBC_Mode<AES>::Encryption*)m_pEncryption), new StringSink(m_sCipherText));
        break;
    }

    case AES_CBC_128: {
        memcpy(m_pbKey, pbKey, AES::MIN_KEYLENGTH);
        memcpy(m_pbIv, pbIv, AES::BLOCKSIZE);
        m_pEncryption = new CBC_Mode<AES>::Encryption(m_pbKey, AES::MIN_KEYLENGTH, m_pbIv);
        m_pEncryptor = new StreamTransformationFilter(*((CBC_Mode<AES>::Encryption*)m_pEncryption), new StringSink(m_sCipherText));
        break;
    }

    case AES_CFB_256: {
        memcpy(m_pbKey, pbKey, AES::MAX_KEYLENGTH);
        memcpy(m_pbIv, pbIv, AES::BLOCKSIZE);
        m_pEncryption = new CFB_Mode<AES>::Encryption(m_pbKey, AES::MAX_KEYLENGTH, m_pbIv);
        m_pEncryptor = new StreamTransformationFilter(*((CFB_Mode<AES>::Encryption*)m_pEncryption), new StringSink(m_sCipherText));
        break;
    }

    case AES_CFB_192: {
        memcpy(m_pbKey, pbKey, 24);
        memcpy(m_pbIv, pbIv, AES::BLOCKSIZE);
        m_pEncryption = new CFB_Mode<AES>::Encryption(m_pbKey, 24, m_pbIv);
        m_pEncryptor = new StreamTransformationFilter(*((CFB_Mode<AES>::Encryption*)m_pEncryption), new StringSink(m_sCipherText));
        break;
    }

    case AES_CFB_128: {
        memcpy(m_pbKey, pbKey, AES::MIN_KEYLENGTH);
        memcpy(m_pbIv, pbIv, AES::BLOCKSIZE);
        m_pEncryption = new CFB_Mode<AES>::Encryption(m_pbKey, AES::MIN_KEYLENGTH, m_pbIv);
        m_pEncryptor = new StreamTransformationFilter(*((CFB_Mode<AES>::Encryption*)m_pEncryption), new StringSink(m_sCipherText));
        break;
    }

    case AES_OFB_256: {
        memcpy(m_pbKey, pbKey, AES::MAX_KEYLENGTH);
        memcpy(m_pbIv, pbIv, AES::BLOCKSIZE);
        m_pEncryption = new OFB_Mode<AES>::Encryption(m_pbKey, AES::MAX_KEYLENGTH, m_pbIv);
        m_pEncryptor = new StreamTransformationFilter(*((OFB_Mode<AES>::Encryption*)m_pEncryption), new StringSink(m_sCipherText));
        break;
    }

    case AES_OFB_192: {
        memcpy(m_pbKey, pbKey, 24);
        memcpy(m_pbIv, pbIv, AES::BLOCKSIZE);
        m_pEncryption = new OFB_Mode<AES>::Encryption(m_pbKey, 24, m_pbIv);
        m_pEncryptor = new StreamTransformationFilter(*((OFB_Mode<AES>::Encryption*)m_pEncryption), new StringSink(m_sCipherText));
        break;
    }

    case AES_OFB_128: {
        memcpy(m_pbKey, pbKey, AES::MIN_KEYLENGTH);
        memcpy(m_pbIv, pbIv, AES::BLOCKSIZE);
        m_pEncryption = new OFB_Mode<AES>::Encryption(m_pbKey, AES::MIN_KEYLENGTH, m_pbIv);
        m_pEncryptor = new StreamTransformationFilter(*((OFB_Mode<AES>::Encryption*)m_pEncryption), new StringSink(m_sCipherText));
        break;
    }

    case AES_CTR_256: {
        memcpy(m_pbKey, pbKey, AES::MAX_KEYLENGTH);
        memcpy(m_pbIv, pbIv, AES::BLOCKSIZE);
        m_pEncryption = new CTR_Mode<AES>::Encryption(m_pbKey, AES::MAX_KEYLENGTH, m_pbIv);
        m_pEncryptor = new StreamTransformationFilter(*((CTR_Mode<AES>::Encryption*)m_pEncryption), new StringSink(m_sCipherText));
        break;
    }

    case AES_CTR_192: {
        memcpy(m_pbKey, pbKey, 24);
        memcpy(m_pbIv, pbIv, AES::BLOCKSIZE);
        m_pEncryption = new CTR_Mode<AES>::Encryption(m_pbKey, 24, m_pbIv);
        m_pEncryptor = new StreamTransformationFilter(*((CTR_Mode<AES>::Encryption*)m_pEncryption), new StringSink(m_sCipherText));
        break;
    }

    case AES_CTR_128: {
        memcpy(m_pbKey, pbKey, AES::MIN_KEYLENGTH);
        memcpy(m_pbIv, pbIv, AES::BLOCKSIZE);
        m_pEncryption = new CTR_Mode<AES>::Encryption(m_pbKey, AES::MIN_KEYLENGTH, m_pbIv);
        m_pEncryptor = new StreamTransformationFilter(*((CTR_Mode<AES>::Encryption*)m_pEncryption), new StringSink(m_sCipherText));
        break;
    }

    //case AES_CBC_CTS_256:{
    //    memcpy(m_pbKey, pbKey, AES::MAX_KEYLENGTH);
    //    memcpy(m_pbIv, pbIv, AES::BLOCKSIZE);
    //    m_pEncryption = new CBC_CTS_Mode<AES>::Encryption(m_pbKey, AES::MAX_KEYLENGTH, m_pbIv);
    //    m_pEncryptor = new StreamTransformationFilter(*((CBC_CTS_Mode<AES>::Encryption*)m_pEncryption), new StringSink(m_sCipherText));
    //    break;
    //}

    //case AES_CBC_CTS_192:{
    //    memcpy(m_pbKey, pbKey, 24);
    //    memcpy(m_pbIv, pbIv, AES::BLOCKSIZE);
    //    m_pEncryption = new CBC_CTS_Mode<AES>::Encryption(m_pbKey, 24, m_pbIv);
    //    m_pEncryptor = new StreamTransformationFilter(*((CBC_CTS_Mode<AES>::Encryption*)m_pEncryption), new StringSink(m_sCipherText));
    //    break;
    //}

    //case AES_CBC_CTS_128:{
    //    memcpy(m_pbKey, pbKey, AES::MIN_KEYLENGTH);
    //    memcpy(m_pbIv, pbIv, AES::BLOCKSIZE);
    //    m_pEncryption = new CBC_CTS_Mode<AES>::Encryption(m_pbKey, AES::MIN_KEYLENGTH, m_pbIv);
    //    m_pEncryptor = new StreamTransformationFilter(*((CBC_CTS_Mode<AES>::Encryption*)m_pEncryption), new StringSink(m_sCipherText));
    //    break;
    //}

    case AES_CFB_FIPS_256: {
        memcpy(m_pbKey, pbKey, AES::MAX_KEYLENGTH);
        memcpy(m_pbIv, pbIv, AES::BLOCKSIZE);
        m_pEncryption = new CFB_FIPS_Mode<AES>::Encryption(m_pbKey, AES::MAX_KEYLENGTH, m_pbIv);
        m_pEncryptor = new StreamTransformationFilter(*((CFB_FIPS_Mode<AES>::Encryption*)m_pEncryption), new StringSink(m_sCipherText));
        break;
    }

    case AES_CFB_FIPS_192: {
        memcpy(m_pbKey, pbKey, 24);
        memcpy(m_pbIv, pbIv, AES::BLOCKSIZE);
        m_pEncryption = new CFB_FIPS_Mode<AES>::Encryption(m_pbKey, 24, m_pbIv);
        m_pEncryptor = new StreamTransformationFilter(*((CFB_FIPS_Mode<AES>::Encryption*)m_pEncryption), new StringSink(m_sCipherText));
        break;
    }

    case AES_CFB_FIPS_128: {
        memcpy(m_pbKey, pbKey, AES::MIN_KEYLENGTH);
        memcpy(m_pbIv, pbIv, AES::BLOCKSIZE);
        m_pEncryption = new CFB_FIPS_Mode<AES>::Encryption(m_pbKey, AES::MIN_KEYLENGTH, m_pbIv);
        m_pEncryptor = new StreamTransformationFilter(*((CFB_FIPS_Mode<AES>::Encryption*)m_pEncryption), new StringSink(m_sCipherText));
        break;
    }

    #pragma endregion


    default:
        throw ("未知密码算法！");
    }

    return true;
}


string CEncrypt::Encrypt(PBYTE pbPlainText, DWORD dwPlainTextLength) {
    string sCipherText;

    switch (m_iCryptoAlg) {
    
    // 不加密
    case PLAIN_TEXT: {
        sCipherText = string((PCHAR)pbPlainText, dwPlainTextLength);
        return sCipherText;
    }

    // ChaCha系列
    case CHACHA_20:
    case CHACHA_12:
    case CHACHA_8:
    case CHACHA_TLS:

    // HC系列
    case HC_256:
    case HC_128:

    // AES系列
    case AES_ECB_256:
    case AES_ECB_192:
    case AES_ECB_128:
    case AES_CBC_256:
    case AES_CBC_192:
    case AES_CBC_128:
    case AES_CFB_256:
    case AES_CFB_192:
    case AES_CFB_128:
    case AES_OFB_256:
    case AES_OFB_192:
    case AES_OFB_128:
    case AES_CTR_256:
    case AES_CTR_192:
    case AES_CTR_128:
    //case AES_CBC_CTS_256:
    //case AES_CBC_CTS_192:
    //case AES_CBC_CTS_128:
    case AES_CFB_FIPS_256:
    case AES_CFB_FIPS_192:
    case AES_CFB_FIPS_128:
    {
        m_pEncryptor->Put(pbPlainText, dwPlainTextLength);
        m_pEncryptor->MessageEnd();
        break;
    }

    default:
        throw ("未知密码算法！");
    }

    sCipherText = m_sCipherText;
    m_sCipherText = "";
    return sCipherText;
}



// ************************** 解密 **************************

CDecrypt::CDecrypt() {
    ;
}


CDecrypt::~CDecrypt() {
    if (m_pbKey != nullptr) {
        delete[] m_pbKey;
    }

    if (m_pbIv != nullptr) {
        delete[] m_pbIv;
    }

    if (m_pDecryptor != nullptr) {
        delete m_pDecryptor;
    }

    if (m_pDecryption != nullptr) {
        delete m_pDecryption;
    }
}


/// @brief Init()中会截取pbKey和pbIv，所以不用担心传入的Key和Iv过长，但需要保证Key和Iv不能短于算法所需长度。
BOOL CDecrypt::Init(CRYPTO_ALG iCryptoAlg, PBYTE pbKey, PBYTE pbIv) {
    m_iCryptoAlg = iCryptoAlg;

    m_pbKey = new BYTE[MAX_KEY_LEN];
    m_pbIv = new BYTE[MAX_IV_LEN];
    memset(m_pbKey, 0, MAX_KEY_LEN);
    memset(m_pbIv, 0, MAX_IV_LEN);

    switch (m_iCryptoAlg) {

    // 不加密
    case PLAIN_TEXT: {
        break;
    }


    #pragma region ChaCha系列
    // ChaCha系列
    case CHACHA_20: {
        memcpy(m_pbKey, pbKey, ChaCha::MAX_KEYLENGTH);
        memcpy(m_pbIv, pbIv, 8);
        m_pDecryption = new ChaCha::Decryption();
        ((ChaCha::Decryption*)m_pDecryption)->SetKeyWithIV(m_pbKey, ChaCha::MAX_KEYLENGTH, m_pbIv, 8);
        m_pDecryptor = new StreamTransformationFilter(*((ChaCha::Decryption*)m_pDecryption), new StringSink(m_sPlainText));
        break;
    }

    case CHACHA_12: {
        memcpy(m_pbKey, pbKey, ChaCha::MAX_KEYLENGTH);
        memcpy(m_pbIv, pbIv, 8);
        string sIv((PCHAR)m_pbIv, 8);
        m_AlgParams = MakeParameters(Name::Rounds(), 12) (Name::IV(), ConstByteArrayParameter(sIv, 8));
        m_pDecryption = new ChaCha::Decryption();
        ((ChaCha::Decryption*)m_pDecryption)->SetKey(m_pbKey, ChaCha::MAX_KEYLENGTH, m_AlgParams);
        m_pDecryptor = new StreamTransformationFilter(*((ChaCha::Decryption*)m_pDecryption), new StringSink(m_sPlainText));
        break;
    }

    case CHACHA_8: {
        memcpy(m_pbKey, pbKey, ChaCha::MAX_KEYLENGTH);
        memcpy(m_pbIv, pbIv, 8);
        string sIv((PCHAR)m_pbIv, 8);
        m_AlgParams = MakeParameters(Name::Rounds(), 8) (Name::IV(), ConstByteArrayParameter(sIv, 8));
        m_pDecryption = new ChaCha::Decryption();
        ((ChaCha::Decryption*)m_pDecryption)->SetKey(m_pbKey, ChaCha::MAX_KEYLENGTH, m_AlgParams);
        m_pDecryptor = new StreamTransformationFilter(*((ChaCha::Decryption*)m_pDecryption), new StringSink(m_sPlainText));
        break;
    }
    case CHACHA_TLS: {
        memcpy(m_pbKey, pbKey, ChaChaTLS::MAX_KEYLENGTH);
        memcpy(m_pbIv, pbIv, 12);
        m_pDecryption = new ChaChaTLS::Decryption();
        ((ChaChaTLS::Decryption*)m_pDecryption)->SetKeyWithIV(m_pbKey, ChaChaTLS::MAX_KEYLENGTH, m_pbIv, 12);
        m_pDecryptor = new StreamTransformationFilter(*((ChaChaTLS::Decryption*)m_pDecryption), new StringSink(m_sPlainText));
        break;
    }
    #pragma endregion


    // HC系列
    case HC_256: {
        memcpy(m_pbKey, pbKey, HC256::MAX_KEYLENGTH);
        memcpy(m_pbIv, pbIv, 32);
        m_pDecryption = new HC256::Decryption();
        ((HC256::Decryption*)m_pDecryption)->SetKeyWithIV(m_pbKey, HC256::MAX_KEYLENGTH, m_pbIv, 32);
        m_pDecryptor = new StreamTransformationFilter(*((HC256::Decryption*)m_pDecryption), new StringSink(m_sPlainText));
        break;
    }
    
    case HC_128: {
        memcpy(m_pbKey, pbKey, HC128::MAX_KEYLENGTH);
        memcpy(m_pbIv, pbIv, 16);
        m_pDecryption = new HC128::Decryption();
        ((HC128::Decryption*)m_pDecryption)->SetKeyWithIV(m_pbKey, HC128::MAX_KEYLENGTH, m_pbIv, 16);
        m_pDecryptor = new StreamTransformationFilter(*((HC128::Decryption*)m_pDecryption), new StringSink(m_sPlainText));
        break;
    }


    #pragma region AES系列
    // AES系列
    case AES_ECB_256: {
        memcpy(m_pbKey, pbKey, AES::MAX_KEYLENGTH);
        m_pDecryption = new ECB_Mode<AES>::Decryption(m_pbKey, AES::MAX_KEYLENGTH);
        m_pDecryptor = new StreamTransformationFilter(*((ECB_Mode<AES>::Decryption*)m_pDecryption), new StringSink(m_sPlainText));
        break;
    }

    case AES_ECB_192: {
        memcpy(m_pbKey, pbKey, 24);
        m_pDecryption = new ECB_Mode<AES>::Decryption(m_pbKey, 24);
        m_pDecryptor = new StreamTransformationFilter(*((ECB_Mode<AES>::Decryption*)m_pDecryption), new StringSink(m_sPlainText));
        break;
    }

    case AES_ECB_128: {
        memcpy(m_pbKey, pbKey, AES::MIN_KEYLENGTH);
        m_pDecryption = new ECB_Mode<AES>::Decryption(m_pbKey, AES::MIN_KEYLENGTH);
        m_pDecryptor = new StreamTransformationFilter(*((ECB_Mode<AES>::Decryption*)m_pDecryption), new StringSink(m_sPlainText));
        break;
    }

    case AES_CBC_256: {
        memcpy(m_pbKey, pbKey, AES::MAX_KEYLENGTH);
        memcpy(m_pbIv, pbIv, AES::BLOCKSIZE);
        m_pDecryption = new CBC_Mode<AES>::Decryption(m_pbKey, AES::MAX_KEYLENGTH, m_pbIv);
        m_pDecryptor = new StreamTransformationFilter(*((CBC_Mode<AES>::Decryption*)m_pDecryption), new StringSink(m_sPlainText));
        break;
    }

    case AES_CBC_192: {
        memcpy(m_pbKey, pbKey, 24);
        memcpy(m_pbIv, pbIv, AES::BLOCKSIZE);
        m_pDecryption = new CBC_Mode<AES>::Decryption(m_pbKey, 24, m_pbIv);
        m_pDecryptor = new StreamTransformationFilter(*((CBC_Mode<AES>::Decryption*)m_pDecryption), new StringSink(m_sPlainText));
        break;
    }

    case AES_CBC_128: {
        memcpy(m_pbKey, pbKey, AES::MIN_KEYLENGTH);
        memcpy(m_pbIv, pbIv, AES::BLOCKSIZE);
        m_pDecryption = new CBC_Mode<AES>::Decryption(m_pbKey, AES::MIN_KEYLENGTH, m_pbIv);
        m_pDecryptor = new StreamTransformationFilter(*((CBC_Mode<AES>::Decryption*)m_pDecryption), new StringSink(m_sPlainText));
        break;
    }

    case AES_CFB_256: {
        memcpy(m_pbKey, pbKey, AES::MAX_KEYLENGTH);
        memcpy(m_pbIv, pbIv, AES::BLOCKSIZE);
        m_pDecryption = new CFB_Mode<AES>::Decryption(m_pbKey, AES::MAX_KEYLENGTH, m_pbIv);
        m_pDecryptor = new StreamTransformationFilter(*((CFB_Mode<AES>::Decryption*)m_pDecryption), new StringSink(m_sPlainText));
        break;
    }

    case AES_CFB_192: {
        memcpy(m_pbKey, pbKey, 24);
        memcpy(m_pbIv, pbIv, AES::BLOCKSIZE);
        m_pDecryption = new CFB_Mode<AES>::Decryption(m_pbKey, 24, m_pbIv);
        m_pDecryptor = new StreamTransformationFilter(*((CFB_Mode<AES>::Decryption*)m_pDecryption), new StringSink(m_sPlainText));
        break;
    }

    case AES_CFB_128: {
        memcpy(m_pbKey, pbKey, AES::MIN_KEYLENGTH);
        memcpy(m_pbIv, pbIv, AES::BLOCKSIZE);
        m_pDecryption = new CFB_Mode<AES>::Decryption(m_pbKey, AES::MIN_KEYLENGTH, m_pbIv);
        m_pDecryptor = new StreamTransformationFilter(*((CFB_Mode<AES>::Decryption*)m_pDecryption), new StringSink(m_sPlainText));
        break;
    }

    case AES_OFB_256: {
        memcpy(m_pbKey, pbKey, AES::MAX_KEYLENGTH);
        memcpy(m_pbIv, pbIv, AES::BLOCKSIZE);
        m_pDecryption = new OFB_Mode<AES>::Decryption(m_pbKey, AES::MAX_KEYLENGTH, m_pbIv);
        m_pDecryptor = new StreamTransformationFilter(*((OFB_Mode<AES>::Decryption*)m_pDecryption), new StringSink(m_sPlainText));
        break;
    }

    case AES_OFB_192: {
        memcpy(m_pbKey, pbKey, 24);
        memcpy(m_pbIv, pbIv, AES::BLOCKSIZE);
        m_pDecryption = new OFB_Mode<AES>::Decryption(m_pbKey, 24, m_pbIv);
        m_pDecryptor = new StreamTransformationFilter(*((OFB_Mode<AES>::Decryption*)m_pDecryption), new StringSink(m_sPlainText));
        break;
    }

    case AES_OFB_128: {
        memcpy(m_pbKey, pbKey, AES::MIN_KEYLENGTH);
        memcpy(m_pbIv, pbIv, AES::BLOCKSIZE);
        m_pDecryption = new OFB_Mode<AES>::Decryption(m_pbKey, AES::MIN_KEYLENGTH, m_pbIv);
        m_pDecryptor = new StreamTransformationFilter(*((OFB_Mode<AES>::Decryption*)m_pDecryption), new StringSink(m_sPlainText));
        break;
    }

    case AES_CTR_256: {
        memcpy(m_pbKey, pbKey, AES::MAX_KEYLENGTH);
        memcpy(m_pbIv, pbIv, AES::BLOCKSIZE);
        m_pDecryption = new CTR_Mode<AES>::Decryption(m_pbKey, AES::MAX_KEYLENGTH, m_pbIv);
        m_pDecryptor = new StreamTransformationFilter(*((CTR_Mode<AES>::Decryption*)m_pDecryption), new StringSink(m_sPlainText));
        break;
    }

    case AES_CTR_192: {
        memcpy(m_pbKey, pbKey, 24);
        memcpy(m_pbIv, pbIv, AES::BLOCKSIZE);
        m_pDecryption = new CTR_Mode<AES>::Decryption(m_pbKey, 24, m_pbIv);
        m_pDecryptor = new StreamTransformationFilter(*((CTR_Mode<AES>::Decryption*)m_pDecryption), new StringSink(m_sPlainText));
        break;
    }

    case AES_CTR_128: {
        memcpy(m_pbKey, pbKey, AES::MIN_KEYLENGTH);
        memcpy(m_pbIv, pbIv, AES::BLOCKSIZE);
        m_pDecryption = new CTR_Mode<AES>::Decryption(m_pbKey, AES::MIN_KEYLENGTH, m_pbIv);
        m_pDecryptor = new StreamTransformationFilter(*((CTR_Mode<AES>::Decryption*)m_pDecryption), new StringSink(m_sPlainText));
        break;
    }

    //case AES_CBC_CTS_256:{
    //    memcpy(m_pbKey, pbKey, AES::MAX_KEYLENGTH);
    //    memcpy(m_pbIv, pbIv, AES::BLOCKSIZE);
    //    m_pDecryption = new CBC_CTS_Mode<AES>::Decryption(m_pbKey, AES::MAX_KEYLENGTH, m_pbIv);
    //    m_pDecryptor = new StreamTransformationFilter(*((CBC_CTS_Mode<AES>::Decryption*)m_pDecryption), new StringSink(m_sPlainText));
    //    break;
    //}

    //case AES_CBC_CTS_192:{
    //    memcpy(m_pbKey, pbKey, 24);
    //    memcpy(m_pbIv, pbIv, AES::BLOCKSIZE);
    //    m_pDecryption = new CBC_CTS_Mode<AES>::Decryption(m_pbKey, 24, m_pbIv);
    //    m_pDecryptor = new StreamTransformationFilter(*((CBC_CTS_Mode<AES>::Decryption*)m_pDecryption), new StringSink(m_sPlainText));
    //    break;
    //}

    //case AES_CBC_CTS_128:{
    //    memcpy(m_pbKey, pbKey, AES::MIN_KEYLENGTH);
    //    memcpy(m_pbIv, pbIv, AES::BLOCKSIZE);
    //    m_pDecryption = new CBC_CTS_Mode<AES>::Decryption(m_pbKey, AES::MIN_KEYLENGTH, m_pbIv);
    //    m_pDecryptor = new StreamTransformationFilter(*((CBC_CTS_Mode<AES>::Decryption*)m_pDecryption), new StringSink(m_sPlainText));
    //    break;
    //}

    case AES_CFB_FIPS_256: {
        memcpy(m_pbKey, pbKey, AES::MAX_KEYLENGTH);
        memcpy(m_pbIv, pbIv, AES::BLOCKSIZE);
        m_pDecryption = new CFB_FIPS_Mode<AES>::Decryption(m_pbKey, AES::MAX_KEYLENGTH, m_pbIv);
        m_pDecryptor = new StreamTransformationFilter(*((CFB_FIPS_Mode<AES>::Decryption*)m_pDecryption), new StringSink(m_sPlainText));
        break;
    }

    case AES_CFB_FIPS_192: {
        memcpy(m_pbKey, pbKey, 24);
        memcpy(m_pbIv, pbIv, AES::BLOCKSIZE);
        m_pDecryption = new CFB_FIPS_Mode<AES>::Decryption(m_pbKey, 24, m_pbIv);
        m_pDecryptor = new StreamTransformationFilter(*((CFB_FIPS_Mode<AES>::Decryption*)m_pDecryption), new StringSink(m_sPlainText));
        break;
    }

    case AES_CFB_FIPS_128: {
        memcpy(m_pbKey, pbKey, AES::MIN_KEYLENGTH);
        memcpy(m_pbIv, pbIv, AES::BLOCKSIZE);
        m_pDecryption = new CFB_FIPS_Mode<AES>::Decryption(m_pbKey, AES::MIN_KEYLENGTH, m_pbIv);
        m_pDecryptor = new StreamTransformationFilter(*((CFB_FIPS_Mode<AES>::Decryption*)m_pDecryption), new StringSink(m_sPlainText));
        break;
    }

    #pragma endregion


    default:
        throw ("未知密码算法！");
    }

    return true;
}


string CDecrypt::Decrypt(PBYTE pbCipherText, DWORD dwCipherTextLength) {
    string sPlainText;

    switch (m_iCryptoAlg) {

    // 不加密
    case PLAIN_TEXT: {
        sPlainText = string((PCHAR)pbCipherText, dwCipherTextLength);
        return sPlainText;
    }


    // ChaCha系列
    case CHACHA_20:
    case CHACHA_12:
    case CHACHA_8:
    case CHACHA_TLS:

    // HC系列
    case HC_256:
    case HC_128:

    // AES系列
    case AES_ECB_256:
    case AES_ECB_192:
    case AES_ECB_128:
    case AES_CBC_256:
    case AES_CBC_192:
    case AES_CBC_128:
    case AES_CFB_256:
    case AES_CFB_192:
    case AES_CFB_128:
    case AES_OFB_256:
    case AES_OFB_192:
    case AES_OFB_128:
    case AES_CTR_256:
    case AES_CTR_192:
    case AES_CTR_128:
    //case AES_CBC_CTS_256:
    //case AES_CBC_CTS_192:
    //case AES_CBC_CTS_128:
    case AES_CFB_FIPS_256:
    case AES_CFB_FIPS_192:
    case AES_CFB_FIPS_128:
    {
        // 有的密码算法在解密时，存在产生异常的可能。
        // 比如AES ECB，当输入不是BLOCK_SIZE的整数倍时。
        // 又因无法在要求对象展开的函数中使用__try，
        // 故另起一个函数
        DecryptBody(pbCipherText, dwCipherTextLength);
        break;
    }

    default:
        throw ("未知密码算法！");
    }

    sPlainText = m_sPlainText;
    m_sPlainText = "";
    return sPlainText;
}


VOID CDecrypt::DecryptBody(PBYTE pbCipherText, DWORD dwCipherTextLength) {
    __try {
        m_pDecryptor->Put(pbCipherText, dwCipherTextLength);
        m_pDecryptor->MessageEnd();
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        DebugPrint("[ERROR] 解密出现异常，返回空字符串\n");
        m_sPlainText = "";
    }
}


// ************************** 测试 **************************

int main() {

    // key和iv如果超出密码算法需要的长度，会在CEncrypt和CDecrypt的Init()中截断至需要的长度，所以不用担心过长。但需要保证不能短于需要的长度。
    BYTE plainText[] = "What makes the desert beautiful is that somewhere it hides a well.";
    BYTE plainText3[] = "Flowers are so inconsistent! But I was too young to know how to love her.";
    BYTE Key[] = "0123456789ABCDEF0123456789ABCDEF";        // 256 bits
    BYTE IV[] = "ABCDEF01234567890123456789ABCDEF";          // 256 bits
   
    
    // 可在此处更改选用的算法
    #define USE_CRYPTO_ALG HC_256


    // 加密初始化
    CEncrypt Enc;
    Enc.Init(USE_CRYPTO_ALG, Key, IV);

    // 第一次加密
    string sCipherText1 = Enc.Encrypt(plainText, strlen((char*)plainText));
    PrintData((PBYTE)sCipherText1.c_str(), sCipherText1.length());

    // 第二次加密
    string sCipherText2 = Enc.Encrypt(plainText, strlen((char*)plainText));
    PrintData((PBYTE)sCipherText2.c_str(), sCipherText2.length());

    // 第三次加密
    string sCipherText3 = Enc.Encrypt(plainText3, strlen((char*)plainText3));
    PrintData((PBYTE)sCipherText3.c_str(), sCipherText3.length());


    // 解密初始化
    CDecrypt Dec;
    Dec.Init(USE_CRYPTO_ALG, Key, IV);

    // 第一次解密
    string sPlainText1 = Dec.Decrypt((PBYTE)sCipherText1.c_str(), sCipherText1.length());
    PrintData((PBYTE)sPlainText1.c_str(), sPlainText1.length());

    // 第二次解密
    string sPlainText2 = Dec.Decrypt((PBYTE)sCipherText2.c_str(), sCipherText2.length());
    PrintData((PBYTE)sPlainText2.c_str(), sPlainText2.length());

    // 第三次解密
    string sPlainText3 = Dec.Decrypt((PBYTE)sCipherText3.c_str(), sCipherText3.length());
    PrintData((PBYTE)sPlainText3.c_str(), sPlainText3.length());


    RsaTest();


    getchar();
}