# PowershellHosting DLL

Load CLR + `System.Management.Automation` in-process, chạy PowerShell pipeline, return stdout + errors dạng text.

**Single global session, không thread-safe.** Caller tự sync.

## Exported API

| Hàm | Trả về | Mô tả |
|---|---|---|
| `PS_Init()` | `BOOL` | Setup CLR, load assembly, lookup methods, create PowerShell instance, apply patches (AMSI/ETW/Transcription). Idempotent. |
| `PS_Execute(script)` | `BSTR` | Chạy 1 script. Return stdout + errors (nếu có). Caller `SysFreeString`. `NULL` = lỗi. |
| `PS_Reset()` | `BOOL` | Dispose instance cũ + tạo mới. Gọi khi script corrupt state. |
| `PS_Shutdown()` | `void` | Cleanup toàn bộ. Idempotent. |

Khi script có lỗi, output format:
```
<stdout>

[ERR]
<TargetObject> : <Exception.Message>
<ScriptStackTrace>
+ <TargetObject>
+ ~~~
    + CategoryInfo          : <msg>
    + FullyQualifiedErrorId : <id>

<terminating exception.ToString() nếu có>
```

## C++ usage

```cpp
#include <windows.h>
#include <comdef.h>

typedef BOOL (*PS_Init_t)();
typedef BSTR (*PS_Execute_t)(LPCWSTR);
typedef BOOL (*PS_Reset_t)();
typedef void (*PS_Shutdown_t)();

HMODULE h = LoadLibraryA("PowershellHosting.dll");
auto PS_Init     = (PS_Init_t)GetProcAddress(h, "PS_Init");
auto PS_Execute  = (PS_Execute_t)GetProcAddress(h, "PS_Execute");
auto PS_Reset    = (PS_Reset_t)GetProcAddress(h, "PS_Reset");
auto PS_Shutdown = (PS_Shutdown_t)GetProcAddress(h, "PS_Shutdown");

if (!PS_Init()) { /* failed */ return; }

BSTR out = PS_Execute(L"Get-Process | Select-Object -First 3 Name");
if (out) {
    wprintf(L"%s\n", out);
    SysFreeString(out);
}

PS_Shutdown();
FreeLibrary(h);
```

## Error handling

- `PS_Execute` return `NULL` → lỗi hệ thống (CLR không khởi tạo, OOM, v.v.).
- `PS_Execute` return non-NULL có chứa `[ERR]` → script chạy thành công nhưng PowerShell ghi error records. Instance vẫn reset sau đó — gọi tiếp OK.
- `PS_Execute` return non-NULL không có `[ERR]` nhưng caller nghi script corrupt → gọi `PS_Reset()` trước khi execute tiếp.

## Threading

Global state. Một caller một thời điểm. Nếu cần đa luồng: wrap calls bằng `CRITICAL_SECTION`.
