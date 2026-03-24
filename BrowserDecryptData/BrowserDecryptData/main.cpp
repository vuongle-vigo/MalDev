#include "Utils.h"
#include <iostream>
#include <Windows.h>
#include <fstream>
#include <bcrypt.h>

#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "Crypt32.lib")
int main() {
	std::string encryptedKeyB64 = "RFBBUEkBAAAA0Iyd3wEV0RGMegDAT8KX6wEAAAAsqWaWDrJAS4dhoYloe2S2EAAAABwAAABHAG8AbwBnAGwAZQAgAEMAaAByAG8AbQBlAAAAEGYAAAABAAAgAAAAEKECdKSAtkmSdd2Ev0xTfqmaEfllOSG78Ts+B4Md+PcAAAAADoAAAAACAAAgAAAALXs9Z7yTVjgJjJupPwY4PwYbkX+Rlef+jtvKJIoWBl8wAAAAjAjXwwuB7mpjyUqpt0X2qN1IEkfgm7TNSakTI606YFjZ2WHpBARVo8LoSKLvuNk/QAAAAH71ThRPImDdDzwT9tZQ/ETxr2YDHjsILeTPAQrY1SUQqs31qXoFdqiYYeMHNeQmMfXIivQ9oNn9lFqwOkYORN0=";
	std::string encryptedKey = Base64Decode(encryptedKeyB64);

	BYTE* bKeyEncrypted = new BYTE[encryptedKey.size() - 5];
	int keySize = encryptedKey.size() - 5;
	for (size_t i = 5; i < encryptedKey.size(); ++i) {
		bKeyEncrypted[i - 5] = static_cast<BYTE>(encryptedKey[i]);
	}

	DATA_BLOB in = { 0 };
	DATA_BLOB out = { 0 };
	in.pbData = bKeyEncrypted;
	in.cbData = keySize;
	BYTE *decryptedKey = new BYTE[keySize];
	if (CryptUnprotectData(&in, NULL, NULL, NULL, NULL, 0, &out)) {
		memcpy(decryptedKey, out.pbData, out.cbData);
		decryptedKey[out.cbData] = '\0'; // Null-terminate the decrypted key
	} else {
		std::cerr << "Decryption failed with error code: " << GetLastError() << std::endl;
		delete[] bKeyEncrypted;
		return 1;
	}

//Open and write to file
	std::fstream outputFile("decrypted_key.bin", std::ios::out | std::ios::binary);
	outputFile.write(reinterpret_cast<char*>(decryptedKey), keySize);
	outputFile.close();
}