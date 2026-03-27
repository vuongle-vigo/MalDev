#include "Base64.h"
#include "CRT.h"

char* Base64Encode(const char* input, int length) {
    char B64Chars[] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/' };
    int output_length = 4 * ((length + 2) / 3);
    char* output;
    if (!AllocMemory(output_length + 1, (LPVOID*) & output)) {
        return NULL;
    }


    int val = 0, valb = -6;
    int out_index = 0;

    for (int i = 0; i < length; i++) {
        val = (val << 8) + input[i];
        valb += 8;
        while (valb >= 0) {
            output[out_index++] = B64Chars[(val >> valb) & 0x3F];
            valb -= 6;
        }
    }

    if (valb > -6) {
        output[out_index++] = B64Chars[((val << 8) >> (valb + 8)) & 0x3F];
    }

    while (out_index < output_length) {
        output[out_index++] = '=';
    }

    output[out_index] = '\0'; 
    return output;
}

char* Base64Decode(const char* input, int& output_length) {
    char B64Chars[] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/' };
    int length = 0;
    while (input[length] != '\0') length++; 

    int* T = NULL;
    if (!AllocMemory(256 * sizeof(int), (LPVOID*) & T)) {
        return NULL;
    }

    for (int i = 0; i < 256; i++) {
        T[i] = -1;
    }
    for (int i = 0; i < 64; i++) {
        T[(int)B64Chars[i]] = i;
    }

    int val = 0, valb = -8;
    output_length = (length / 4) * 3;
    if (input[length - 1] == '=') output_length--;
    if (input[length - 2] == '=') output_length--;

    char* output = NULL;
    if (!AllocMemory(256 * sizeof(char), (LPVOID*)&output)) {
        return NULL;
    }
    int out_index = 0;

    for (int i = 0; i < length; i++) {
        if (T[(int)input[i]] == -1) break;
        val = (val << 6) + T[(int)input[i]];
        valb += 6;
        if (valb >= 0) {
            output[out_index++] = (char)((val >> valb) & 0xFF);
            valb -= 8;
        }
    }

    output[out_index] = '\0';
    if (!FreeMemory(T)) {
        return NULL;
    }

    return output;
}