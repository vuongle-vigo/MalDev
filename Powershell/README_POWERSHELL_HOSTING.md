# PowerShell Hosting - Kỹ thuật tạo PowerShell Host từ C++

## Mục lục

1. [Tổng quan](#tổng-quan)
2. [PowerShell Runspace Architecture](#powershell-runspace-architecture)
3. [Khởi tạo Runspace](#khởi-tạo-runspace)
4. [PowerShell Class API](#powershell-class-api)
5. [Streams và Error Handling](#streams-và-error-handling)
6. [Output Formatting](#output-formatting)
7. [Interactive Console](#interactive-console)
8. [Luồng thực thi](#luồng-thực-thi)

---

## Tổng quan

PowerShell Hosting cho phép embed PowerShell engine vào ứng dụng C++. Thay vì gọi `powershell.exe -Command`, ta có thể:

- Tạo PowerShell runspace từ C++
- Thêm script/command vào pipeline
- Invoke và lấy kết quả
- Xử lý multiple streams (output, error, warning, verbose, debug)
- Tạo interactive console

**Ưu điểm so với `powershell.exe`:**
- Kiểm soát hoàn toàn execution context
- Xử lý kết quả trực tiếp trong C++
- Không cần parse text output
- Tích hợp sâu với ứng dụng

---

## PowerShell Runspace Architecture

### Khái niệm cơ bản

```
┌─────────────────────────────────────────────────────────┐
│                    Application (C++)                     │
├─────────────────────────────────────────────────────────┤
│                    PowerShell Runspace                   │
│  ┌─────────────────────────────────────────────────────┐ │
│  │              RunspaceConfiguration                   │ │
│  │  - Initial session state                            │ │
│  │  - Available commands                               │ │
│  │  - Language mode                                   │ │
│  └─────────────────────────────────────────────────────┘ │
│  ┌─────────────────────────────────────────────────────┐ │
│  │                   Pipeline                           │ │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────┐ │ │
│  │  │   Command 1   │→ │   Command 2   │→ │  Output  │ │ │
│  │  └──────────────┘  └──────────────┘  └──────────┘ │ │
│  └─────────────────────────────────────────────────────┘ │
│  ┌─────────────────────────────────────────────────────┐ │
│  │                    Streams                           │ │
│  │  Output | Error | Warning | Verbose | Debug | Info │ │
│  └─────────────────────────────────────────────────────┘ │
├─────────────────────────────────────────────────────────┤
│                  .NET CLR Runtime                        │
└─────────────────────────────────────────────────────────┘
```

### Các thành phần chính

| Thành phần | .NET Class | Mô tả |
|------------|------------|-------|
| **Runspace** | `Runspace` | Môi trường thực thi PowerShell |
| **RunspaceConfiguration** | `RunspaceConfiguration` | Cấu hình initial cho runspace |
| **PowerShell** | `System.Management.Automation.PowerShell` | Quản lý pipeline |
| **Pipeline** | `Pipeline` | Chuỗi commands được thực thi |
| **Streams** | `PSDataStreams` | Các output streams khác nhau |

---

## Khởi tạo Runspace

### Tạo RunspaceConfiguration

```cpp
BOOL CreateInitialRunspaceConfiguration(mscorlib::_AppDomain* pAppDomain, VARIANT* pvtRunspaceConfiguration)
{
    mscorlib::_Type* pRunspaceConfigurationType = NULL;
    mscorlib::_MethodInfo* pCreateInfo = NULL;
    VARIANT vtResult = { 0 };

    // 1. Lấy type RunspaceConfiguration
    GetType("System.Management.Automation.Runspaces.RunspaceConfiguration");
    
    // 2. Gọi static method Create()
    GetMethod("Create", 0);  // 0 arguments
    
    // 3. Invoke không tham số
    InvokeMethod(NULL, NULL, &vtResult);
    
    // 4. Copy vào output parameter
    memcpy(pvtRunspaceConfiguration, &vtResult, sizeof(vtResult));
}
```

**Tương đương PowerShell:**
```powershell
$runspaceConfig = [System.Management.Automation.Runspaces.RunspaceConfiguration]::Create()
```

### Start Console Shell

```cpp
BOOL StartConsoleShell(
    mscorlib::_AppDomain* pAppDomain, 
    VARIANT vtRunspaceConfiguration,
    LPCWSTR pwszBanner,        // "Windows PowerShell\nCopyright..."
    LPCWSTR pwszHelp,          // Help message
    LPCWSTR* ppwszArguments,   // Arguments array
    DWORD dwArgumentCount       // Số lượng arguments
)
{
    // 1. Lấy type ConsoleShell
    GetType("Microsoft.PowerShell.ConsoleShell");
    
    // 2. Tìm method Start với 4 tham số
    GetMethod("Start", 4);
    
    // 3. Tạo SafeArray chứa arguments
    SAFEARRAY* pStartArguments = SafeArrayCreateVector(VT_VARIANT, 0, 4);
    
    SafeArrayPutElement(pStartArguments, 0, &vtRunspaceConfiguration);
    SafeArrayPutElement(pStartArguments, 1, &vtBannerText);
    SafeArrayPutElement(pStartArguments, 2, &vtHelpText);
    SafeArrayPutElement(pStartArguments, 3, &vtArguments);
    
    // 4. Invoke
    InvokeMethod(vtEmpty, pStartArguments, &vtResult);
}
```

---

## PowerShell Class API

### 1. Tạo PowerShell Instance

```cpp
BOOL PowerShellCreate(mscorlib::_AppDomain* pAppDomain, VARIANT* pvtPowerShellInstance)
{
    // Tương đương: $ps = [System.Management.Automation.PowerShell]::Create()
    
    GetType("System.Management.Automation.PowerShell");
    GetMethod("Create", 0);  // Static, no args
    InvokeMethod(vtEmpty, NULL, &vtInstance);
    memcpy(pvtPowerShellInstance, &vtInstance, sizeof(vtInstance));
}
```

### 2. Thêm Script

```cpp
BOOL PowerShellAddScript(
    mscorlib::_AppDomain* pAppDomain, 
    VARIANT vtPowerShellInstance, 
    LPWSTR pwszScript
)
{
    // Tương đương: $ps.AddScript($script)
    
    GetMethod("AddScript", 1);  // 1 argument
    
    VARIANT vtScript;
    InitVariantFromString(pwszScript, &vtScript);
    
    SAFEARRAY* pArgs = SafeArrayCreateVector(VT_VARIANT, 0, 1);
    SafeArrayPutElement(pArgs, 0, &vtScript);
    
    InvokeMethod(vtPowerShellInstance, pArgs, &vtResult);
}
```

### 3. Thêm Command

```cpp
BOOL PowerShellAddCommand(
    mscorlib::_AppDomain* pAppDomain, 
    VARIANT vtPowerShellInstance, 
    LPCWSTR pwszCommand
)
{
    // Tương đương: $ps.AddCommand("Out-String").AddParameter("stream", $true)
    
    GetMethod("AddCommand", 2);  // 2 arguments
    
    VARIANT vtCommand;
    VARIANT vtUseLocalScope;
    
    InitVariantFromString(pwszCommand, &vtCommand);
    InitVariantFromBoolean(FALSE, &vtUseLocalScope);  // Không dùng local scope
    
    SAFEARRAY* pArgs = SafeArrayCreateVector(VT_VARIANT, 0, 2);
    SafeArrayPutElement(pArgs, 0, &vtCommand);
    SafeArrayPutElement(pArgs, 1, &vtUseLocalScope);
    
    InvokeMethod(vtPowerShellInstance, pArgs, &vtResult);
}
```

### 4. Invoke Pipeline

```cpp
BOOL PowerShellInvoke(
    mscorlib::_AppDomain* pAppDomain, 
    VARIANT vtPowerShellInstance, 
    VARIANT* pvtInvokeResult
)
{
    // Tương đương: $result = $ps.Invoke()
    
    GetMethod("Invoke", 0);  // 0 arguments, instance method
    
    InvokeMethod(vtPowerShellInstance, NULL, &vtResult);
    memcpy(pvtInvokeResult, &vtResult, sizeof(vtResult));
}
```

### 5. Dispose

```cpp
BOOL PowerShellDispose(
    mscorlib::_AppDomain* pAppDomain, 
    VARIANT vtPowerShellInstance
)
{
    GetMethod("Dispose", 0);
    InvokeMethod(vtPowerShellInstance, NULL, &vtResult);
}
```

---

## Streams và Error Handling

### PowerShell Streams

PowerShell có 6 output streams:

| Stream | Number | Mục đích | Thường dùng |
|--------|--------|----------|--------------|
| **Output** | 1 | Output chính | `$output` |
| **Error** | 2 | Error records | `$ps.Streams.Error` |
| **Warning** | 3 | Warning messages | `$ps.Streams.Warning` |
| **Verbose** | 4 | Verbose output | `$VerbosePreference` |
| **Debug** | 5 | Debug messages | `$DebugPreference` |
| **Information** | 6 | Info messages | `$ps.Streams.Information` |

### Kiểm tra Errors

```cpp
BOOL PowerShellHadErrors(
    mscorlib::_AppDomain* pAppDomain, 
    VARIANT vtPowerShellInstance, 
    PBOOL pbHadErrors
)
{
    // Đọc property HadErrors
    GetPropertyValue("HadErrors", &vtHadErrors);
    
    *pbHadErrors = vtHadErrors.boolVal;  // VT_BOOL
}
```

### Đọc Stream

```cpp
BOOL PowerShellGetStream(
    mscorlib::_AppDomain* pAppDomain, 
    VARIANT vtPowerShellInstance, 
    LPCWSTR pwszStreamName,      // L"Error", L"Warning", L"Information"
    VARIANT* pvtStream
)
{
    // 1. Lấy property Streams
    GetPropertyValue("Streams", &vtStreams);
    
    // 2. Lấy type PSDataStreams
    GetType("System.Management.Automation.PSDataStreams");
    
    // 3. Đọc stream bằng tên
    GetPropertyValue("Error", &vtStream);  // Hoặc Warning, Information...
}
```

---

## Output Formatting

### Đọc kết quả Invoke

```cpp
void PrintPowerShellInvokeResult(mscorlib::_AppDomain* pAppDomain, VARIANT vtInvokeResult)
{
    // $result.Count - số lượng items
    VARIANT vtCount = GetProperty("Count");
    VARIANT vtCountValue;
    PropertyInfo_GetValue(vtCount, vtInvokeResult, &vtCountValue);
    
    // $result.Item[i] - từng item
    for (int i = 0; i < vtCountValue.lVal; i++)
    {
        VARIANT vtItem = GetItem(vtInvokeResult, i);
        
        // Gọi .ToString() để lấy string representation
        VARIANT vtString;
        InvokeMethod("ToString", vtItem, &vtString);
        
        wprintf(L"%ws", vtString.bstrVal);
    }
}
```

**Output format:**
```
+-----------------------------------+
| POWERSHELL STANDARD OUTPUT STREAM |
+-----------------------------------+
<result lines here>
```

### Đọc Error Records

```cpp
void PrintErrorRecord(mscorlib::_AppDomain* pAppDomain, VARIANT vtErrorRecord)
{
    // TargetObject - Object that caused the error
    VARIANT vtTargetObject = GetProperty("TargetObject");
    
    // ScriptStackTrace - Stack trace
    VARIANT vtScriptStackTrace = GetProperty("ScriptStackTrace");
    
    // CategoryInfo - Error category
    VARIANT vtCategoryInfo = GetProperty("CategoryInfo");
    
    // Exception.Message
    VARIANT vtException = GetProperty("Exception");
    VARIANT vtExceptionMessage = GetProperty("Message");
    
    // In định dạng PowerShell
    wprintf(L"%ws : %ws\n", vtTargetObject.bstrVal, vtExceptionMessage.bstrVal);
    wprintf(L"%ws\n", vtScriptStackTrace.bstrVal);
    wprintf(L"    + CategoryInfo : %ws\n", vtCategoryInfo.bstrVal);
}
```

---

## Interactive Console

### Luồng tạo Console

```
1. Initialize CLR
         ↓
2. Create RunspaceConfiguration
         ↓
3. StartConsoleShell()
         ↓
4. User interacts with PowerShell
         ↓
5. User exits (Ctrl+C, exit command)
         ↓
6. Cleanup
```

### Code flow

```cpp
void CreatePowerShellConsole()
{
    mscorlib::_AppDomain* pAppDomain = NULL;
    CLR_CONTEXT cc = { 0 };
    VARIANT vtConfig = { 0 };
    
    // Banner và Help
    LPCWSTR pwszBanner = L"Windows PowerShell\nCopyright (C) Microsoft Corporation...";
    LPCWSTR pwszHelp = L"Help message";
    LPCWSTR ppwszArgs[] = { NULL };
    
    // 1. Initialize CLR
    if (!clr::InitializeCommonLanguageRuntime(&cc, &pAppDomain))
        goto exit;
    
    // 2. Create RunspaceConfiguration  
    if (!CreateInitialRunspaceConfiguration(pAppDomain, &vtConfig))
        goto exit;
    
    // 3. Start shell
    if (!StartConsoleShell(pAppDomain, vtConfig, pwszBanner, pwszHelp, ppwszArgs, 1))
        goto exit;
    
exit:
    VariantClear(&vtConfig);
    clr::DestroyCommonLanguageRuntime(&cc, pAppDomain);
}
```

---

## Luồng thực thi

### Execute Script Flow

```
┌─────────────────────────────────────────────────────────────┐
│                      ExecutePowerShellScript                 │
├─────────────────────────────────────────────────────────────┤
│  1. Initialize CLR                                          │
│     clr::InitializeCommonLanguageRuntime()                  │
│         ↓                                                   │
│  2. Create PowerShell Instance                              │
│     PowerShell::Create()                                    │
│         ↓                                                   │
│  3. Add Script                                              │
│     PowerShell::AddScript("Get-Process")                     │
│         ↓                                                   │
│  4. Add Command (optional)                                  │
│     PowerShell::AddCommand("Out-String")                     │
│         ↓                                                   │
│  5. Invoke                                                  │
│     PowerShell::Invoke()                                    │
│         ↓                                                   │
│  6. Process Results                                         │
│     ├─ Output Stream (primary)                              │
│     ├─ Error Stream                                         │
│     ├─ Warning Stream                                       │
│     ├─ Information Stream                                   │
│     └─ Verbose/Debug Streams                                │
│         ↓                                                   │
│  7. Check HadErrors                                         │
│         ↓                                                   │
│  8. Print Errors if any                                    │
│         ↓                                                   │
│  9. Cleanup                                                 │
│     PowerShell::Dispose()                                   │
│     CLR::Destroy()                                          │
└─────────────────────────────────────────────────────────────┘
```

### Ví dụ: Execute Script đơn giản

```cpp
void ExecutePowerShellScript(LPWSTR pwszScript)
{
    VARIANT vtPowerShell = { 0 };
    VARIANT vtResult = { 0 };
    BOOL bHadErrors = FALSE;
    
    // 1. Initialize
    clr::InitializeCommonLanguageRuntime(&cc, &pAppDomain);
    
    // 2. Create PowerShell
    PowerShellCreate(pAppDomain, &vtPowerShell);
    
    // 3. Add script
    PowerShellAddScript(pAppDomain, vtPowerShell, pwszScript);
    
    // 4. Add Out-String (convert objects to string)
    PowerShellAddCommand(pAppDomain, vtPowerShell, L"Out-String");
    
    // 5. Invoke
    if (PowerShellInvoke(pAppDomain, vtPowerShell, &vtResult))
    {
        PrintPowerShellInvokeResult(pAppDomain, vtResult);
        PrintPowerShellInvokeInformation(pAppDomain, vtPowerShell);
    }
    
    // 6. Check errors
    if (PowerShellHadErrors(pAppDomain, vtPowerShell, &bHadErrors) && bHadErrors)
    {
        PrintPowerShellInvokeErrors(pAppDomain, vtPowerShell);
    }
    
    // 7. Cleanup
    PowerShellDispose(pAppDomain, vtPowerShell);
    clr::DestroyCommonLanguageRuntime(&cc, pAppDomain);
}
```

---

## So sánh với powershell.exe

| Khía cạnh | `powershell.exe` | PowerShell Hosting |
|-----------|------------------|-------------------|
| **Control** | Process-level | API-level |
| **Results** | Text parsing | Structured objects |
| **Streams** | Redirects only | Full access |
| **Errors** | Exit code | Full ErrorRecord |
| **Lifecycle** | Spawn process | In-process |
| **Performance** | Slow (process) | Fast (in-process) |
| **Dependencies** | External binary | CLR + managed assemblies |

### Khi nào dùng Hosting?

**Dùng `powershell.exe`:**
- Quick scripts, prototyping
- Simple automation
- Khi không cần xử lý structured data

**Dùng Hosting API:**
- Deep integration
- Real-time output processing
- Multiple script executions
- Error handling phức tạp
- Performance critical applications

---

## Tài liệu tham khảo

1. [MSDN: Windows PowerShell Hosting](https://docs.microsoft.com/en-us/previous-versions/windows/desktop/ms714659(v=vs.85))
2. [MSDN: RunspaceConfiguration Class](https://docs.microsoft.com/en-us/dotnet/api/system.management.automation.runspaces.runspaceconfiguration)
3. [MSDN: PowerShell Class](https://docs.microsoft.com/en-us/dotnet/api/system.management.automation.powershell)
4. [MSDN: PSDataStreams](https://docs.microsoft.com/en-us/dotnet/api/system.management.automation.psdatastreams)
5. [PowerShell Gallery: Hosting Examples](https://www.powershellgallery.com/packages)

---

## Ví dụ sử dụng

### Basic Script Execution
```cpp
// Execute: Get-Process | Select-Object -First 5 Name, CPU
ExecutePowerShellScript(L"Get-Process | Select-Object -First 5 Name, CPU");
```

### With Error Handling
```cpp
void SafeExecute(LPWSTR script)
{
    VARIANT vtResult, vtPowerShell;
    BOOL bHadErrors;
    
    PowerShellCreate(pAppDomain, &vtPowerShell);
    PowerShellAddScript(pAppDomain, vtPowerShell, script);
    PowerShellInvoke(pAppDomain, vtPowerShell, &vtResult);
    
    if (PowerShellHadErrors(pAppDomain, vtPowerShell, &bHadErrors) && bHadErrors)
    {
        PrintPowerShellInvokeErrors(pAppDomain, vtPowerShell);
        return;
    }
    
    PrintPowerShellInvokeResult(pAppDomain, vtResult);
    PowerShellDispose(pAppDomain, vtPowerShell);
}
```

---

*Document generated: August 2026*
