#include <Windows.h>
#include <amsi.h>
#include <iostream>

#pragma comment(lib, "amsi.lib")

int main()
{
    HAMSICONTEXT amsiContext = nullptr;
    HAMSISESSION amsiSession = nullptr;
    AMSI_RESULT result;

    HRESULT hr = AmsiInitialize(L"AMSI-Test", &amsiContext);
    if (FAILED(hr))
    {
        std::cout << "AmsiInitialize failed: 0x"
            << std::hex << hr << std::endl;
        return 1;
    }

    hr = AmsiOpenSession(amsiContext, &amsiSession);
    if (FAILED(hr))
    {
        std::cout << "AmsiOpenSession failed: 0x"
            << std::hex << hr << std::endl;

        AmsiUninitialize(amsiContext);
        return 1;
    }

    const char testData[] = "Invoke-Expression \"AMSI Test Sample : 7e72c3ce - 861b - 4339 - 8740 - 0ac1484c1386\"";

    hr = AmsiScanBuffer(
        amsiContext,
        (PVOID)testData,
        sizeof(testData) - 1,
        L"AMSI-Test",
        amsiSession,
        &result
    );

    std::cout << "AmsiScanBuffer HRESULT: 0x"
        << std::hex << hr << std::endl;

    std::cout << "AMSI_RESULT: 0x"
        << std::hex << result << std::endl;

    if (SUCCEEDED(hr))
    {
        if (AmsiResultIsMalware(result))
            std::cout << "Result: MALWARE" << std::endl;
        else
            std::cout << "Result: CLEAN" << std::endl;
    }

    AmsiCloseSession(amsiContext, amsiSession);
    AmsiUninitialize(amsiContext);

    return 0;
}