#include "pch.h"
#include "Misc.h"

#include "Include/rsa.h"
#include "Include/base64.h"
#include "Include/osrng.h"				// AutoSeededRandomPool

using namespace std;
using namespace CryptoPP;


// 生成RSA密钥
void RsaGenerateKey(int iKeyLen, string& sPublicKey, string& sPrivateKey) {
	// 伪随机数生成器
	AutoSeededRandomPool rng;

	// 生成私钥
	RSA::PrivateKey rsaPrivateKey;
	rsaPrivateKey.GenerateRandomWithKeySize(rng, iKeyLen);

	// 根据私钥生成公钥
	RSA::PublicKey rsaPublicKey(rsaPrivateKey);

	// 密钥以string形式存储
	string sPublicKeyTemp, sPrivateKeyTemp;
	rsaPublicKey.Save(StringSink(sPublicKeyTemp).Ref());
	rsaPrivateKey.Save(StringSink(sPrivateKeyTemp).Ref());

	// Base64编码密钥
	StringSource ss1(sPublicKeyTemp, true, new Base64Encoder(new StringSink(sPublicKey)));
	StringSource ss2(sPrivateKeyTemp, true, new Base64Encoder(new StringSink(sPrivateKey)));
}


// 从Base64字符串加载公钥或私钥
template <typename Key>
const Key LoadKeyFromBase64Str(const std::string& str)
{
	Key key;
	StringSource source(str.c_str(), true, new Base64Decoder);
	key.Load(source);
	return key;
}


string RsaEncryptUsePublicKey(string sPublicKey, string sPlainText) {
	RSA::PublicKey rsaPublicKey = LoadKeyFromBase64Str<RSA::PublicKey>(sPublicKey);
	AutoSeededRandomPool prng;

	// 加密
	string sCipherText;
	RSAES_OAEP_SHA_Encryptor Enc(rsaPublicKey);
	StringSource ss1(sPlainText, true,
		new PK_EncryptorFilter(prng, Enc,
			new StringSink(sCipherText)
		) // PK_EncryptorFilter
	); // StringSource

	return sCipherText;
}


string RsaDecryptUsePrivateKey(string sPrivateKey, string sCipherText) {
	RSA::PrivateKey rsaPrivateKey = LoadKeyFromBase64Str<RSA::PrivateKey>(sPrivateKey);
	AutoSeededRandomPool prng;

	// 解密
	string sPlainText;
	RSAES_OAEP_SHA_Decryptor Dec(rsaPrivateKey);
	StringSource ss2(sCipherText, true,
		new PK_DecryptorFilter(prng, Dec,
			new StringSink(sPlainText)
		) // PK_DecryptorFilter
	); // StringSource

	return sPlainText;
}


// Crypto++不支持私钥加密，公钥解密, 可见 https://www.cryptopp.com/wiki/Raw_RSA
// 所以我只能手动模拟计算，因而此函数的安全性未知。
string RsaEncryptUsePrivateKey(string sPrivateKey, string sPlainText) {


	//RSA::PrivateKey rsaPrivateKey = LoadKeyFromBase64Str<RSA::PrivateKey>(sPrivateKey);
	//AutoSeededRandomPool prng;

	//string sCipherText;
	//RSAES_OAEP_SHA_Encryptor Enc(rsaPrivateKey);
	//StringSource ss1(sPlainText, true,
	//	new PK_EncryptorFilter(prng, Enc,
	//		new StringSink(sCipherText)
	//	) // PK_EncryptorFilter
	//); // StringSource

	//return sCipherText;

	RSA::PrivateKey rsaPrivateKey = LoadKeyFromBase64Str<RSA::PrivateKey>(sPrivateKey);

	const Integer& n = rsaPrivateKey.GetModulus();
	const Integer& d = rsaPrivateKey.GetPrivateExponent();

	Integer m((const byte*)sPlainText.data(), sPlainText.size());
	Integer c = a_exp_b_mod_c(m, d, n);

	size_t req = c.MinEncodedSize();
	string sCipherText;
	sCipherText.resize(req);
	c.Encode((byte*)&sCipherText[0], sCipherText.size());

	return sCipherText;
}


// Crypto++不支持私钥加密，公钥解密, 可见 https://www.cryptopp.com/wiki/Raw_RSA
// 所以我只能手动模拟计算，因而此函数的安全性未知。
string RsaDecryptUsePublicKey(string sPublicKey, string sCipherText) {
	//RSA::PublicKey rsaPublicKey = LoadKeyFromBase64Str<RSA::PublicKey>(sPublicKey);
	//AutoSeededRandomPool prng;

	//string sPlainText;
	//RSAES_OAEP_SHA_Decryptor Dec(rsaPublicKey);
	//StringSource ss2(sCipherText, true,
	//	new PK_DecryptorFilter(prng, Dec,
	//		new StringSink(sPlainText)
	//	) // PK_DecryptorFilter
	//); // StringSource

	//return sPlainText;

	RSA::PublicKey rsaPublicKey = LoadKeyFromBase64Str<RSA::PublicKey>(sPublicKey);

	const Integer& n = rsaPublicKey.GetModulus();
	const Integer& e = rsaPublicKey.GetPublicExponent();

	Integer c((const byte*)sCipherText.data(), sCipherText.size());
	Integer m = a_exp_b_mod_c(c, e, n);

	size_t req = m.MinEncodedSize();
	string sPlainText;
	sPlainText.resize(req);
	m.Encode((byte*)&sPlainText[0], sPlainText.size());

	return sPlainText;
}


// 测试
VOID RsaTest() {
	// 生成密钥
	string sPublicKey, sPrivateKey;
	RsaGenerateKey(3072, sPublicKey, sPrivateKey);

	printf("公钥：\n");
	cout << sPublicKey;

	printf("私钥：\n");
	cout << sPrivateKey;


	// 公钥加密，私钥解密
	string plain = "RSA Encryption Use Public Key!", cipher, recovered;

	cipher = RsaEncryptUsePublicKey(sPublicKey, plain);
	printf("密文：\n");
	PrintData((PBYTE)cipher.c_str(), cipher.length());

	recovered = RsaDecryptUsePrivateKey(sPrivateKey, cipher);
	printf("恢复的明文：\n");
	PrintData((PBYTE)recovered.c_str(), recovered.length());


	// 私钥加密，公钥解密
	string plain2 = "RSA Encryption Use Private Key!", cipher2, recovered2;

	cipher2 = RsaEncryptUsePrivateKey(sPrivateKey, plain2);
	printf("密文：\n");
	PrintData((PBYTE)cipher2.c_str(), cipher2.length());

	recovered2 = RsaDecryptUsePublicKey(sPublicKey, cipher2);
	printf("恢复的明文：\n");
	PrintData((PBYTE)recovered2.c_str(), recovered2.length());
}