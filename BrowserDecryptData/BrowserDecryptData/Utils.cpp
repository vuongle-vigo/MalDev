#include "Utils.h"

static const std::string base64_chars =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static bool IsBase64(unsigned char c) {
    return (isalnum(c) || (c == '+') || (c == '/'));
}


std::string Base64Decode(const std::string& encodedData) {
    int in_len = encodedData.size();
    int i = 0;
    int j = 0;
    int in_ = 0;
    unsigned char char_array_4[4], char_array_3[3];
    std::string ret;

    while (in_len-- && (encodedData[in_] != '=') && IsBase64(encodedData[in_])) {
        char_array_4[i++] = encodedData[in_]; in_++;
        if (i == 4) {
            for (i = 0; i < 4; i++)
                char_array_4[i] = base64_chars.find(char_array_4[i]);

            char_array_3[0] = (char_array_4[0] << 2) | (char_array_4[1] >> 4);
            char_array_3[1] = ((char_array_4[1] & 15) << 4) | (char_array_4[2] >> 2);
            char_array_3[2] = ((char_array_4[2] & 3) << 6) | char_array_4[3];

            for (i = 0; (i < 3); i++)
                ret += char_array_3[i];
            i = 0;
        }
    }

    if (i) {
        for (int j = i; j < 4; j++)
            char_array_4[j] = 0;

        for (int j = 0; j < 4; j++)
            char_array_4[j] = base64_chars.find(char_array_4[j]);

        char_array_3[0] = (char_array_4[0] << 2) | (char_array_4[1] >> 4);
        char_array_3[1] = ((char_array_4[1] & 15) << 4) | (char_array_4[2] >> 2);
        char_array_3[2] = ((char_array_4[2] & 3) << 6) | char_array_4[3];

        for (int j = 0; (j < i - 1); j++) ret += char_array_3[j];
    }

    return ret;
}