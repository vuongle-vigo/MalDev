#pragma once
bool WriteMessageToFile(const char* path, const char* message);
char* Base64Encode(const char* input, int length);
char* Base64Decode(const char* input, int& output_length);