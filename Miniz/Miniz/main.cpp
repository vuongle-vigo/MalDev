#include <windows.h>
#include <filesystem>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>

#include "zip/src/zip.h"

namespace fs = std::filesystem;

static std::string PathToUtf8(const fs::path& p) {
#if defined(__cpp_char8_t)
    std::u8string u8 = p.generic_u8string();
    return std::string(reinterpret_cast<const char*>(u8.c_str()), u8.size());
#else
    return p.generic_u8string();
#endif
}

bool ReadFileToBuffer(const fs::path& filePath, std::vector<char>& buffer) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        return false;
    }

    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (size < 0) {
        return false;
    }

    buffer.resize(static_cast<size_t>(size));

    if (size == 0) {
        return true;
    }

    return file.read(buffer.data(), size).good();
}

bool ZipFolder(const fs::path& folderPath, const fs::path& zipPath) {
    if (!fs::exists(folderPath) || !fs::is_directory(folderPath)) {
        std::cout << "Folder not found\n";
        return false;
    }

    fs::path absFolderPath = fs::absolute(folderPath);
    fs::path absZipPath = fs::absolute(zipPath);

    if (fs::exists(absZipPath)) {
        fs::remove(absZipPath);
    }

    // zip_open vẫn cần char*, nên output zip path nên để ASCII đơn giản.
    // Ví dụ: C:\Users\vuong\Desktop\b.zip
    std::string zipPathStr = absZipPath.string();

    struct zip_t* zip = zip_open(
        zipPathStr.c_str(),
        ZIP_DEFAULT_COMPRESSION_LEVEL,
        'w'
    );

    if (!zip) {
        std::cout << "zip_open failed\n";
        return false;
    }

    bool ok = true;

    try {
        for (const auto& entry : fs::recursive_directory_iterator(absFolderPath)) {
            if (!entry.is_regular_file()) {
                continue;
            }

            fs::path fullPath = fs::absolute(entry.path());

            if (fullPath == absZipPath) {
                std::cout << "Skip output zip\n";
                continue;
            }

            fs::path relativePath = fs::relative(fullPath, absFolderPath);

            // Tên trong zip dùng UTF-8
            std::string relUtf8 = PathToUtf8(relativePath);

            std::wcout << L"Add: " << fullPath.wstring()
                << L" -> " << relativePath.wstring() << L"\n";

            std::vector<char> data;
            if (!ReadFileToBuffer(fullPath, data)) {
                std::wcout << L"Read failed: " << fullPath.wstring() << L"\n";
                ok = false;
                break;
            }

            if (zip_entry_open(zip, relUtf8.c_str()) != 0) {
                std::cout << "zip_entry_open failed: " << relUtf8 << "\n";
                ok = false;
                break;
            }

            if (!data.empty()) {
                if (zip_entry_write(zip, data.data(), data.size()) != 0) {
                    std::cout << "zip_entry_write failed\n";
                    zip_entry_close(zip);
                    ok = false;
                    break;
                }
            }

            if (zip_entry_close(zip) != 0) {
                std::cout << "zip_entry_close failed\n";
                ok = false;
                break;
            }
        }
    }
    catch (const std::exception& e) {
        std::cout << "Exception: " << e.what() << "\n";
        ok = false;
    }

    std::cout << "Before zip_close\n";
    zip_close(zip);
    std::cout << "After zip_close\n";

    return ok && fs::exists(absZipPath) && fs::file_size(absZipPath) > 0;
}

int main() {
    fs::path folder = LR"(C:\Users\vuong\Downloads\b)";
    fs::path zipFile = LR"(C:\Users\vuong\Desktop\b.zip)";

    if (ZipFolder(folder, zipFile)) {
        std::cout << "Zip OK\n";
    }
    else {
        std::cout << "Zip failed\n";
    }

    return 0;
}