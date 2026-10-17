# 硬件监测接口

默认硬件后端为未修改的官方 LibreHardwareMonitorLib 0.9.6。没有加入厂商私有接口。

## 结构

- `HardwareMonitoring.cs`：后端与补充源契约、Libre 适配器、Windows 磁盘补充源、合并和来源注册。
- `Sensors.cs`：按需采样进程，只处理命令、JSON 帧、父进程验证与退出。
- `SensorTransport.h`：宿主与采集进程通信、响应超时、普通与授权采集生命周期。
- `MonitorProjection.h`：把原始读数和标准指标转换为界面数据；校验单位和数值范围。
- `Monitor.h`：采样调度、缓存、设备身份和自动发起授权策略。

## 选择数据源

编辑 `app/sensors/monitor-sources.json`，完全退出应用的后台核心后重新启动：

```json
{
  "backend": "librehardwaremonitor",
  "supplements": ["windows-storage"]
}
```

将 `supplements` 改为 `[]` 可只使用 Libre。默认配置只在 Libre 缺失对应磁盘读数时查询 Windows。未知后端会回退到 Libre，并在读取提示中保留配置错误；未知补充源会跳过并记录错误。配置不能直接加载任意 DLL 或可执行文件。

## 替换后端或补充数据

实现 `IHardwareMonitorBackend`，通过 `Read()` 返回 `BackendSample`，并在 `MonitoringSources.Backends` 注册构造函数。实现 `IMonitoringSupplement`，通过 `Read(primary)` 返回 `SupplementSample`，并在 `MonitoringSources.Supplements` 注册。新接口只需重新编译采集进程，不要求修改页面或命名管道协议。

每项 `Metric` 包含 `value`、`unit`、`source`、`status`。原始传感器保留类别、类型、设备标识和来源；替换后端也可直接提供标准指标，不必模拟 Libre 的传感器名称。

| 标准指标 | 单位 |
| --- | --- |
| cpu、gpu、memory | `%` |
| cpuTemp、gpuTemp | `C` |
| cpuPower、gpuPower | `W` |
| cpuClock、gpuClock | `MHz` |
| memoryUsedGB、memoryTotalGB、gpuUsedGB、gpuTotalGB | `GiB` |
| diskUsedGB、diskTotalGB | `GB`，十进制，兼容 Libre 存储读数 |
| diskReadMB、diskWriteMB | `MiB/s` |

补充源按配置顺序填充缺失项，不覆盖主后端的有效值。空值表示未提供；NaN、无穷和负值拒绝作为有效指标。磁盘空闲时的 0 速率是有效值。来源失败时保留其他来源的读数并输出错误，下次请求会重新尝试。不要把估算值、电池功耗、功耗限制或热区温度登记为 CPU 实测封装功耗或核心温度。

Windows 磁盘容量是已挂载固定卷的合计，读写速率是物理磁盘 `_Total` 性能统计。它们与 Libre 的物理磁盘列表范围可能不同，因此回退页面显示“Windows 磁盘统计”，并单列卷容量；不虚构物理磁盘型号或逐盘速率。

## 权限

主界面保持普通权限。构建默认下载并校验官方 PawnIO 2.2.0，安装器默认选中驱动安装。驱动就绪且传感器开始响应后，每次核心会话最多自动发起一次只读采集进程的 Windows 原生授权，包括虚拟机。取消后继续普通采集且不循环弹窗，可通过首页“授权采集”手动重试。缺少驱动时显示“需驱动”。

## 验证

运行 `test-monitoring-sources.ps1` 验证替换后端、优先级、失败恢复、无效值和释放资源。运行后台核心的 `--self-test <报告路径>` 验证宿主数据转换、单位检查和权限策略。`probe-sensors.ps1` 可读取两帧真实数据，不会写硬件设置。
