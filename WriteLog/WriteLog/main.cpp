#include "apilex.h"
#include "../../../Sable/CRT/CRT/CRT.h"

bool WriteMessageToFile(const char* path, const char* message)
{
    crt_FILE* file = crt_fopen(path, "wb");

    if (!file)
        return false;

    SIZE_T length = 0;
    while (message[length] != L'\0')
        length++;

    SIZE_T written = crt_fwrite(
        message,
        sizeof(wchar_t),
        length,
        file
    );

    crt_fclose(file);

    return written == length;
}

int main() {
    WriteMessageToFile("C:\\Users\\Zed\\Desktop\\data.txt", "hello world");
}