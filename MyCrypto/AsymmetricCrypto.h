#pragma once

#include <cstring>
using namespace std;

extern void RsaGenerateKey(int iKeyLen, string& sPublicKey, string& sPrivateKey);
extern string RsaEncryptUsePublicKey(string sPublicKey, string sPlainText);
extern string RsaDecryptUsePrivateKey(string sPrivateKey, string sCipherText);
extern string RsaEncryptUsePrivateKey(string sPrivateKey, string sPlainText);
extern string RsaDecryptUsePublicKey(string sPublicKey, string sCipherText);

VOID RsaTest();