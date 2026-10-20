using System;
using System.Diagnostics;
using System.IO;
using System.IO.Pipes;
using System.Runtime.InteropServices;
using System.Security.Principal;
using System.Text;
using System.Threading;
using System.Collections.Concurrent;
using System.Web.Script.Serialization;
using Microsoft.Win32.SafeHandles;
using Hhub.Monitoring;

[assembly: System.Reflection.AssemblyTitle("H hub")]
[assembly: System.Reflection.AssemblyProduct("H hub")]
[assembly: System.Reflection.AssemblyDescription("H hub")]

// Demand-driven transport host; adapters and source selection are independent.
internal static class Sensors
{
    [DllImport("kernel32.dll", SetLastError = true)]
    static extern bool GetNamedPipeServerProcessId(SafePipeHandle pipe, out uint pid);
    [DllImport("shell32.dll", CharSet = CharSet.Unicode)]
    static extern int SetCurrentProcessExplicitAppUserModelID(string id);
    sealed class Session : IDisposable
    {
        public TextReader Input = Console.In;
        public TextWriter Output = Console.Out;
        NamedPipeClientStream pipe;
        Timer watchdog;
        Process parent;
        readonly BlockingCollection<string> commands = new BlockingCollection<string>(1);
        long lastCommand = Stopwatch.GetTimestamp(), stopRequested;
        int stopping, reading;
        public void Connect(string[] args)
        {
            if (args.Length == 0) return;
            int parentId; Guid nonce;
            string prefix = "Hhub.Sensors." + WindowsIdentity.GetCurrent().User.Value + ".";
            if (args.Length != 4 || args[0] != "--pipe" || args[2] != "--parent" ||
                !args[1].StartsWith(prefix, StringComparison.Ordinal) || !Guid.TryParse(args[1].Substring(prefix.Length), out nonce) ||
                !Int32.TryParse(args[3], out parentId) || parentId <= 0) throw new ArgumentException("Invalid sensor session arguments");
            parent = Process.GetProcessById(parentId);
            string expected = Path.GetFullPath(Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "..", "core", "Hhub.exe"));
            if (!String.Equals(parent.MainModule.FileName, expected, StringComparison.OrdinalIgnoreCase)) throw new UnauthorizedAccessException("Unexpected sensor parent");
            pipe = new NamedPipeClientStream(".", args[1], PipeDirection.InOut, PipeOptions.Asynchronous, TokenImpersonationLevel.Identification);
            pipe.Connect(10000);
            uint server;
            if (!GetNamedPipeServerProcessId(pipe.SafePipeHandle, out server) || server != (uint)parentId) throw new UnauthorizedAccessException("Unexpected pipe server");
            Input = new StreamReader(pipe, new UTF8Encoding(false));
            Output = new StreamWriter(pipe, new UTF8Encoding(false)) { AutoFlush = true };
        }
        static double SecondsSince(long time) { return (Stopwatch.GetTimestamp() - time) / (double)Stopwatch.Frequency; }
        void Stop()
        {
            if (Interlocked.Exchange(ref stopping, 1) == 0) Interlocked.Exchange(ref stopRequested, Stopwatch.GetTimestamp());
            try { commands.CompleteAdding(); } catch (ObjectDisposedException) { }
        }
        public void BeginCommands()
        {
            // Read cancellation independently of hardware discovery/update. A
            // stalled driver must not keep an elevated helper resident forever.
            new Thread(delegate() {
                try {
                    string command;
                    while ((command = Input.ReadLine()) != null) {
                        Interlocked.Exchange(ref lastCommand, Stopwatch.GetTimestamp());
                        if (command == "quit") break;
                        if (command == "sample") commands.TryAdd(command);
                    }
                } catch (IOException) { } catch (ObjectDisposedException) { }
                finally { Stop(); }
            }) { IsBackground = true }.Start();
            watchdog = new Timer(delegate {
                try {
                    if (parent != null && parent.HasExited) Environment.Exit(0);
                    if (Volatile.Read(ref stopping) != 0 && SecondsSince(Interlocked.Read(ref stopRequested)) > 2) Environment.Exit(0);
                    if (SecondsSince(Interlocked.Read(ref lastCommand)) > (Volatile.Read(ref reading) != 0 ? 60 : 15)) Environment.Exit(0);
                } catch { Environment.Exit(0); }
            }, null, 500, 500);
        }
        public string NextCommand()
        {
            string command;
            while (Volatile.Read(ref stopping) == 0) if (commands.TryTake(out command, 500)) return command;
            return null;
        }
        public void Reading(bool active) { Volatile.Write(ref reading, active ? 1 : 0); }
        public void Dispose() { if (watchdog != null) watchdog.Dispose(); if (pipe != null) pipe.Dispose(); if (parent != null) parent.Dispose(); commands.Dispose(); }
    }
    public static int Main(string[] args)
    {
        Marshal.ThrowExceptionForHR(SetCurrentProcessExplicitAppUserModelID("Hhub.Desktop"));
        Console.OutputEncoding = new UTF8Encoding(false);
        var json = new JavaScriptSerializer { MaxJsonLength = 1048576 };
        try {
            using (var session = new Session()) {
                session.Connect(args);
                session.BeginCommands();
                using (var pipeline = MonitoringSources.Create(Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "monitor-sources.json"))) {
                    string command;
                    while ((command = session.NextCommand()) != null) {
                        if (command == "quit") break;
                        if (command != "sample") continue;
                        session.Reading(true);
                        var sample = pipeline.Read();
                        session.Reading(false);
                        session.Output.WriteLine(json.Serialize(sample));
                        session.Output.Flush();
                    }
                }
            }
            return 0;
        } catch (IOException) { return 0; }
        catch (Exception error) { Console.Error.WriteLine(error.GetType().Name + ": " + error.Message); return 1; }
    }
}
