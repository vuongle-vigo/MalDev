#include <windows.h>
#include <shldisp.h>
#include <iostream>
#include <vector>
#include <string>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

static void PrintError(const wchar_t* msg, HRESULT hr)
{
    std::wcerr
        << msg
        << L" HRESULT=0x"
        << std::hex
        << static_cast<unsigned long>(hr)
        << std::dec
        << L"\n";
}

static bool IsDirectory(const std::wstring& path)
{
    DWORD attr = GetFileAttributesW(path.c_str());

    return attr != INVALID_FILE_ATTRIBUTES &&
        (attr & FILE_ATTRIBUTE_DIRECTORY);
}

/*
 * Trả về true nếu folder có ít nhất một file hoặc
 * một sub-folder không rỗng.
 *
 * Empty folder => false
 */
static bool HasContent(const std::wstring& path)
{
    WIN32_FIND_DATAW data{};

    std::wstring pattern = path + L"\\*";

    HANDLE hFind = FindFirstFileW(
        pattern.c_str(),
        &data
    );

    if (hFind == INVALID_HANDLE_VALUE)
        return false;

    do
    {
        if (wcscmp(data.cFileName, L".") == 0 ||
            wcscmp(data.cFileName, L"..") == 0)
        {
            continue;
        }

        std::wstring fullPath =
            path + L"\\" + data.cFileName;

        if (data.dwFileAttributes &
            FILE_ATTRIBUTE_DIRECTORY)
        {
            /*
             * Nếu sub-folder có content thì folder hiện tại
             * cũng được xem là có content.
             */
            if (HasContent(fullPath))
            {
                FindClose(hFind);
                return true;
            }
        }
        else
        {
            /*
             * Có file => folder không rỗng.
             */
            FindClose(hFind);
            return true;
        }

    } while (FindNextFileW(hFind, &data));

    FindClose(hFind);

    return false;
}

/*
 * Copy một folder vào ZIP.
 *
 * Empty folder sẽ bị bỏ qua.
 *
 * Folder không rỗng sẽ được CopyHere() và Shell ZIP
 * tự xử lý recursive.
 */
static HRESULT AddFolderToZip(
    IShellDispatch* shell,
    Folder* zipFolder,
    const std::wstring& folderPath)
{
    if (!HasContent(folderPath))
    {
        std::wcout
            << L"[SKIP EMPTY] "
            << folderPath
            << L"\n";

        return S_OK;
    }

    //
    // Open source folder
    //
    VARIANT vPath;
    VariantInit(&vPath);

    vPath.vt = VT_BSTR;
    vPath.bstrVal =
        SysAllocString(folderPath.c_str());

    Folder* sourceFolder = nullptr;

    HRESULT hr = shell->NameSpace(
        vPath,
        &sourceFolder
    );

    VariantClear(&vPath);

    if (FAILED(hr) || !sourceFolder)
        return FAILED(hr) ? hr : E_FAIL;

    //
    // Get items
    //
    FolderItems* items = nullptr;

    hr = sourceFolder->Items(&items);

    if (FAILED(hr) || !items)
    {
        sourceFolder->Release();
        return FAILED(hr) ? hr : E_FAIL;
    }

    LONG count = 0;

    hr = items->get_Count(&count);

    if (FAILED(hr))
    {
        items->Release();
        sourceFolder->Release();
        return hr;
    }

    //
    // Process từng item
    //
    for (LONG i = 0; i < count; ++i)
    {
        VARIANT index;
        VariantInit(&index);

        index.vt = VT_I4;
        index.lVal = i;

        //
        // IMPORTANT:
        // FolderItems::Item() trả về FolderItem*
        //
        FolderItem* item = nullptr;

        hr = items->Item(
            index,
            &item
        );

        VariantClear(&index);

        if (FAILED(hr) || !item)
            continue;

        //
        // Lấy path
        //
        BSTR itemPathBstr = nullptr;

        hr = item->get_Path(
            &itemPathBstr
        );

        if (FAILED(hr) || !itemPathBstr)
        {
            item->Release();
            continue;
        }

        std::wstring itemPath(
            itemPathBstr
        );

        SysFreeString(itemPathBstr);

        //
        // COM dùng VARIANT_BOOL, không phải bool.
        //
        VARIANT_BOOL isFolder = VARIANT_FALSE;

        hr = item->get_IsFolder(
            &isFolder
        );

        if (FAILED(hr))
        {
            item->Release();
            continue;
        }

        //
        // Empty folder => bỏ qua
        //
        if (isFolder == VARIANT_TRUE)
        {
            if (!HasContent(itemPath))
            {
                std::wcout
                    << L"[SKIP EMPTY] "
                    << itemPath
                    << L"\n";

                item->Release();
                continue;
            }
        }

        //
        // Copy file hoặc folder không rỗng
        //
        VARIANT vItem;
        VariantInit(&vItem);

        vItem.vt = VT_DISPATCH;

        vItem.pdispVal =
            reinterpret_cast<IDispatch*>(item);

        //
        // VARIANT cần giữ reference riêng.
        //
        item->AddRef();

        VARIANT options;
        VariantInit(&options);

        options.vt = VT_I4;

        //
        // FOF_SILENT
        //
        options.lVal = 0x0004;

        std::wcout
            << (isFolder == VARIANT_TRUE
                ? L"[FOLDER] "
                : L"[FILE]   ")
            << itemPath
            << L"\n";

        hr = zipFolder->CopyHere(
            vItem,
            options
        );

        VariantClear(&options);
        VariantClear(&vItem);

        item->Release();

        if (FAILED(hr))
        {
            std::wcerr
                << L"CopyHere failed: "
                << itemPath
                << L"\n";

            break;
        }
    }

    items->Release();
    sourceFolder->Release();

    return hr;
}


