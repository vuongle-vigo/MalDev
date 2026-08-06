# PowerShell Hosting - Kỹ thuật tạo PowerShell Host từ C++

## Mục lục

1. [Tổng quan](#tổng-quan)
2. [Kiến trúc tổng thể](#kiến-trúc-tổng-thể)
3. [Khởi tạo CLR (Common Language Runtime)](#khởi-tạo-clr-common-language-runtime)
4. [PowerShell Runspace Configuration](#powershell-runspace-configuration)
5. [PowerShell Class API](#powershell-class-api)
6. [Streams và Error Handling](#streams-và-error-handling)
7. [Output Formatting và Printing](#output-formatting-và-printing)
8. [Interactive Console](#interactive-console)
9. [Luồng thực thi hoàn chỉnh](#luồng-thực-thi-hoàn-chỉnh)

---

## Tổng quan

PowerShell Hosting cho phép embed PowerShell engine vào ứng dụng C++ native. Thay vì gọi `powershell.exe` như một process riêng biệt, ta tích hợp trực tiếp PowerShell vào trong process của ứng dụng.

### Ưu điểm

| Khía cạnh | `powershell.exe` | PowerShell Hosting |
|-----------|-----------------|-------------------|
| Control | Process-level | API-level |
| Results | Text parsing | Structured objects |
| Streams | Limited | Full access |
| Errors | Exit code | Full ErrorRecord |
| Lifecycle | Spawn process | In-process |
| Performance | Slow (process) | Fast (in-process) |

### Components cần thiết

```
┌─────────────────────────────────────────────────────────────┐
│                     C++ Native Code                          │
├─────────────────────────────────────────────────────────────┤
│  mscoree.lib / mscoree.h    - CLR Hosting API               │
│  CLRCreateInstance()         - Entry point                  │
├─────────────────────────────────────────────────────────────┤
│                     .NET CLR Runtime                         │
├─────────────────────────────────────────────────────────────┤
│  System.Management.Automation   - PowerShell assembly       │
│  Microsoft.PowerShell.ConsoleHost - Console host assembly   │
└─────────────────────────────────────────────────────────────┘
```

---

## Kiến trúc tổng thể

```
┌─────────────────────────────────────────────────────────────┐
│                    Application (C++)                         │
├─────────────────────────────────────────────────────────────┤
│                      CLR_CONTEXT                             │
│  ┌─────────────┐ ┌──────────────┐ ┌────────────────────┐   │
│  │  pMetaHost  │ │ pRuntimeInfo │ │   pRuntimeHost    │   │
│  └─────────────┘ └──────────────┘ └────────────────────┘   │
├─────────────────────────────────────────────────────────────┤
│                   AppDomain (v4.0.30319)                     │
│  ┌─────────────────────────────────────────────────────┐   │
│  │                 mscorlib::_AppDomain*                │   │
│  └─────────────────────────────────────────────────────┘   │
├─────────────────────────────────────────────────────────────┤
│                   Managed Assemblies                         │
│  ┌─────────────────────────────────────────────────────┐   │
│  │     System.Management.Automation                     │   │
│  │  ┌────────────┐ ┌──────────────┐ ┌───────────────┐  │   │
│  │  │  PowerShell │ │ RunspaceCfg  │ │ PSDataStreams │  │   │
│  │  └────────────┘ └──────────────┘ └───────────────┘  │   │
│  └─────────────────────────────────────────────────────┘   │
│  ┌─────────────────────────────────────────────────────┐   │
│  │     Microsoft.PowerShell.ConsoleHost                  │   │
│  │  ┌──────────────────────────────────────────────┐   │   │
│  │  │         ConsoleShell.Start()                   │   │   │
│  │  └──────────────────────────────────────────────┘   │   │
│  └─────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

---

## Khởi tạo CLR (Common Language Runtime)

### CLR Hosting API Flow

```
┌─────────────────────────────────────────────────────────────────┐
│                 InitializeCommonLanguageRuntime                  │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  1. CLRCreateInstance(CLSID_CLRMetaHost)                       │
│     ↓                                                            │
│     ICLRMetaHost* pMetaHost                                     │
│                                                                 │
│  2. ICLRMetaHost->GetRuntime("v4.0.30319")                     │
│     ↓                                                            │
│     ICLRRuntimeInfo* pRuntimeInfo                               │
│                                                                 │
│  3. ICLRRuntimeInfo->IsLoadable()                               │
│     ↓                                                            │
│     BOOL bIsLoadable                                             │
│                                                                 │
│  4. ICLRRuntimeInfo->GetInterface(CLSID_CorRuntimeHost)        │
│     ↓                                                            │
│     ICorRuntimeHost* pRuntimeHost                                │
│                                                                 │
│  5. ICorRuntimeHost->Start()                                    │
│     ↓                                                            │
│     CLR started successfully                                     │
│                                                                 │
│  6. ICorRuntimeHost->CreateDomain(APP_DOMAIN)                   │
│     ↓                                                            │
│     IUnknown* pAppDomainThunk                                    │
│                                                                 │
│  7. pAppDomainThunk->QueryInterface(IID_AppDomain)              │
│     ↓                                                            │
│     mscorlib::_AppDomain* pAppDomain                            │
│                                                                 │
│  Output: CLR_CONTEXT + pAppDomain                                │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### Code implementation

```cpp
BOOL clr::InitializeCommonLanguageRuntime(
    PCLR_CONTEXT  pClrContext,
    mscorlib::_AppDomain** ppAppDomain
)
{
    BOOL bResult           = FALSE;
    HRESULT hr;
    ICLRMetaHost*          pMetaHost       = NULL;
    ICLRRuntimeInfo*       pRuntimeInfo    = NULL;
    ICorRuntimeHost*       pRuntimeHost     = NULL;
    IUnknown*              pAppDomainThunk  = NULL;
    BOOL                   bIsLoadable;
    mscorlib::_AppDomain*  pAppDomain       = NULL;

    // Step 1: Get ICLRMetaHost interface
    hr = CLRCreateInstance(
        CLSID_CLRMetaHost,
        IID_ICLRMetaHost,
        reinterpret_cast<PVOID*>(&pMetaHost)
    );
    EXIT_ON_HRESULT_ERROR(L"CLRCreateInstance", hr);

    // Step 2: Get runtime info for .NET Framework 4.x
    hr = pMetaHost->GetRuntime(
        L"v4.0.30319",
        IID_ICLRRuntimeInfo,
        reinterpret_cast<PVOID*>(&pRuntimeInfo)
    );
    EXIT_ON_HRESULT_ERROR(L"IMetaHost->GetRuntime", hr);

    // Step 3: Check if runtime is loadable
    hr = pRuntimeInfo->IsLoadable(&bIsLoadable);
    EXIT_ON_HRESULT_ERROR(L"IRuntimeInfo->IsLoadable", hr);

    // Step 4: Get ICorRuntimeHost interface
    hr = pRuntimeInfo->GetInterface(
        CLSID_CorRuntimeHost,
        IID_ICorRuntimeHost,
        reinterpret_cast<PVOID*>(&pRuntimeHost)
    );
    EXIT_ON_HRESULT_ERROR(L"IRuntimeInfo->GetInterface", hr);

    // Step 5: Start the CLR
    hr = pRuntimeHost->Start();
    EXIT_ON_HRESULT_ERROR(L"IRuntimeHost->Start", hr);

    // Step 6: Create AppDomain
    hr = pRuntimeHost->CreateDomain(
        APP_DOMAIN,      // L"MalDevPowerShell"
        nullptr,
        &pAppDomainThunk
    );
    EXIT_ON_HRESULT_ERROR(L"IRuntimeHost->CreateDomain", hr);

    // Step 7: Query for AppDomain interface
    hr = pAppDomainThunk->QueryInterface(IID_PPV_ARGS(&pAppDomain));
    EXIT_ON_HRESULT_ERROR(L"IAppDomainThunk->QueryInterface", hr);

    // Success - store interfaces in context
    pClrContext->pMetaHost       = pMetaHost;
    pClrContext->pRuntimeInfo    = pRuntimeInfo;
    pClrContext->pRuntimeHost    = pRuntimeHost;
    pClrContext->pAppDomainThunk = pAppDomainThunk;
    *ppAppDomain                 = pAppDomain;
    bResult                      = TRUE;

exit:
    // Cleanup on failure (success case transfers ownership)
    if (!bResult && pAppDomain)       pAppDomain->Release();
    if (!bResult && pAppDomainThunk)  pAppDomainThunk->Release();
    if (!bResult && pRuntimeHost)    pRuntimeHost->Release();
    if (!bResult && pRuntimeInfo)    pRuntimeInfo->Release();
    if (!bResult && pMetaHost)       pMetaHost->Release();

    return bResult;
}
```

### Cleanup

```cpp
void clr::DestroyCommonLanguageRuntime(
    PCLR_CONTEXT           pClrContext,
    mscorlib::_AppDomain*  pAppDomain
)
{
    if (pAppDomain)                          pAppDomain->Release();
    if (pClrContext->pAppDomainThunk)        pClrContext->pAppDomainThunk->Release();
    if (pClrContext->pRuntimeHost)            pClrContext->pRuntimeHost->Release();
    if (pClrContext->pRuntimeInfo)           pClrContext->pRuntimeInfo->Release();
    if (pClrContext->pMetaHost)              pClrContext->pMetaHost->Release();
}
```

### Finding Assembly Path

```cpp
BOOL clr::FindAssemblyPath(
    LPCWSTR pwszAssemblyName,
    LPWSTR* ppwszAssemblyPath
)
{
    LPCWSTR pwszAssemblyFolderPath = L"C:\\Windows\\Microsoft.NET\\assembly\\GAC_MSIL";
    WIN32_FIND_DATA ffd = { 0 };
    HANDLE hFind = FindFirstFileW(searchPath, &ffd);

    // Search pattern: GAC_MSIL\{AssemblyName}\{Version}\{AssemblyName}.dll
    // Example: GAC_MSIL\System.Management.Automation\v4.0_7.0.0.0__31bf3856ad364e35\System.Management.Automation.dll

    do {
        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            swprintf_s(assemblyPath, MAX_PATH,
                L"%ws\\%ws\\%ws\\%ws.dll",
                pwszAssemblyFolderPath,
                pwszAssemblyName,
                ffd.cFileName,
                pwszAssemblyName
            );
            // Verify file exists...
        }
    } while (FindNextFileW(hFind, &ffd));
}
```

---

## PowerShell Runspace Configuration

### Khái niệm Runspace

```
┌─────────────────────────────────────────────────────────────┐
│                      Runspace                                │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────────────────────────────────────────────┐   │
│  │            RunspaceConfiguration                     │   │
│  │  - InitialSessionState                              │   │
│  │  - Available cmdlets                                 │   │
│  │  - Language mode                                     │   │
│  │  - Execution policy                                  │   │
│  └─────────────────────────────────────────────────────┘   │
│  ┌─────────────────────────────────────────────────────┐   │
│  │                 SessionState                          │   │
│  │  - Variables                                         │   │
│  │  - Providers                                         │   │
│  │  - Imported modules                                  │   │
│  └─────────────────────────────────────────────────────┘   │
│  ┌─────────────────────────────────────────────────────┐   │
│  │                   Pipeline                            │   │
│  │  [Command1] → [Command2] → [Command3] → [Output]    │   │
│  └─────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

### Tạo RunspaceConfiguration

```cpp
BOOL CreateInitialRunspaceConfiguration(
    mscorlib::_AppDomain*  pAppDomain,
    VARIANT*               pvtRunspaceConfiguration
)
{
    mscorlib::_Type*        pRunspaceConfigurationType = NULL;
    mscorlib::_MethodInfo*  pCreateInfo = NULL;
    VARIANT vtEmpty = { 0 };
    VARIANT vtResult = { 0 };

    // Lấy type: System.Management.Automation.Runspaces.RunspaceConfiguration
    clr::GetType(
        pAppDomain,
        ASSEMBLY_NAME_SYSTEM_MANAGEMENT_AUTOMATION,
        L"System.Management.Automation.Runspaces.RunspaceConfiguration",
        &pRunspaceConfigurationType
    );

    // Gọi static method: RunspaceConfiguration.Create()
    clr::GetMethod(
        pRunspaceConfigurationType,
        BINDING_FLAGS_PUBLIC_STATIC,
        L"Create",
        0,              // 0 arguments
        &pCreateInfo
    );

    clr::InvokeMethod(pCreateInfo, vtEmpty, NULL, &vtResult);
    memcpy_s(pvtRunspaceConfiguration, sizeof(*pvtRunspaceConfiguration), &vtResult, sizeof(vtResult));

    // Cleanup
    pCreateInfo->Release();
    pRunspaceConfigurationType->Release();
}
```

### Start Console Shell

```cpp
BOOL StartConsoleShell(
    mscorlib::_AppDomain*  pAppDomain,
    VARIANT                vtRunspaceConfiguration,
    LPCWSTR                pwszBanner,         // "Windows PowerShell\nCopyright..."
    LPCWSTR                pwszHelp,           // Help message
    LPCWSTR*               ppwszArguments,     // Arguments array
    DWORD                  dwArgumentCount     // Số lượng arguments
)
{
    mscorlib::_Type*        pConsoleShellType = NULL;
    mscorlib::_MethodInfo*  pStartMethodInfo = NULL;
    SAFEARRAY*              pStartArguments = NULL;
    VARIANT vtBannerText, vtHelpText, vtArguments, vtResult;

    // Lấy type: Microsoft.PowerShell.ConsoleShell
    clr::GetType(
        pAppDomain,
        ASSEMBLY_NAME_MICROSOFT_POWERSHELL_CONSOLEHOST,
        L"Microsoft.PowerShell.ConsoleShell",
        &pConsoleShellType
    );

    // Lấy method: ConsoleShell.Start(RunspaceConfiguration, Banner, Help, Arguments)
    clr::GetMethod(
        pConsoleShellType,
        BINDING_FLAGS_PUBLIC_STATIC,
        L"Start",
        4,              // 4 arguments
        &pStartMethodInfo
    );

    // Chuẩn bị arguments
    InitVariantFromString(pwszBanner, &vtBannerText);
    InitVariantFromString(pwszHelp, &vtHelpText);
    InitVariantFromStringArray(ppwszArguments, dwArgumentCount, &vtArguments);

    // Tạo SafeArray chứa 4 arguments
    pStartArguments = SafeArrayCreateVector(VT_VARIANT, 0, 4);
    SafeArrayPutElement(pStartArguments, &(LONG){0}, &vtRunspaceConfiguration);
    SafeArrayPutElement(pStartArguments, &(LONG){1}, &vtBannerText);
    SafeArrayPutElement(pStartArguments, &(LONG){2}, &vtHelpText);
    SafeArrayPutElement(pStartArguments, &(LONG){3}, &vtArguments);

    // Invoke
    clr::InvokeMethod(pStartMethodInfo, vtEmpty, pStartArguments, &vtResult);

    // Cleanup
    SafeArrayDestroy(pStartArguments);
    // ... release other resources
}
```

---

## PowerShell Class API

### Tạo PowerShell Instance

```cpp
BOOL PowerShellCreate(
    mscorlib::_AppDomain*  pAppDomain,
    VARIANT*               pvtPowerShellInstance
)
{
    mscorlib::_Type*        pPowerShellType = NULL;
    mscorlib::_MethodInfo*  pCreateMethodInfo = NULL;
    VARIANT vtEmpty = { 0 };
    VARIANT vtInstance = { 0 };

    // Type: System.Management.Automation.PowerShell
    clr::GetType(
        pAppDomain,
        ASSEMBLY_NAME_SYSTEM_MANAGEMENT_AUTOMATION,
        L"System.Management.Automation.PowerShell",
        &pPowerShellType
    );

    // Static method: PowerShell.Create()
    clr::GetMethod(
        pPowerShellType,
        BINDING_FLAGS_PUBLIC_STATIC,
        L"Create",
        0,
        &pCreateMethodInfo
    );

    clr::InvokeMethod(pCreateMethodInfo, vtEmpty, NULL, &vtInstance);
    memcpy_s(pvtPowerShellInstance, sizeof(*pvtPowerShellInstance), &vtInstance, sizeof(vtInstance));

    pCreateMethodInfo->Release();
    pPowerShellType->Release();
}
```

### Thêm Script

```cpp
BOOL PowerShellAddScript(
    mscorlib::_AppDomain*  pAppDomain,
    VARIANT                vtPowerShellInstance,
    LPWSTR                 pwszScript
)
{
    mscorlib::_Type*        pPowerShellType = NULL;
    mscorlib::_MethodInfo*  pAddScriptMethodInfo = NULL;
    VARIANT vtScript = { 0 };
    VARIANT vtResult = { 0 };
    SAFEARRAY* pAddScriptArguments = NULL;

    // Instance method: PowerShell.AddScript(string script)
    clr::GetMethod(
        pPowerShellType,
        BINDING_FLAGS_PUBLIC_INSTANCE,
        L"AddScript",
        1,
        &pAddScriptMethodInfo
    );

    InitVariantFromString(pwszScript, &vtScript);
    pAddScriptArguments = SafeArrayCreateVector(VT_VARIANT, 0, 1);
    SafeArrayPutElement(pAddScriptArguments, &(LONG){0}, &vtScript);

    clr::InvokeMethod(pAddScriptMethodInfo, vtPowerShellInstance, pAddScriptArguments, &vtResult);

    SafeArrayDestroy(pAddScriptArguments);
    VariantClear(&vtScript);
    VariantClear(&vtResult);
}
```

### Thêm Command

```cpp
BOOL PowerShellAddCommand(
    mscorlib::_AppDomain*  pAppDomain,
    VARIANT                vtPowerShellInstance,
    LPCWSTR                pwszCommand
)
{
    mscorlib::_Type*        pPowerShellType = NULL;
    mscorlib::_MethodInfo*  pAddCommandMethodInfo = NULL;
    VARIANT vtCommand = { 0 };
    VARIANT vtUseLocalScope = { 0 };
    VARIANT vtResult = { 0 };
    SAFEARRAY* pAddCommandArguments = NULL;

    // Instance method: PowerShell.AddCommand(string command, bool useLocalScope)
    clr::GetMethod(
        pPowerShellType,
        BINDING_FLAGS_PUBLIC_INSTANCE,
        L"AddCommand",
        2,
        &pAddCommandMethodInfo
    );

    InitVariantFromString(pwszCommand, &vtCommand);
    InitVariantFromBoolean(FALSE, &vtUseLocalScope);  // Không dùng local scope

    pAddCommandArguments = SafeArrayCreateVector(VT_VARIANT, 0, 2);
    SafeArrayPutElement(pAddCommandArguments, &(LONG){0}, &vtCommand);
    SafeArrayPutElement(pAddCommandArguments, &(LONG){1}, &vtUseLocalScope);

    clr::InvokeMethod(pAddCommandMethodInfo, vtPowerShellInstance, pAddCommandArguments, &vtResult);

    SafeArrayDestroy(pAddCommandArguments);
    VariantClear(&vtCommand);
    VariantClear(&vtResult);
}
```

### Invoke Pipeline

```cpp
BOOL PowerShellInvoke(
    mscorlib::_AppDomain*  pAppDomain,
    VARIANT                vtPowerShellInstance,
    VARIANT*               pvtInvokeResult
)
{
    mscorlib::_Type*        pPowerShellType = NULL;
    mscorlib::_MethodInfo*  pInvokeMethodInfo = NULL;
    VARIANT vtInvokeResult = { 0 };

    // Instance method: PowerShell.Invoke()
    clr::GetMethod(
        pPowerShellType,
        BINDING_FLAGS_PUBLIC_INSTANCE,
        L"Invoke",
        0,
        &pInvokeMethodInfo
    );

    clr::InvokeMethod(pInvokeMethodInfo, vtPowerShellInstance, NULL, &vtInvokeResult);
    memcpy_s(pvtInvokeResult, sizeof(*pvtInvokeResult), &vtInvokeResult, sizeof(vtInvokeResult));

    pInvokeMethodInfo->Release();
    pPowerShellType->Release();
}
```

### Dispose

```cpp
BOOL PowerShellDispose(
    mscorlib::_AppDomain*  pAppDomain,
    VARIANT                vtPowerShellInstance
)
{
    mscorlib::_Type*        pPowerShellType = NULL;
    mscorlib::_MethodInfo*  pDisposeMethodInfo = NULL;
    VARIANT vtResult = { 0 };

    // Instance method: PowerShell.Dispose()
    clr::GetMethod(
        pPowerShellType,
        BINDING_FLAGS_PUBLIC_INSTANCE,
        L"Dispose",
        0,
        &pDisposeMethodInfo
    );

    clr::InvokeMethod(pDisposeMethodInfo, vtPowerShellInstance, NULL, &vtResult);

    pDisposeMethodInfo->Release();
    pPowerShellType->Release();
    VariantClear(&vtResult);
}
```

---

## Streams và Error Handling

### PowerShell Streams

```
┌─────────────────────────────────────────────────────────────┐
│                    PSDataStreams                             │
├─────────────────────────────────────────────────────────────┤
│  Output    (Stream 1)  - Output chính                       │
│  Error     (Stream 2)  - Error records                      │
│  Warning   (Stream 3)  - Warning messages                    │
│  Verbose   (Stream 4)  - Verbose output                     │
│  Debug     (Stream 5)  - Debug messages                     │
│  Information (Stream 6) - Info messages                     │
└─────────────────────────────────────────────────────────────┘
```

### Kiểm tra Errors

```cpp
BOOL PowerShellHadErrors(
    mscorlib::_AppDomain*  pAppDomain,
    VARIANT                vtPowerShellInstance,
    PBOOL                  pbHadErrors
)
{
    VARIANT vtHadErrors = { 0 };

    // Property: PowerShell.HadErrors
    clr::GetPropertyValue(
        pPowerShellType,
        BINDING_FLAGS_PUBLIC_INSTANCE,
        vtPowerShellInstance,
        L"HadErrors",
        &vtHadErrors
    );

    *pbHadErrors = vtHadErrors.boolVal;  // VT_BOOL

    pPowerShellType->Release();
}
```

### Đọc Stream

```cpp
BOOL PowerShellGetStream(
    mscorlib::_AppDomain*  pAppDomain,
    VARIANT                vtPowerShellInstance,
    LPCWSTR                pwszStreamName,  // L"Error", L"Warning", L"Information"
    VARIANT*               pvtStream
)
{
    VARIANT vtStreams = { 0 };
    VARIANT vtStream = { 0 };

    // 1. Đọc property Streams
    clr::GetPropertyValue(
        pPowerShellType,
        BINDING_FLAGS_PUBLIC_INSTANCE,
        vtPowerShellInstance,
        L"Streams",
        &vtStreams
    );

    // 2. Đọc stream cụ thể
    clr::GetPropertyValue(
        pPSDataStreamsType,
        BINDING_FLAGS_PUBLIC_INSTANCE,
        vtStreams,
        pwszStreamName,
        &vtStream
    );

    memcpy_s(pvtStream, sizeof(*pvtStream), &vtStream, sizeof(vtStream));

    VariantClear(&vtStreams);
    pPSDataStreamsType->Release();
    pPowerShellType->Release();
}
```

---

## Output Formatting và Printing

### In kết quả Invoke

```cpp
void PrintPowerShellInvokeResult(
    mscorlib::_AppDomain*  pAppDomain,
    VARIANT                vtInvokeResult
)
{
    VARIANT vtInvokeResultType = { 0 };
    VARIANT vtInvokeResultCount = { 0 };
    VARIANT vtItem = { 0 };
    VARIANT vtValueAsString = { 0 };
    mscorlib::_Type* pPSObjectType = NULL;
    mscorlib::_MethodInfo* pToStringMethodInfo = NULL;

    // Lấy type của result
    dotnet::System_Object_GetType(pAppDomain, vtInvokeResult, &vtInvokeResultType);

    // Đọc Count property
    dotnet::System_Type_GetProperty(pAppDomain, vtInvokeResultType, L"Count", &vtCountProperty);
    dotnet::System_Reflection_PropertyInfo_GetValue(pAppDomain, vtCountProperty, vtInvokeResult, &vtInvokeResultCount);

    if (vtInvokeResultCount.lVal > 0)
    {
        wprintf(L"\n");
        wprintf(L"+-----------------------------------+\n");
        wprintf(L"| POWERSHELL STANDARD OUTPUT STREAM |\n");
        wprintf(L"+-----------------------------------+\n");

        for (int i = 0; i < vtInvokeResultCount.lVal; i++)
        {
            // Đọc item[i]
            dotnet::System_Reflection_PropertyInfo_GetValue(
                pAppDomain, vtItemProperty, vtInvokeResult, pIndex, &vtValue
            );

            // Gọi .ToString()
            clr::InvokeMethod(pToStringMethodInfo, vtValue, NULL, &vtValueAsString);

            wprintf(L"%ws", vtValueAsString.bstrVal);

            VariantClear(&vtValueAsString);
            VariantClear(&vtValue);
        }
    }
}
```

### In Error Record

```cpp
void PrintErrorRecord(
    mscorlib::_AppDomain*  pAppDomain,
    VARIANT                vtErrorRecord
)
{
    VARIANT vtTargetObject, vtScriptStackTrace, vtCategoryInfo;
    VARIANT vtExceptionMessage, vtFullyQualifiedErrorId;
    WORD wOldColor = 0;

    // Đọc các properties của ErrorRecord
    dotnet::System_Type_GetProperty(pAppDomain, vtErrorRecordType, L"TargetObject", &vtTargetObjectProperty);
    dotnet::System_Reflection_PropertyInfo_GetValue(pAppDomain, vtTargetObjectProperty, vtErrorRecord, &vtTargetObject);

    dotnet::System_Type_GetProperty(pAppDomain, vtErrorRecordType, L"ScriptStackTrace", &vtScriptStackTraceProperty);
    dotnet::System_Reflection_PropertyInfo_GetValue(pAppDomain, vtScriptStackTraceProperty, vtErrorRecord, &vtScriptStackTrace);

    dotnet::System_Type_GetProperty(pAppDomain, vtErrorRecordType, L"CategoryInfo", &vtCategoryInfoProperty);
    dotnet::System_Reflection_PropertyInfo_GetValue(pAppDomain, vtCategoryInfoProperty, vtErrorRecord, &vtCategoryInfo);

    dotnet::System_Type_GetProperty(pAppDomain, vtErrorRecordType, L"FullyQualifiedErrorId", &vtFullyQualifiedErrorIdProperty);
    dotnet::System_Reflection_PropertyInfo_GetValue(pAppDomain, vtFullyQualifiedErrorIdProperty, vtErrorRecord, &vtFullyQualifiedErrorId);

    dotnet::System_Type_GetProperty(pAppDomain, vtErrorRecordType, L"Exception", &vtExceptionProperty);
    dotnet::System_Reflection_PropertyInfo_GetValue(pAppDomain, vtExceptionProperty, vtErrorRecord, &vtException);

    // In với màu đỏ
    SetConsoleTextColor(FOREGROUND_RED | FOREGROUND_INTENSITY, &wOldColor);

    wprintf(L"%ws : %ws\n", vtTargetObject.bstrVal, vtExceptionMessage.bstrVal);
    wprintf(L"%ws\n", vtScriptStackTrace.bstrVal);
    wprintf(L"+ CategoryInfo : %ws\n", vtCategoryInfoMessage.bstrVal);
    wprintf(L"+ FullyQualifiedErrorId : %ws\n", vtFullyQualifiedErrorId.bstrVal);

    SetConsoleTextColor(wOldColor, NULL);
}
```

### Set Console Text Color

```cpp
void SetConsoleTextColor(WORD wColor, PWORD pwOldColor)
{
    HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi = { 0 };

    GetConsoleScreenBufferInfo(hStdOut, &csbi);

    if (pwOldColor)
        *pwOldColor = csbi.wAttributes & 0x000f;  // Save current foreground

    WORD wAttributes = csbi.wAttributes & 0xfff0;
    wAttributes |= wColor & 0x000f;

    SetConsoleTextAttribute(hStdOut, wAttributes);
}
```

---

## Interactive Console

### Luồng tạo Console

```
┌─────────────────────────────────────────────────────────────┐
│                  CreatePowerShellConsole                     │
├─────────────────────────────────────────────────────────────┤
│  1. InitializeCommonLanguageRuntime(&cc, &pAppDomain)        │
│     ↓                                                        │
│  2. CreateInitialRunspaceConfiguration(&vtConfig)           │
│     ↓                                                        │
│  3. PatchAllTheThings(pAppDomain)  [bypass - optional]      │
│     ↓                                                        │
│  4. StartConsoleShell(pAppDomain, vtConfig, ...)            │
│     ↓                                                        │
│  5. User tương tác với PowerShell console                   │
│     ↓                                                        │
│  6. User exit (exit command, Ctrl+C)                         │
│     ↓                                                        │
│  7. Cleanup                                                  │
│                                                                │
└─────────────────────────────────────────────────────────────┘
```

### Code

```cpp
void CreatePowerShellConsole()
{
    mscorlib::_AppDomain*  pAppDomain = NULL;
    CLR_CONTEXT            cc = { 0 };
    VARIANT                vtInitialRunspaceConfiguration = { 0 };

    LPCWSTR pwszBannerText = L"Windows PowerShell\nCopyright (C) Microsoft Corporation...";
    LPCWSTR pwszHelpText = L"Try the new cross-platform PowerShell...";
    LPCWSTR ppwszArguments[] = { NULL };

    // Step 1: Initialize CLR
    if (!clr::InitializeCommonLanguageRuntime(&cc, &pAppDomain))
        goto exit;

    // Step 2: Create RunspaceConfiguration
    if (!CreateInitialRunspaceConfiguration(pAppDomain, &vtInitialRunspaceConfiguration))
        goto exit;

    // Step 3: Patch (AMSI, ETW, etc. - optional)
    PatchAllTheThings(pAppDomain);

    // Step 4: Start interactive shell
    if (!StartConsoleShell(
        pAppDomain,
        vtInitialRunspaceConfiguration,
        pwszBannerText,
        pwszHelpText,
        ppwszArguments,
        ARRAYSIZE(ppwszArguments)
    ))
        goto exit;

exit:
    VariantClear(&vtInitialRunspaceConfiguration);
    clr::DestroyCommonLanguageRuntime(&cc, pAppDomain);
}
```

---

## Luồng thực thi hoàn chỉnh

### Execute Script Flow

```
┌─────────────────────────────────────────────────────────────────┐
│                  ExecutePowerShellScript                         │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  1. InitializeCommonLanguageRuntime(&cc, &pAppDomain)           │
│     ↓                                                            │
│  2. PowerShellCreate(&vtPowerShell)                             │
│     ↓                                                            │
│  3. PowerShellAddScript(vtPowerShell, script)                   │
│     ↓                                                            │
│  4. PowerShellAddCommand(vtPowerShell, "Out-String")            │
│     ↓                                                            │
│  5. PatchAllTheThings(pAppDomain)                                │
│     ↓                                                            │
│  6. PowerShellInvoke(vtPowerShell, &vtResult)                   │
│     ↓                                                            │
│  7. PrintPowerShellInvokeResult(vtResult)                       │
│     ↓                                                            │
│  8. PrintPowerShellInvokeInformation(vtPowerShell)              │
│     ↓                                                            │
│  9. PowerShellHadErrors(vtPowerShell, &bHadErrors)              │
│     ↓                                                            │
│  10. IF bHadErrors THEN                                         │
│         PrintPowerShellInvokeErrors(vtPowerShell)               │
│      END IF                                                      │
│     ↓                                                            │
│  11. PowerShellDispose(vtPowerShell)                             │
│     ↓                                                            │
│  12. DestroyCommonLanguageRuntime(&cc, pAppDomain)               │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### Complete Implementation

```cpp
void ExecutePowerShellScript(LPWSTR pwszScript)
{
    mscorlib::_AppDomain*  pAppDomain = NULL;
    CLR_CONTEXT            cc = { 0 };
    VARIANT                vtPowerShell = { 0 };
    VARIANT                vtInvokeResult = { 0 };
    BOOL                   bHadErrors = FALSE;

    // 1. Initialize CLR
    if (!clr::InitializeCommonLanguageRuntime(&cc, &pAppDomain))
        goto exit;

    // 2. Create PowerShell instance
    if (!PowerShellCreate(pAppDomain, &vtPowerShell))
        goto exit;

    // 3. Add script to pipeline
    if (!PowerShellAddScript(pAppDomain, vtPowerShell, pwszScript))
        goto exit;

    // 4. Add Out-String to convert objects to string
    if (!PowerShellAddCommand(pAppDomain, vtPowerShell, L"Out-String"))
        goto exit;

    // 5. Apply patches (AMSI, ETW, etc.)
    PatchAllTheThings(pAppDomain);

    // 6. Invoke pipeline
    if (PowerShellInvoke(pAppDomain, vtPowerShell, &vtInvokeResult))
    {
        // 7. Print main output
        PrintPowerShellInvokeResult(pAppDomain, vtInvokeResult);

        // 8. Print information stream
        PrintPowerShellInvokeInformation(pAppDomain, vtPowerShell);
    }

    // 9. Check for errors
    if (!PowerShellHadErrors(pAppDomain, vtPowerShell, &bHadErrors))
        goto exit;

    // 10. Print errors if any
    if (bHadErrors)
    {
        PrintPowerShellInvokeErrors(pAppDomain, vtPowerShell);
    }

exit:
    // 11. Cleanup
    if (pAppDomain && vtPowerShell.punkVal)
        PowerShellDispose(pAppDomain, vtPowerShell);

    VariantClear(&vtInvokeResult);
    VariantClear(&vtPowerShell);
    clr::DestroyCommonLanguageRuntime(&cc, pAppDomain);
}
```

### Ví dụ sử dụng

```cpp
int main()
{
    // Execute simple command
    ExecutePowerShellScript(L"Get-Process | Select-Object -First 5 Name, CPU");

    // Execute cmdlet
    ExecutePowerShellScript(L"Get-Service | Where-Object {$_.Status -eq 'Running'}");

    // Execute multi-line script
    ExecutePowerShellScript(
        L"$services = Get-Service;\n"
        L"$services | Where-Object {$_.Status -eq 'Running'} | "
        L"Select-Object -First 5 Name, Status"
    );

    // Interactive console
    CreatePowerShellConsole();

    return 0;
}
```

---

## Tài liệu tham khảo

### Microsoft Documentation

| Topic | URL |
|-------|-----|
| CLR Hosting Interfaces | [docs.microsoft.com/.../hosting/clr-hosting-interfaces](https://docs.microsoft.com/en-us/dotnet/framework/unmanaged-api/hosting/clr-hosting-interfaces) |
| CLRCreateInstance | [docs.microsoft.com/.../hosting/clrcreateinstance-function](https://docs.microsoft.com/en-us/dotnet/framework/unmanaged-api/hosting/clrcreateinstance-function) |
| ICLRMetaHost | [docs.microsoft.com/.../hosting/iclrmetahost-interface](https://docs.microsoft.com/en-us/dotnet/framework/unmanaged-api/hosting/iclrmetahost-interface) |
| ICLRRuntimeInfo | [docs.microsoft.com/.../hosting/iclrruntimeinfo-interface](https://docs.microsoft.com/en-us/dotnet/framework/unmanaged-api/hosting/iclrruntimeinfo-interface) |
| ICorRuntimeHost | [docs.microsoft.com/.../hosting/icorruntimehost-interface](https://docs.microsoft.com/en-us/dotnet/framework/unmanaged-api/hosting/icorruntimehost-interface) |

### PowerShell SDK

| Topic | URL |
|-------|-----|
| RunspaceConfiguration | [docs.microsoft.com/.../runspaces/runspaceconfiguration](https://docs.microsoft.com/en-us/dotnet/api/system.management.automation.runspaces.runspaceconfiguration) |
| PowerShell Class | [docs.microsoft.com/.../powershell](https://docs.microsoft.com/en-us/dotnet/api/system.management.automation.powershell) |
| PSDataStreams | [docs.microsoft.com/.../psdatastreams](https://docs.microsoft.com/en-us/dotnet/api/system.management.automation.psdatastreams) |
| ErrorRecord | [docs.microsoft.com/.../errorrecord](https://docs.microsoft.com/en-us/dotnet/api/system.management.automation.errorrecord) |

---

## Assembly Dependencies

| Assembly | Path | Mục đích |
|----------|------|----------|
| `System.Management.Automation` | `GAC_MSIL\System.Management.Automation\` | PowerShell engine |
| `System.Management` | `GAC_MSIL\System.Management\` | WMI support |
| `Microsoft.PowerShell.ConsoleHost` | `GAC_MSIL\Microsoft.PowerShell.ConsoleHost\` | Console shell |
| `System.Management.Automation.resources` | `GAC_MSIL\System.Management.Automation.resources\` | Localization |
| `mscorlib` | Built-in | .NET base types |

---

*Document generated: August 2026*
