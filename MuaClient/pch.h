#pragma once

#include <string>
#include <inttypes.h>					//  PRIx64

#include "../MyHpSocket/MyHpSocket.h"
#include "../MyCrypto/MyCrypto.h"
#include "../MyMsgPack/MyMsgPack.h"


const int iRsaKeyBits = 3072;
const std::string sRsaPublicKey = 
	"MIIBoDANBgkqhkiG9w0BAQEFAAOCAY0AMIIBiAKCAYEAyaLwYQ2txLT+3Zl2E59eM+TD1W2J"
	"m+HlE4wCegBODNZX2KcGXia+dheW/+0ziAMo/6RgulS0l0cnkKzvsZAsCxptGU3Jl8grT0bq"
	"4//JuSvKOABFyPyRE8cAGMOXZnUEyZQPPQ8AQnLX3szyb38AyycPmhHpx6T1XRTvoPd37tUd"
	"1qoR83p/NXO1yy9J+mPoBNfOTujLskgT5tUxGdSx/bjIq8mkE+SmO78TPEwPQu7pm0qWU3Bl"
	"l7thwmYfNCAydc3VWf8/ZX4RDTtHRJvMk7MSxra7OBdf/7xHB1vyKQ0pzy5WuP+qEUXJz9Qt"
	"Vu7F7O4OdnzXyfhqkf0tc+kSs6VRS+YaxbE5p061ZfG6JiMHEihQQVu7jE8/gyuh2seaxrsO"
	"B+J7Qv29U+0WArPrkgkHl0i6SqFSxlNHiQwz/f7XJF738gJVKZjq2WHzrcblP8Jq4CpwpabA"
	"Obw9bLep+NXF3cqZenQFx+ALTfv8KPyQ3DHdsErFTwf7TEweBzgxAgER";


#ifndef ASSERT
	#ifdef _DEBUG
		#include <cassert>
		#define ASSERT assert
	#else
		#define ASSERT
	#endif
#endif