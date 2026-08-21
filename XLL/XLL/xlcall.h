#pragma once

// xlcall.h - definitions for Excel XLL add-ins
#ifndef XL_CALL_H
#define XL_CALL_H

#ifdef __cplusplus
extern "C" {
#endif

    // Excel function types
    typedef struct {
        union {
            double num;
            char* str;
            unsigned char* bytes;
            unsigned short bool_;
            int err;
            struct {
                unsigned long count;
                int arrayref;
            } array;
            struct {
                int w;
                int x;
                int y;
                int z;
            } sref;
        } val;
        unsigned char type;
        unsigned char reserved;
        unsigned short cb;
    } XLOPER12;

    // Function return types
    typedef XLOPER12* LPXLOPER12;

    // Excel12 APIextern LPXLOPER12 __stdcall Excel12f(int xlfn, int argc, ...);
    extern int __stdcall Excel12(int xlfn, int argc, LPXLOPER12* operRes, ...);

    // Return constants
#define xlretSuccess 0
#define xlretAbort 1
#define xlretInvXlfn 2
#define xlretInvCount 4
#define xlretInvType 8
#define xlretInvalid 16
#define xlretFailed 64

// Data types
#define xltypeNum 1
#define xltypeStr 2
#define xltypeBool 4
#define xltypeErr 16
#define xltypeMulti 64// Error codes
#define xlerrValue 15
#define xlerrNull 0
#define xlerrRef 23
#define xlerrName 29
#define xlerrNum 36
#define xlerrNA 42

#ifdef __cplusplus
}
#endif
#endif