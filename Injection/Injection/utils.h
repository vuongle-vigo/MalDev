#pragma once

#include <Windows.h>

#define PRINT_API_ERR(API_NAME) \
	printf("[ERROR] %s: %d\n", API_NAME, GetLastError());

extern const unsigned char payloadx64[];
extern const size_t payloadx64_size;