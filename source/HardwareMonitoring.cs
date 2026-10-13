using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Management;
using System.Security.Principal;
using System.Web.Script.Serialization;
using LibreHardwareMonitor.Hardware;

namespace Hhub.Monitoring
{
    // A backend owns hardware discovery. Supplements can fill missing metrics, never overwrite them.
    public interface IHardwareMonitorBackend : IDisposable
    {
        string Id { get; }
        BackendSample Read();
    }
    public interface IMonitoringSupplement : IDisposable
    {
        string Id { get; }
        SupplementSample Read(BackendSample primary);
    }
    public sealed class Reading
    {
        public string id, name, hardware, hardwareId, category, type, source;
        public double? value;
    }
    public sealed class HardwareIdentity
    {
        public string id, name, category;
        public int sensorCount;
    }
    public sealed class Metric
    {
        public double? value;
        public string unit, source, status;
        public Metric(double? number, string units, string provider)
        {
            value = number.HasValue && !Double.IsNaN(number.Value) && !Double.IsInfinity(number.Value) && number.Value >= 0 ? number : null;
            unit = units; source = provider; status = value.HasValue ? "available" : "unavailable";
        }
    }
    public sealed class Volume
    {
        public string id, name;
        public double totalGB, freeGB, usedGB;
    }
    public sealed class BackendSample
    {
        public string source;
        public bool driverInstalled;
        public readonly List<Reading> sensors = new List<Reading>();
        public readonly List<HardwareIdentity> hardware = new List<HardwareIdentity>();
        public readonly List<string> errors = new List<string>();
        public readonly Dictionary<string, Metric> metrics = new Dictionary<string, Metric>(StringComparer.Ordinal);
    }
    public sealed class SupplementSample
    {
        public readonly Dictionary<string, Metric> metrics = new Dictionary<string, Metric>(StringComparer.Ordinal);
        public readonly List<Volume> volumes = new List<Volume>();
        public readonly List<string> errors = new List<string>();
    }
    public sealed class LibreHardwareBackend : IHardwareMonitorBackend
    {
        Computer computer;
        bool opened;
        public string Id { get { return "librehardwaremonitor"; } }
        public BackendSample Read()
        {
            var sample = new BackendSample { source = "LibreHardwareMonitor 0.9.6" };
            try {
                if (computer == null) computer = new Computer {
                    IsCpuEnabled = true, IsGpuEnabled = true, IsMemoryEnabled = true,
                    IsStorageEnabled = true, IsBatteryEnabled = true, IsMotherboardEnabled = false,
                    IsControllerEnabled = false, IsNetworkEnabled = false
                };
                if (!opened) { computer.Open(); opened = true; }
                foreach (var item in computer.Hardware) ReadHardware(item, sample);
                sample.driverInstalled = LibreHardwareMonitor.PawnIo.PawnIo.IsInstalled;
                Normalize(sample);
            } catch (Exception error) { sample.errors.Add(Id + ": " + error.Message); }
            return sample;
        }
        void ReadHardware(IHardware item, BackendSample sample)
        {
            bool updated = true;
            try { item.Update(); } catch (Exception error) { updated = false; sample.errors.Add(item.Name + ": " + error.Message); }
            sample.hardware.Add(new HardwareIdentity { id = item.Identifier.ToString(), name = item.Name,
                category = item.HardwareType.ToString(), sensorCount = item.Sensors.Length });
            foreach (var sensor in item.Sensors) {
                var value = updated ? sensor.Value : null;
                sample.sensors.Add(new Reading { id = sensor.Identifier.ToString(), name = sensor.Name,
                    hardware = item.Name, hardwareId = item.Identifier.ToString(), category = item.HardwareType.ToString(),
                    type = sensor.SensorType.ToString(), source = Id,
                    value = value.HasValue && !Single.IsNaN(value.Value) && !Single.IsInfinity(value.Value) ? (double?)value.Value : null });
            }
            foreach (var child in item.SubHardware) ReadHardware(child, sample);
        }
        static void Normalize(BackendSample sample)
        {
            var memory = sample.sensors.Where(s => s.category == "Memory" && s.hardwareId == "/ram").ToList();
            Add(sample, "memoryUsedGB", Find(memory, "Data", "Memory Used"), "GiB");
            var free = Find(memory, "Data", "Memory Available");
            var used = Find(memory, "Data", "Memory Used");
            Add(sample, "memoryTotalGB", used.HasValue && free.HasValue ? used + free : null, "GiB");
            Add(sample, "memory", Find(memory, "Load", "Memory"), "%");
            var disks = sample.sensors.Where(s => s.category == "Storage").GroupBy(s => s.hardwareId).ToList();
            AggregateDisk(sample, disks, "diskReadMB", "Throughput", "Read Rate", "MiB/s", 1048576);
            AggregateDisk(sample, disks, "diskWriteMB", "Throughput", "Write Rate", "MiB/s", 1048576);
            AggregateDisk(sample, disks, "diskTotalGB", "Data", "Total Space", "GB", 1);
            double occupied = 0; bool complete = disks.Count > 0;
            foreach (var disk in disks) {
                var total = Find(disk, "Data", "Total Space"); var available = Find(disk, "Data", "Free Space");
                var load = Find(disk, "Load", "Used Space");
                if (total.HasValue && available.HasValue && available <= total) occupied += total.Value - available.Value;
                else if (total.HasValue && load.HasValue && load >= 0 && load <= 100) occupied += total.Value * load.Value / 100;
                else complete = false;
            }
            Add(sample, "diskUsedGB", complete ? (double?)occupied : null, "GB");
        }
        static double? Find(IEnumerable<Reading> readings, string type, string name)
        {
            var match = readings.FirstOrDefault(s => s.type == type && s.name == name && s.value.HasValue && s.value >= 0);
            return match == null ? null : match.value;
        }
        static void AggregateDisk(BackendSample sample, IList<IGrouping<string, Reading>> disks, string key, string type, string name, string unit, double divisor)
        {
            double total = 0; bool complete = disks.Count > 0;
            foreach (var disk in disks) { var value = Find(disk, type, name); if (!value.HasValue) complete = false; else total += value.Value / divisor; }
            Add(sample, key, complete ? (double?)total : null, unit);
        }
        static void Add(BackendSample sample, string key, double? value, string unit) { sample.metrics[key] = new Metric(value, unit, "librehardwaremonitor"); }
        public void Dispose() { if (computer != null) { try { computer.Close(); } finally { computer = null; opened = false; } } }
    }
    public sealed class WindowsStorageSupplement : IMonitoringSupplement
    {
        public string Id { get { return "windows-storage"; } }
        static bool Missing(BackendSample primary, string key) { Metric value; return !primary.metrics.TryGetValue(key, out value) || !value.value.HasValue; }
        public SupplementSample Read(BackendSample primary)
        {
            var result = new SupplementSample();
            if (Missing(primary, "diskUsedGB") || Missing(primary, "diskTotalGB")) {
                double total = 0, used = 0;
                foreach (var drive in DriveInfo.GetDrives()) try {
                    if (drive.DriveType != DriveType.Fixed || !drive.IsReady) continue;
                    double size = drive.TotalSize / 1000000000.0, free = drive.TotalFreeSpace / 1000000000.0;
                    result.volumes.Add(new Volume { id = drive.Name, name = drive.Name, totalGB = size, freeGB = free, usedGB = size - free });
                    total += size; used += size - free;
                } catch (Exception error) { result.errors.Add(Id + " volume: " + error.Message); }
                if (result.volumes.Count > 0) { result.metrics["diskTotalGB"] = new Metric(total, "GB", Id); result.metrics["diskUsedGB"] = new Metric(used, "GB", Id); }
            }
            if (Missing(primary, "diskReadMB") || Missing(primary, "diskWriteMB")) try {
                var options = new EnumerationOptions { Timeout = TimeSpan.FromSeconds(2), ReturnImmediately = false };
                using (var query = new ManagementObjectSearcher("root\\cimv2", "SELECT DiskReadBytesPersec,DiskWriteBytesPersec FROM Win32_PerfFormattedData_PerfDisk_PhysicalDisk WHERE Name='_Total'", options))
                using (var rows = query.Get()) foreach (ManagementObject row in rows) using (row) {
                    if (row["DiskReadBytesPersec"] != null) result.metrics["diskReadMB"] = new Metric(Convert.ToDouble(row["DiskReadBytesPersec"]) / 1048576.0, "MiB/s", Id);
                    if (row["DiskWriteBytesPersec"] != null) result.metrics["diskWriteMB"] = new Metric(Convert.ToDouble(row["DiskWriteBytesPersec"]) / 1048576.0, "MiB/s", Id);
                }
            } catch (Exception error) { result.errors.Add(Id + " counters: " + error.Message); }
            return result;
        }
        public void Dispose() { }
    }
    public sealed class MonitoringPipeline : IDisposable
    {
        readonly IHardwareMonitorBackend backend;
        readonly IList<IMonitoringSupplement> supplements;
        readonly List<string> configurationErrors;
        public MonitoringPipeline(IHardwareMonitorBackend primary, IList<IMonitoringSupplement> extra, IEnumerable<string> errors = null)
        {
            if (primary == null) throw new ArgumentNullException("primary");
            backend = primary; supplements = extra ?? new List<IMonitoringSupplement>();
            configurationErrors = errors == null ? new List<string>() : new List<string>(errors);
        }
        public Dictionary<string, object> Read()
        {
            BackendSample sample;
            try { sample = backend.Read(); if (sample == null) throw new InvalidOperationException("Backend returned no sample"); }
            catch (Exception error) { sample = new BackendSample { source = backend.Id }; sample.errors.Add(backend.Id + ": " + error.Message); }
            sample.errors.AddRange(configurationErrors);
            var volumes = new List<Volume>();
            foreach (var extra in supplements) try {
                var addition = extra.Read(sample); if (addition == null) throw new InvalidOperationException("Supplement returned no sample");
                foreach (var entry in addition.metrics) {
                    Metric current;
                    if (entry.Value != null && entry.Value.value.HasValue && (!sample.metrics.TryGetValue(entry.Key, out current) || current == null || !current.value.HasValue)) sample.metrics[entry.Key] = entry.Value;
                }
                volumes.AddRange(addition.volumes); sample.errors.AddRange(addition.errors);
            } catch (Exception error) { sample.errors.Add(extra.Id + ": " + error.Message); }
            bool elevated = new WindowsPrincipal(WindowsIdentity.GetCurrent()).IsInRole(WindowsBuiltInRole.Administrator);
            return new Dictionary<string, object> {
                { "schemaVersion", 1 }, { "backend", backend.Id }, { "source", sample.source },
                { "sampleTime", DateTimeOffset.UtcNow.ToUnixTimeSeconds() }, { "sensors", sample.sensors },
                { "hardware", sample.hardware }, { "metrics", sample.metrics }, { "volumes", volumes },
                { "errors", sample.errors }, { "driverInstalled", sample.driverInstalled }, { "elevated", elevated }
            };
        }
        public void Dispose() { foreach (var extra in supplements) try { extra.Dispose(); } catch { } backend.Dispose(); }
    }
    public static class MonitoringSources
    {
        // Register reviewed adapters here. Configuration selects IDs, never executable paths or DLLs.
        public static readonly Dictionary<string, Func<IHardwareMonitorBackend>> Backends = new Dictionary<string, Func<IHardwareMonitorBackend>>(StringComparer.Ordinal) {
            { "librehardwaremonitor", () => new LibreHardwareBackend() }
        };
        public static readonly Dictionary<string, Func<IMonitoringSupplement>> Supplements = new Dictionary<string, Func<IMonitoringSupplement>>(StringComparer.Ordinal) {
            { "windows-storage", () => new WindowsStorageSupplement() }
        };
        public static MonitoringPipeline Create(string configurationPath)
        {
            string selected = "librehardwaremonitor"; var extras = new List<string> { "windows-storage" }; var errors = new List<string>();
            if (File.Exists(configurationPath)) try {
                var json = new JavaScriptSerializer().Deserialize<Dictionary<string, object>>(File.ReadAllText(configurationPath));
                object value;
                if (json.TryGetValue("backend", out value)) selected = Convert.ToString(value);
                if (json.TryGetValue("supplements", out value)) {
                    var items = value as System.Collections.IEnumerable;
                    if (items == null || value is string) throw new InvalidDataException("supplements must be an array");
                    extras.Clear(); foreach (var item in items) extras.Add(Convert.ToString(item));
                }
            } catch (Exception error) { errors.Add("Monitoring configuration: " + error.Message); selected = "librehardwaremonitor"; extras = new List<string> { "windows-storage" }; }
            Func<IHardwareMonitorBackend> factory;
            if (String.IsNullOrEmpty(selected) || !Backends.TryGetValue(selected, out factory)) { errors.Add("Unknown backend; using librehardwaremonitor: " + selected); factory = Backends["librehardwaremonitor"]; }
            var supplements = new List<IMonitoringSupplement>(); var seen = new HashSet<string>();
            foreach (var id in extras) { Func<IMonitoringSupplement> create; if (id != null && seen.Add(id) && Supplements.TryGetValue(id, out create)) supplements.Add(create()); else errors.Add("Unknown or duplicate supplement: " + id); }
            return new MonitoringPipeline(factory(), supplements, errors);
        }
    }
}
