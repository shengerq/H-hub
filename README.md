# H hub

当前桌面端与手机端项目源码。桌面端为 Windows 11 x64 原生 WinUI 3 / C++/WinRT 应用，尚未迁移为 WebUI。桌面版本元数据为 1.0.9，源码包含此版本之后的本机修复。

## 项目结构

- `source/`：桌面界面、独立核心、硬件监测、原生 HID、音频处理、场景与手机连接服务，以及测试和构建脚本。
- `app/assets/`：桌面产品资源；`app/phone-web/`：浏览器手机端页面；`app/sensors/`：监测配置与依赖许可。
- `mobile/android/`：独立 Android 手机 App，包括 Java 源码、Manifest、资源、构建和测试脚本。
- `installer/`：桌面安装器源码和可选驱动安装脚本，没有生成的安装包。
- `driver/`：驱动适配源码、许可和依赖说明；`signed-upstream/` 包含用于安装器的未修改 FxSound 已签名驱动输入。

## 当前行为

桌面 EQ 默认 10 段，设置页支持 1–12 段；10 段默认频率与调频范围依据 FxSound 原版。EQ 设置为独立页面。音频线程的设备扫描已移出实时处理路径，关闭桌面应用时核心和 EQ 一起停止并恢复路由。现有用户配置兼容保留。

手机端目前是 Android 0.4.0 原生项目；手机网页资源和桌面 HTTPS 配对服务也一并包含。手机端处理的是电脑音频配置，不处理手机本地音频。连接、安全、功能和实机测试范围见 [手机端说明](mobile/README.md)。未包含 iOS 项目或已验证的 iOS 安装包。

## 桌面构建

1. 安装 Visual Studio C++ Build Tools、Windows SDK 与 .NET Framework 开发组件。
2. 执行 `source/fetch-dependencies.ps1`，取得脚本中锁定并校验的 LibreHardwareMonitor 依赖。
3. 执行 `source/build.ps1`。NuGet 会恢复项目中锁定版本的 WinUI、C++/WinRT、WebView2 依赖。MSBuild 应使用 x64 工具架构。
4. 执行生成的 `app/Hhub.Core.exe --self-test report.txt` 运行核心自检。

第三方运行库和构建输出没有放入仓库。构建安装器还需要官方 PawnIO 安装程序，放入 `driver/pawnio/PawnIO_setup.exe`；下载与许可见该目录说明。自有驱动开发构建需要 WDK 和独立签名条件，不能把未签名开发输出作为正式驱动。

## Android 构建

Android 构建方法、锁定工具和第三方依赖见 [mobile/android](mobile/android/) 及 [手机端说明](mobile/README.md)。从源码运行 `fetch-dependencies.ps1`、`build.ps1`；APK 和开发签名密钥需在本地生成，不上传个人发布密钥。

## 许可与验证范围

保留各文件和第三方目录中的原始许可。FxSound 派生 EQ 为 AGPL-3.0-or-later，许可及改动记录见 `source/vendor/fxsound/`；LibreHardwareMonitor 为 MPL-2.0；PawnIO 和驱动遵循各自许可。根目录 LICENSE 为 FxSound 集成所需 AGPL 许可文本，各组件的原始声明继续有效。

核心自检与本机试听、硬件实测是不同的验证范围。EV63 等外设的协议与模拟测试不能替代全部真机验证，详见各模块说明。最近 EQ 修复已由用户实际试听确认。

本次仅整理源码，未发行新的安装包。没有沿用已删除发行版的下载链接或旧更新清单，也不包含个人配置、授权令牌、配对秘密、诊断录音和编译缓存。
