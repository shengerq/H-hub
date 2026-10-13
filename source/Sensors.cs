using System;
using System.Diagnostics;
using System.IO;
using System.IO.Pipes;
using System.Runtime.InteropServices;
using System.Security.Principal;
using System.Text;
using System.Threading;
using System.Web.Script.Serialization;
using Microsoft.Win32.SafeHandles;
using Hhub.Monitoring;

// Demand-driven transport host; adapters and source selection are independent.
internal static class Sensors
{
    [DllImport("kernel32.dll", SetLastError = true)]
    static extern bool GetNamedPipeServerProcessId(SafePipeHandle pipe, out uint pid);
    sealed class Session : IDisposable
    {
        public TextReader Input = Console.In;
        public TextWriter Output = Console.Out;
        NamedPipeClientStream pipe;
        Timer watchdog;
        public void Connect(string[] args)
        {
            if (args.Length == 0) return;
            int parentId; Guid nonce;
            string prefix = "Hhub.Sensors." + WindowsIdentity.GetCurrent().User.Value + ".";
            if (args.Length != 4 || args[0] != "--pipe" || args[2] != "--parent" ||
                !args[1].StartsWith(prefix, StringComparison.Ordinal) || !Guid.TryParse(args[1].Substring(prefix.Length), out nonce) ||
                !Int32.TryParse(args[3], out parentId) || parentId <= 0) throw new ArgumentException("Invalid sensor session arguments");
            var parent = Process.GetProcessById(parentId);
            string expected = Path.GetFullPath(Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "..", "Hhub.Core.exe"));
            if (!String.Equals(parent.MainModule.FileName, expected, StringComparison.OrdinalIgnoreCase)) throw new UnauthorizedAccessException("Unexpected sensor parent");
            pipe = new NamedPipeClientStream(".", args[1], PipeDirection.InOut, PipeOptions.Asynchronous, TokenImpersonationLevel.Identification);
            pipe.Connect(10000);
            uint server;
            if (!GetNamedPipeServerProcessId(pipe.SafePipeHandle, out server) || server != (uint)parentId) throw new UnauthorizedAccessException("Unexpected pipe server");
            Input = new StreamReader(pipe, new UTF8Encoding(false));
            Output = new StreamWriter(pipe, new UTF8Encoding(false)) { AutoFlush = true };
            watchdog = new Timer(delegate { try { if (parent.HasExited) Environment.Exit(0); } catch { Environment.Exit(0); } }, null, 1000, 1000);
        }
        public void Dispose() { if (watchdog != null) watchdog.Dispose(); if (pipe != null) pipe.Dispose(); }
    }
    public static int Main(string[] args)
    {
        Console.OutputEncoding = new UTF8Encoding(false);
        var json = new JavaScriptSerializer { MaxJsonLength = 1048576 };
        try {
            using (var session = new Session()) {
                session.Connect(args);
                using (var pipeline = MonitoringSources.Create(Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "monitor-sources.json"))) {
                    string command;
                    while ((command = session.Input.ReadLine()) != null) {
                        if (command == "quit") break;
                        if (command != "sample") continue;
                        session.Output.WriteLine(json.Serialize(pipeline.Read()));
                        session.Output.Flush();
                    }
                }
            }
            return 0;
        } catch (IOException) { return 0; }
        catch (Exception error) { Console.Error.WriteLine(error.GetType().Name + ": " + error.Message); return 1; }
    }
}
