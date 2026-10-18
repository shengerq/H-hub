# Hhub 插件宿主协议 1.2（主程序 1.4.0）

系统监测、设备监测、场景切换、插件管理、主页面、顶栏和侧边栏属于主程序。插件自己的页面、预设及业务算法属于插件。插件页面显示在主窗口顶栏和侧边栏之外的整个内容区域。辅助进程不弹出单独窗口。

## 新增插件

只构建并导入 `.hhub` 包，不修改或重新编译主程序。组件名称和 SHA-256 从用户批准安装的包清单读取。摘要用于完整性验证，不等于作者签名。更新时在插件管理中审阅新的清单。

```json
{
  "schemaVersion": 1,
  "id": "example.audio-tool",
  "name": "音频工具",
  "author": "Example",
  "version": "1.0.0",
  "apiVersion": "1.0",
  "permissions": [],
  "requiredHost": {
    "id": "hhub",
    "apiVersion": "1.2",
    "minimumVersion": "1.4.0",
    "execution": "host-only",
    "standalone": false
  },
  "implementation": {
    "type": "native-module",
    "abiVersion": 1,
    "library": "AudioTool.dll"
  },
  "navigation": { "title": "音频工具", "glyph": "" },
  "capabilities": ["status", "applyPreset"],
  "nativeAssets": [
    { "path": "AudioTool.dll", "encoding": "base64", "sha256": "文件摘要", "data": "文件内容" }
  ]
}
```

插件 ID 使用小写字母、数字、点、下划线和连字符。组件路径必须是插件目录内的相对路径。安装事务验证全部内容后替换组件目录；安装后的清单仅保存路径、摘要和字节数，不保留 Base64。入口 DLL 必须在组件列表中。包上限为 8 MiB，单组件上限为 4 MiB。

## 页面与调用

导出函数原型见 `source/PluginApi.h`。ABI 1 兼容现有 EQ、Windows 页面模块。插件可以提供原生子窗口，或通过可选的 `HhubPluginCreateXamlPage` 在辅助进程自己的 WinUI Border 内创建页面。宿主将辅助进程窗口嵌入主窗口内容区，统一处理尺寸、侧栏覆盖和宿主提示条遮挡。

初始 JSON 包含 `state`、`dark` 和兼容的应用/输出绑定。`state` 是快照；用户设置由插件通过宿主通用配置和调用服务保存，不能靠修改快照来保存。插件自行创建标题、控件、滚动区域、设置对话框及预设选择器。

`HhubPluginRequest` 接收 UTF-16 JSON，返回包含 `ok` 和业务数据的 JSON。失败返回 `ok:false,error:"…"`。内存由插件使用 `CoTaskMemAlloc` 分配，宿主释放。页面在其 UI 线程创建和销毁。

宿主 IPC 通用请求：

```json
{"op":"pluginInvoke","body":{"id":"example.audio-tool","operation":"status","body":{}}}
```

`operation` 必须在插件 `capabilities` 中声明。结果位于返回状态的 `pluginResult`。旧版场景、手机和 EQ 请求保留兼容适配；新增插件不需要添加这样的专用分支。

通用配置使用 `{"op":"pluginData","body":{"id":"example.audio-tool","value":{"preset":"my-preset"}}}`。传入 `value` 时写入，不传时读取；配置位于返回状态的 `pluginData[id]`，更新插件时保留，卸载时移除。单个配置上限为 512 Ki 个 UTF-16 字符。

## 运行与停止

每个原生插件的页面和后台请求在对应的 `Hhub.PluginHost.exe` 辅助进程中运行。主程序自身不映射插件 DLL。后台请求进程在首次调用时启动；页面进程在打开页面时启动，离开页面后释放。辅助进程及普通子进程进入以插件 ID 区分的 Windows Job。

主程序报告进程数、异常和强制停止状态。请求超时会关闭出错的辅助进程，主程序保留响应。插件管理的“强制停止”结束该插件 Job 中的进程，并阻止后台策略立即重新启动。用户再次“打开”时恢复。关闭主程序和核心后，Job 的最后一个句柄关闭，普通辅助进程自动清理。

```json
{"op":"pluginRuntime","body":{}}
{"op":"pluginForceStop","body":{"id":"example.audio-tool"}}
{"op":"pluginStart","body":{"id":"example.audio-tool"}}
```

可选 `lifecycle.uninstall` 声明卸载清理操作。清理成功后停止进程并删除插件组件；系统已经应用的设置是否回退由插件自身策略决定。Windows 插件沿用原有卸载行为。

这是进程故障隔离，不是权限沙箱。需要提升权限的驱动安装和系统操作仍沿用对应插件的显式授权流程。

## 验收

独立测试插件使用宿主源码中不存在的 ID `example.third-party.synth` 和不同作者。验证在主程序文件哈希保持不变的情况下安装、调用、更新和卸载；另外验证崩溃、超时、强制停止、摘要篡改拒绝、原版页面内嵌、尺寸变化和导航恢复。测试插件和私有测试配置不进入正式安装包。

## API 1.3：独立后台与原生功能调用

手机遥控及其他应用插件通过同一个 `pluginInvoke` 调用声明的能力。宿主向插件提供当前状态；插件可返回 `hostRequests`，宿主在能力调用完成后，逐项验证 `hostOperations` 声明及原生操作白名单后执行。此设计避免后台回调宿主产生同步等待死锁。

`lifecycle.startup/shutdown/uninstall` 分别提供启动恢复、退出和卸载清理。监测和强制停止均按插件 ID 和共享 Job 实施，后台子进程继承该 Job。宿主不需要识别手机协议或后台文件名。

通用设备任务使用 `pluginTaskSubmit/Validate/Ack`。任务提供方负责授权、参数范围及回读检查；宿主仅负责分发给已有设备插件。`workerExtensions` 从已校验的包内资产加载页面任务扩展，插件更新不需要修改或重新编译主程序。


## 1.0 发行代际

本次应用与三个内置应用插件统一为 1.0；原生接口版本仍为 1.3。新插件应在顶层和 `requiredHost` 中声明 `releaseEpoch: 1`，并使用 `minimumVersion: "1.0"`。新代际按新版本号比较；未声明代际的旧插件保留历史宿主最高 1.5.0 的兼容范围，并继续验证接口、ABI 和组件摘要。仅改动显示版本不会使已支持的第三方插件失效。
