using System;
using System.Collections.Generic;
using Hhub.Monitoring;

internal static class MonitoringTests
{
    sealed class Backend : IHardwareMonitorBackend
    {
        public string Id { get { return "replacement"; } }
        public bool Fail;
        public bool Disposed;
        public BackendSample Read() {
            if (Fail) throw new InvalidOperationException("backend offline");
            var sample = new BackendSample { source = "replacement" };
            sample.metrics["diskReadMB"] = new Metric(7, "MiB/s", Id);
            sample.metrics["cpuTemp"] = new Metric(null, "C", Id);
            return sample;
        }
        public void Dispose() { Disposed = true; }
    }
    sealed class Supplement : IMonitoringSupplement
    {
        public string Id { get { return "extra"; } }
        public bool Fail;
        public bool Disposed;
        public SupplementSample Read(BackendSample sample) {
            if (Fail) throw new InvalidOperationException("supplement offline");
            var result = new SupplementSample();
            result.metrics["diskReadMB"] = new Metric(99, "MiB/s", Id);
            result.metrics["cpuTemp"] = new Metric(50, "C", Id);
            result.metrics["cpuPower"] = new Metric(Double.NaN, "W", Id);
            return result;
        }
        public void Dispose() { Disposed = true; }
    }
    static void Ensure(bool condition, string description) { if (!condition) throw new Exception(description); Console.WriteLine("PASS " + description); }
    public static int Main()
    {
        var primary = new Backend(); var extra = new Supplement();
        using (var pipeline = new MonitoringPipeline(primary, new List<IMonitoringSupplement> { extra })) {
            var frame = pipeline.Read(); var metrics = (Dictionary<string, Metric>)frame["metrics"];
            Ensure((string)frame["backend"] == "replacement", "Replacement backend identity reaches transport unchanged");
            Ensure(metrics["diskReadMB"].value == 7, "Supplement cannot overwrite a valid primary reading");
            Ensure(metrics["cpuTemp"].value == 50 && metrics["cpuTemp"].source == "extra", "Missing reading is filled with explicit provenance");
            Ensure(!metrics.ContainsKey("cpuPower"), "Non-finite supplement readings are rejected");
            extra.Fail = true; frame = pipeline.Read(); metrics = (Dictionary<string, Metric>)frame["metrics"];
            Ensure(metrics["diskReadMB"].value == 7 && ((List<string>)frame["errors"]).Count == 1, "Supplement failure preserves valid primary data");
            extra.Fail = false; primary.Fail = true; frame = pipeline.Read(); metrics = (Dictionary<string, Metric>)frame["metrics"];
            Ensure(metrics["diskReadMB"].value == 99 && ((List<string>)frame["errors"]).Count == 1, "Backend failure still permits independent supplements");
            primary.Fail = false; frame = pipeline.Read(); metrics = (Dictionary<string, Metric>)frame["metrics"];
            Ensure(metrics["diskReadMB"].value == 7 && ((List<string>)frame["errors"]).Count == 0, "A later sample recovers without stale errors");
        }
        Ensure(primary.Disposed && extra.Disposed, "Pipeline releases every registered source");
        var zero = new Metric(0, "MiB/s", "test"); Ensure(zero.value == 0, "Idle disk rate is a valid zero");
        Ensure(!new Metric(-1, "W", "test").value.HasValue, "Negative readings remain unavailable");
        return 0;
    }
}
