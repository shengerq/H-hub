# H hub 桌面端源码（source/）

本目录是 H hub 的 Windows 桌面端源码，采用 **C++/WinRT + WinUI 3** 开发，
目标平台 **Windows 11 x64**。项目分多个可执行模块，通过本机 IPC 协作。

## 模块结构

| 文件 | 构建方式 | 产物 | 作用 |
| --- | --- | --- | --- |
| App.cpp + 各 *.inc / *.h | MSBuild（Hhub.vcxproj） | Hhub.exe | 主界面（WinUI 3） |
| Core.cpp | MSBuild（Hhub.Core.vcxproj） | Hhub.Core.exe | 后台核心服务（配置、插件、HID、音频路由） |
| WindowsWorker.cpp | MSBuild（Hhub.Windows.vcxproj） | Hhub.Windows.exe | Windows 设置相关工作进程 |
| AudioTest.cpp | MSBuild（Hhub.AudioTest.vcxproj） | Hhub.AudioTest.exe | 音频自检工具（不作为发布产物） |
| Sensors.cs + HardwareMonitoring.cs | csc.exe | Hhub.Sensors.exe | 硬件监测（基于 LibreHardwareMonitor） |
| PhoneServer.cs | csc.exe | Hhub.Phone.exe | 手机端局域网服务 |
| UpdateService.cs | csc.exe | Hhub.Update.exe | 更新检查 |
| PluginCatalog.cs | csc.exe | Hhub.Catalog.exe | 插件目录 |
| vendor/qrcodegen | 随 Hhub.vcxproj 编译 | - | 二维码生成 |
| vendor/fxsound | 随音频模块引用 | - | FxSound 派生 EQ 内核（AGPL-3.0-or-later） |

## 构建环境要求

- Windows 11 x64
- **Visual Studio C++ Build Tools**（v143 工具集，含 MSBuild）
- **Windows SDK**（10.0）
- **.NET Framework 4.x 开发组件**（提供 csc.exe，用于编译 C# 服务）
- PowerShell 5.1 及以上

## 构建步骤

在仓库根目录下执行：

    # 1. 拉取第三方依赖（下载 LibreHardwareMonitor 0.9.6 到 app/sensors，并校验 SHA-256）
    powershell -NoProfile -File source\fetch-dependencies.ps1

    # 2. 构建（默认 Release，输出到 ..\app\）
    powershell -NoProfile -File source\build.ps1

    # 或指定自定义输出目录：
    powershell -NoProfile -File source\build.ps1 -Configuration Release -OutputDirectory D:\hhub-out

build.ps1 依次完成：

1. 把 app/ 复制到输出目录（若指定了 OutputDirectory）；
2. 用 csc.exe 编译 4 个 C# 服务到 app/；
3. 用 MSBuild 依次构建 Hhub.Core.vcxproj、Hhub.AudioTest.vcxproj、Hhub.Windows.vcxproj、Hhub.vcxproj；
4. 复制 VC143 运行时 DLL 到输出目录。

## 输出产物（默认输出到仓库根的 app/）

- Hhub.exe（主界面）
- Hhub.Core.exe（后台核心）
- Hhub.Windows.exe
- Hhub.Sensors.exe
- Hhub.Phone.exe
- Hhub.Update.exe
- Hhub.Catalog.exe
- 以及 assets/、phone-web/、sensors/ 等运行时资源

## 构建依赖版本（来自 *.vcxproj）

| 依赖 | 版本 |
| --- | --- |
| Microsoft.WindowsAppSDK | 1.8.260921001 |
| Microsoft.Windows.CppWinRT | 3.0.260818.1 |
| Microsoft.Web.WebView2 | 1.0.3179.45 |
| LibreHardwareMonitor | 0.9.6（由 fetch-dependencies.ps1 下载） |

## 解决方案文件

Hhub.sln 串联 4 个 vcxproj（Hhub / Hhub.Core / Hhub.Windows / Hhub.AudioTest），
统一为 Release|x64 配置。可直接用 Visual Studio 打开，或交给 build.ps1 构建。

## 已知缺失

发布/安装环节还依赖若干不在仓库中的文件（PawnIO 安装器、更新清单、发布说明等），
清单见仓库根目录的 MISSING-ASSETS.md。

## 许可

本目录包含多份不同许可的代码与第三方组件：

- vendor/fxsound/：AGPL-3.0-or-later，许可文本见 vendor/fxsound/LICENSE-AGPL-3.0.txt
- vendor/qrcodegen：见其源文件头部声明
- 其余第三方声明见仓库根 LICENSE 与各目录说明
