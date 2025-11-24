#include <windows.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <iostream>
#include <comutil.h>
#pragma comment(lib, "comsuppw.lib")
#pragma comment(lib, "Ole32.lib")

bool unzip_single_file(
    const wchar_t* zipPath,
    const wchar_t* innerFileName,
    const wchar_t* outputFolder
)
{
    CoInitialize(nullptr);

    IShellDispatch* pShell = nullptr;
    if (FAILED(CoCreateInstance(CLSID_Shell, nullptr, CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&pShell))))
    {
        std::wcerr << L"ERROR: Failed to create IShellDispatch.\n";
        return false;
    }

    Folder* pZipFolder = nullptr;
    VARIANT varZip; VariantInit(&varZip);
    varZip.vt = VT_BSTR;
    varZip.bstrVal = SysAllocString(zipPath);

    pShell->NameSpace(varZip, &pZipFolder);
    if (!pZipFolder) {
        std::wcerr << L"ERROR: Failed to open ZIP file.\n";
        return false;
    }

    Folder* pDestFolder = nullptr;
    VARIANT varDest; VariantInit(&varDest);
    varDest.vt = VT_BSTR;
    varDest.bstrVal = SysAllocString(outputFolder);

    pShell->NameSpace(varDest, &pDestFolder);
    if (!pDestFolder) {
        std::wcerr << L"ERROR: Failed to open output folder.\n";
        return false;
    }

    FolderItems* items = nullptr;
    pZipFolder->Items(&items);
    if (!items) {
        std::wcerr << L"ERROR: Failed to enumerate items in ZIP.\n";
        return false;
    }

    long count = 0;
    items->get_Count(&count);

    for (long i = 0; i < count; i++)
    {
        FolderItem* item = nullptr;
        _variant_t idx(i);

        HRESULT hr = items->Item(idx, &item);
        if (FAILED(hr) || !item) {
            std::wcerr << L"ERROR: Failed to read item index " << i << L".\n";
            continue;
        }

        BSTR name;
        item->get_Name(&name);

        if (wcscmp(name, innerFileName) == 0)
        {
            VARIANT v; VariantInit(&v);
            v.vt = VT_DISPATCH;
            v.pdispVal = item;

            HRESULT hrCopy = pDestFolder->CopyHere(v, _variant_t(0));
            if (FAILED(hrCopy)) {
                std::wcerr << L"ERROR: CopyHere failed.\n";
                return false;
            }

            return true;
        }
    }

    std::wcerr << L"ERROR: File not found inside ZIP.\n";
    return false;
}
