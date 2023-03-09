#include "pch.h"
#include "GeneralSocket.h"

using namespace nsGeneralSocket;


CRYPTO_ALG nsGeneralSocket::GetCryptoAlgByServiceType(SERVICE_TYPE wServiceType) {
	CRYPTO_ALG iCryptoAlg;
	switch (wServiceType) {
	case MAIN_SOCKET_SERVICE:
		iCryptoAlg = HC_128;
		break;
	case REMOTE_SHELL_SERVICE:
		iCryptoAlg = AES_CFB_128;
		break;
	case FILE_TRANSFER_SERVICE:
		iCryptoAlg = CHACHA_20;
		break;
	case IMAGE_CAPTURE_SERVICE:
		iCryptoAlg = AES_OFB_192;
		break;
	case DESKTOP_MONITOR_SERVICE:
		iCryptoAlg = CHACHA_8;
		break;
	default:
		iCryptoAlg = CHACHA_12;
	}
	return iCryptoAlg;
}