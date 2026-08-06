using System;
using System.Linq;
using System.Management.Automation;

namespace PowershellCS
{
    internal class Program
    {
        static void Main(string[] args)
        {
            using (PowerShell ps = PowerShell.Create())
            {
                while (true)
                {
                    ps.Commands.Clear();
                    ps.AddScript("(Get-Location).Path");

                    string currentPath = ps.Invoke().FirstOrDefault()?.ToString();

                    Console.Write($"PS {currentPath}> ");

                    string cmd = Console.ReadLine();

                    if (string.IsNullOrWhiteSpace(cmd))
                        continue;

                    if (cmd.Equals("exit", StringComparison.OrdinalIgnoreCase))
                        break;

                    ps.Commands.Clear();
                    ps.AddScript(cmd);

                    var results = ps.Invoke();

                    foreach (PSObject obj in results)
                        Console.WriteLine(obj);

                    if (ps.HadErrors)
                    {
                        foreach (var err in ps.Streams.Error)
                            Console.WriteLine(err);
                    }

                    ps.Streams.Error.Clear();
                }
            }

        }
    }
}
