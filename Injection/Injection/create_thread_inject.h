#pragma once
#include "utils.h" 
BOOL create_remote_thread_inject(_In_ HANDLE hTargetProc, _In_ const unsigned char* shellcode);
BOOL create_thread_self_inject(_In_ const unsigned char* shellcode[]);