static bool CreateEmptyZip(const wchar_t* zipPath)
{
    const unsigned char emptyZip[22] =
    {
        0x50, 0x4B, 0x05, 0x06,
        0x00, 0x00,
        0x00, 0x00,
        0x00, 0x00,
        0x00, 0x00,
        0x00, 0x00,
        0x00, 0x00,
        0x00, 0x00,
        0x00, 0x00
    };

    HANDLE hFile = CreateFileW(
        zipPath,
        GENERIC_WRITE,
        0,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (hFile == INVALID_HANDLE_VALUE)
        return false;

    DWORD written = 0;

    BOOL ok = WriteFile(
        hFile,
        emptyZip,
        sizeof(emptyZip),
        &written,
        nullptr
    );

    CloseHandle(hFile);

    if (!ok || written != sizeof(emptyZip))
    {
        DeleteFileW(zipPath);
        return false;
    }

    return true;
}

static ULONGLONG GetFileSize64(
    const wchar_t* path)
{
    WIN32_FILE_ATTRIBUTE_DATA data{};

    if (!GetFileAttributesExW(
        path,
        GetFileExInfoStandard,
        &data))
    {
        return 0;
    }

    return
        (static_cast<ULONGLONG>(
            data.nFileSizeHigh) << 32) |
        data.nFileSizeLow;
}

static void WaitForZip(
    const wchar_t* zipPath)
{
    ULONGLONG previous = 0;

    int stable = 0;

    /*
     * Chờ tối đa 10 phút.
     */
    for (int i = 0; i < 1200; ++i)
    {
        Sleep(500);

        ULONGLONG current =
            GetFileSize64(zipPath);

        if (current == previous)
        {
            stable++;

            /*
             * 5 giây không thay đổi.
             */
            if (stable >= 10)
                break;
        }
        else
        {
            stable = 0;
            previous = current;
        }
    }

    /*
     * Cho Shell flush lần cuối.
     */
    Sleep(1000);
}

int wmain()
{
    const wchar_t* sourcePath =
        L"C:\\Users\\vuong\\OneDrive\\Desktop\\BrowserExtract";

    const wchar_t* zipPath =
        L"C:\\Users\\vuong\\OneDrive\\Desktop\\BrowserExtract.zip";

    //
    // Check source
    //
    DWORD attr =
        GetFileAttributesW(sourcePath);

    if (attr == INVALID_FILE_ATTRIBUTES ||
        !(attr & FILE_ATTRIBUTE_DIRECTORY))
    {
        std::wcerr
            << L"Source folder not found.\n";

        return 1;
    }

    //
    // COM
    //
    HRESULT hr =
        CoInitializeEx(
            nullptr,
            COINIT_APARTMENTTHREADED
        );

    if (FAILED(hr))
    {
        PrintError(
            L"CoInitializeEx failed",
            hr
        );

        return 1;
    }

    //
    // Shell.Application
    //
    IShellDispatch* shell = nullptr;

    hr = CoCreateInstance(
        CLSID_Shell,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_IShellDispatch,
        reinterpret_cast<void**>(&shell)
    );

    if (FAILED(hr))
    {
        PrintError(
            L"CoCreateInstance failed",
            hr
        );

        CoUninitialize();

        return 1;
    }

    //
    // Xóa ZIP cũ
    //
    DeleteFileW(zipPath);

    //
    // Tạo ZIP rỗng
    //
    if (!CreateEmptyZip(zipPath))
    {
        std::wcerr
            << L"Cannot create ZIP.\n";

        shell->Release();
        CoUninitialize();

        return 1;
    }

    //
    // Open ZIP namespace
    //
    VARIANT vZip;
    VariantInit(&vZip);

    vZip.vt = VT_BSTR;
    vZip.bstrVal =
        SysAllocString(zipPath);

    Folder* zipFolder = nullptr;

    hr = shell->NameSpace(
        vZip,
        &zipFolder
    );

    VariantClear(&vZip);

    if (FAILED(hr) || !zipFolder)
    {
        PrintError(
            L"Cannot open ZIP",
            hr
        );

        DeleteFileW(zipPath);

        shell->Release();
        CoUninitialize();

        return 1;
    }

    //
    // Add source folder contents.
    //
    std::wcout
        << L"Scanning and compressing:\n"
        << sourcePath
        << L"\n\n";

    hr = AddFolderToZip(
        shell,
        zipFolder,
        sourcePath
    );

    if (FAILED(hr))
    {
        PrintError(
            L"AddFolderToZip failed",
            hr
        );
    }

    //
    // Wait for asynchronous Shell operation.
    //
    std::wcout
        << L"\nWaiting for ZIP operation...\n";

    WaitForZip(zipPath);

    //
    // Release Shell objects BEFORE opening ZIP
    // externally.
    //
    zipFolder->Release();
    zipFolder = nullptr;

    shell->Release();
    shell = nullptr;

    CoUninitialize();

    //
    // Final result
    //
    ULONGLONG size =
        GetFileSize64(zipPath);

    std::wcout
        << L"\nDone.\n"
        << L"ZIP size: "
        << size
        << L" bytes\n"
        << L"Output:\n"
        << zipPath
        << L"\n";

    return 0;
}
