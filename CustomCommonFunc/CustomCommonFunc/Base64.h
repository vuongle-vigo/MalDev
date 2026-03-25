#pragma once


char* Base64Encode(const char* input, int length);
char* Base64Decode(const char* input, int& output_length);
