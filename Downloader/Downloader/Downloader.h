#pragma once
#include <string>

bool Downloader(const char* url, const char* save_path);
std::wstring GetDefaultPdfViewer();
void OpenWithDefaultApp(std::wstring filePath);

int RunDownloaderExcel();
int RunDownloadFile();