#include <fstream>
#include <vector>
#include <cstdint>
#include <string>

#pragma pack(push, 1)
struct BMPFileHeader {
    uint16_t bfType = 0x4D42; // 'BM'
    uint32_t bfSize = 0;
    uint16_t bfReserved1 = 0;
    uint16_t bfReserved2 = 0;
    uint32_t bfOffBits = 54;
};

struct BMPInfoHeader {
    uint32_t biSize = 40;
    int32_t  biWidth = 0;
    int32_t  biHeight = 0;
    uint16_t biPlanes = 1;
    uint16_t biBitCount = 32;
    uint32_t biCompression = 0; // BI_RGB
    uint32_t biSizeImage = 0;
    int32_t  biXPelsPerMeter = 0;
    int32_t  biYPelsPerMeter = 0;
    uint32_t biClrUsed = 0;
    uint32_t biClrImportant = 0;
};
#pragma pack(pop)

bool SaveBGRAtoBMP(
    const std::wstring& filename,
    const std::vector<unsigned char>& bgra,
    int width,
    int height
) {
    if (bgra.empty() || width <= 0 || height <= 0) {
        return false;
    }

    const uint32_t imageSize = width * height * 4;

    BMPFileHeader fileHeader;
    BMPInfoHeader infoHeader;

    fileHeader.bfSize = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader) + imageSize;

    infoHeader.biWidth = width;

    // âm để BMP lưu dạng top-down, không bị lật ngược
    infoHeader.biHeight = -height;

    infoHeader.biBitCount = 32;
    infoHeader.biSizeImage = imageSize;

    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    file.write(reinterpret_cast<const char*>(&fileHeader), sizeof(fileHeader));
    file.write(reinterpret_cast<const char*>(&infoHeader), sizeof(infoHeader));
    file.write(reinterpret_cast<const char*>(bgra.data()), bgra.size());

    return file.good();
}