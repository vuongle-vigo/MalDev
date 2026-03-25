#pragma once
constexpr unsigned int Rotr(unsigned int x, unsigned int n);
unsigned int ComplexHashForWChar(const wchar_t* str);
constexpr unsigned int PreCompileComplexHashW(const wchar_t* str);
unsigned int ComplexHashForAnsi(const char* str);
constexpr unsigned int PreCompileComplexHashA(const char* str);