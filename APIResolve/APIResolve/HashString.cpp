#include "HashString.h"

// Circular rotate function (rotate right)
constexpr unsigned int Rotr(unsigned int x, unsigned int n) {
    return (x >> n) | (x << (32 - n));
}

// Function to calculate a complex hash for wchar_t* (wide character string)
unsigned int ComplexHashForWChar(const wchar_t* str) {
    unsigned int hash = 0xFFFFFFFF;  // Initial value for hash, commonly used for more complex algorithms
    unsigned int temp;

    // Traverse through each wide character in the input string
    for (int i = 0; str[i] != L'\0'; ++i) {
        temp = str[i] + hash;
        hash = Rotr(hash, 3) ^ temp;  // Rotate and apply XOR operation
    }

    return hash;
}

constexpr unsigned int PreCompileComplexHashW(const wchar_t* str) {
    unsigned int hash = 0xFFFFFFFF;  // Initial value for hash, commonly used for more complex algorithms
    unsigned int temp;

    // Traverse through each wide character in the input string
    for (int i = 0; str[i] != L'\0'; ++i) {
        temp = str[i] + hash;
        hash = Rotr(hash, 3) ^ temp;  // Rotate and apply XOR operation
    }

    return hash;
}

unsigned int ComplexHashForAnsi(const char* str) {
    unsigned int hash = 0xFFFFFFFF;  // Initial value for hash, commonly used for more complex algorithms
    unsigned int temp;

    // Traverse through each character in the input string
    for (int i = 0; str[i] != '\0'; ++i) {
        temp = str[i] + hash;
        hash = Rotr(hash, 3) ^ temp;  // Rotate and apply XOR operation
    }

    return hash;
}

constexpr unsigned int PreCompileComplexHashA(const char* str) {
    unsigned int hash = 0xFFFFFFFF;  // Initial value for hash, commonly used for more complex algorithms
    unsigned int temp;

    // Traverse through each character in the input string
    for (int i = 0; str[i] != '\0'; ++i) {
        temp = str[i] + hash;
        hash = Rotr(hash, 3) ^ temp;  // Rotate and apply XOR operation
    }

    return hash;
}