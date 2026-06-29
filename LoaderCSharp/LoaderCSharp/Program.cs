using System;
using System.Reflection;
using System.Reflection.Emit;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;

namespace LoaderCSharp;

internal static class Program
{
    // [original] Returns a pointer inside JIT-compiled code.
    // NOTE: the method body is just IL "Console.WriteLine(randomString)" padded N times.
    // It is NOT an RWX buffer for arbitrary shellcode.
    public static IntPtr GenerateRWXMemory(int ByteCount)
    {
        AssemblyName name = new AssemblyName("Assembly");
        AssemblyBuilder assemblyBuilder =
            AssemblyBuilder.DefineDynamicAssembly(name, AssemblyBuilderAccess.Run);
        ModuleBuilder moduleBuilder = assemblyBuilder.DefineDynamicModule("Module");
        MethodBuilder methodBuilder = moduleBuilder.DefineGlobalMethod(
            "MethodName",
            MethodAttributes.FamANDAssem | MethodAttributes.Family | MethodAttributes.Static,
            typeof(void),
            new Type[0]);
        ILGenerator il = methodBuilder.GetILGenerator();

        while (ByteCount > 0)
        {
            int num = 4;
            StringBuilder sb = new StringBuilder();
            Random random = new Random();
            for (int i = 0; i < num; i++)
            {
                double num2 = random.NextDouble();
                int num3 = Convert.ToInt32(Math.Floor(25.0 * num2));
                char value = Convert.ToChar(num3 + 65);
                sb.Append(value);
            }
            il.EmitWriteLine(sb.ToString());
            ByteCount -= 18;
        }
        il.Emit(OpCodes.Ret);
        moduleBuilder.CreateGlobalFunctions();

        RuntimeMethodHandle methodHandle = moduleBuilder.GetMethods()[0].MethodHandle;
        RuntimeHelpers.PrepareMethod(methodHandle);
        return methodHandle.GetFunctionPointer();
    }

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern bool VirtualQuery(IntPtr lpAddress, out MEMORY_BASIC_INFORMATION lpBuffer, uint dwLength);

    [StructLayout(LayoutKind.Sequential)]
    private struct MEMORY_BASIC_INFORMATION
    {
        public IntPtr BaseAddress;
        public IntPtr AllocationBase;
        public uint   AllocationProtect;
        public IntPtr RegionSize;
        public uint   State;
        public uint   Protect;
        public uint   Type;
    }

    private static int Main()
    {
        IntPtr p = GenerateRWXMemory(2048);
        Console.WriteLine($"Pointer: 0x{p:X}");

        if (VirtualQuery(p, out var mbi, (uint)Marshal.SizeOf<MEMORY_BASIC_INFORMATION>()))
        {
            Console.WriteLine($"BaseAddress: 0x{mbi.BaseAddress:X}");
            Console.WriteLine($"RegionSize : 0x{mbi.RegionSize:X}");
            Console.WriteLine($"State      : 0x{mbi.State:X}");
            Console.WriteLine($"Protect    : 0x{mbi.Protect:X}");
            Console.WriteLine($"RWX?       : {mbi.Protect == 0x40}");
        }
        else
        {
            Console.WriteLine($"VirtualQuery failed: {Marshal.GetLastWin32Error()}");
        }

        // Probe a few bytes around the pointer to show what's there.
        Console.Write("Bytes    : ");
        for (int i = 0; i < 32; i++)
            Console.Write($"{Marshal.ReadByte(p, i):X2} ");
        Console.WriteLine();

        return 0;
    }
}