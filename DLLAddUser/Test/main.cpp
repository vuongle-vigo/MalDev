#include <windows.h>
#include <lm.h>
#include <iostream>

#pragma comment(lib, "Netapi32.lib")

int wmain() {
    USER_INFO_1 ui;
    DWORD parm_err = 0;

    ZeroMemory(&ui, sizeof(ui));

    ui.usri1_name = (LPWSTR)L"testuser";
    ui.usri1_password = (LPWSTR)L"Password123!";
    ui.usri1_priv = USER_PRIV_USER;
    ui.usri1_home_dir = NULL;
    ui.usri1_comment = (LPWSTR)L"Created by NetUserAdd";
    ui.usri1_flags = UF_SCRIPT | UF_NORMAL_ACCOUNT;
    ui.usri1_script_path = NULL;

    NET_API_STATUS status = NetUserAdd(
        NULL,       // NULL = local machine
        1,          // USER_INFO_1
        (LPBYTE)&ui,
        &parm_err
    );

    if (status == NERR_Success) {
        std::wcout << L"User created successfully\n";
    }
    else {
        std::wcout << L"NetUserAdd failed: " << status
            << L", parm_err: " << parm_err << L"\n";
    }

    return 0;
}