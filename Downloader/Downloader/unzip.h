#pragma once
bool unzip_single_file(
    const wchar_t* zipPath,
    const wchar_t* innerFileName,
    const wchar_t* outputFolder
);