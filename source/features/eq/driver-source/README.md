# Hhub Audio：FxSound 驱动修改版

当前发布方案已回到原版 FxSound 签名驱动，见[共享驱动规则](../SHARED-DRIVER.md)。
本目录仅保留实验源码及构建记录，不用于当前 EQ 插件分发包，也不继续作为送签发布方案。

版本：**1.0.0.0**。基于 [FxSound 驱动源码](https://github.com/fxsound2/fxsound-driver)
提交 `c78fc6d031d16bd0a5dbbdff4871cfb8715d343d`，保留上游 AGPL、Microsoft
Limited Public License 和源码版权声明。`fxvad/` 保留原源码目录名便于追踪上游。

底层音频传输仍沿用 FxSound 的实现；Hhub 修改设备、服务、文件名和接口标识，
是为了避免与原 FxSound 驱动发生身份冲突。独立命名不改变上游来源或适用许可。

本目录是该修改版的**开发源码**。SYS、INF、CAT 已完整构建，WDK 可签名性
检查通过。当前分发的 EQ 1.0 插件仍使用原签名 FXVAD 包，尚未切换到新驱动。
微软正式签名和与 FxSound 同机播放验证尚未完成，不能据此宣称当前插件已解决共存问题。

`DRIVER-NOTICE.txt` 说明独立命名是为了避免安装、更新和卸载时与原驱动冲突，
同时保留原许可、版权、归属和适用的源码提供要求。它与两份原许可证一起
嵌入 SYS 资源，并随构建包分发；SYS 文件属性的“备注”也标明了来源和修改原因。

## 独立身份

权威清单为 `driver-identity.json`，构建前自动检查源码与清单一致。

| 项目 | Hhub 独立值 |
| --- | --- |
| 显示名称 | `Hhub Audio` |
| 硬件 ID | `ROOT\HHUBVAD` |
| 驱动服务 | `HHUBVAD` |
| 驱动文件 | `hhubvad.sys` |
| 安装声明 / 目录文件 | `hhubvad.inf` / `hhubvadntamd64.cat` |
| 波形 / 拓扑引用名 | `HhubWave` / `HhubTopology` |
| 产品 GUID | `{8B6F317D-63E4-4E91-9D80-FA4D37A9725B}` |
| 私有媒体分类 GUID | `{5A03CC71-9C61-4C93-B603-AD739D82B10E}` |

内核 `PcRegisterSubDevice` 使用的引用名与 INF `AddInterface` 保持一致。
Windows 标准音频分类、扬声器类型和系统代理 CLSID 保持系统规定的值。
资源中的产品名、原始文件名、版本和图标均使用 Hhub，保留 FxSound 版权归属。
音频传输实现继续复用上游。

## 校验与构建

使用 Windows PowerShell 或 PowerShell，在本目录运行：

```powershell
.\Test-DriverIdentity.ps1
.\Test-DriverIdentity.Tests.ps1
.\Build-Driver.ps1 -CheckOnly
.\Build-Driver.ps1
```

构建目标为 Windows 10/11 x64，需要匹配的 **SDK/WDK 10.0.26100.0**、Visual
Studio 2022 C++ Build Tools、Spectre 库及 `WindowsKernelModeDriver10.0` 工具集。
安装参考[微软 WDK 文档](https://learn.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk)。
仅安装用户态 Windows SDK 不足以构建内核驱动。脚本自动发现 64 位 MSBuild，
也可通过 `-MSBuild`、`-KitVersion`、`-OutputRoot` 指定路径和版本。

每次构建使用 `build/<时间戳-随机编号>/` 的全新目录，并要求同时生成 Hhub 的
SYS、INF、CAT 和 `build-record.json`。旧的 `x64/Release` 缓存不作为有效产物。
`SignMode=Off` 仅构建未签名文件，不修改系统签名策略或安装设备。

2026-10-19 本机验证：身份校验和 7 个错误注入检查通过；完整 x64 SYS 编译、
WDK 可签名性检查和 CAT 生成通过（无可签名性错误或警告）。上游 `stdunk.h`
会打印其内核 new/delete API 将来移除的编译提示，当前构建成功。

本机使用工作区工具链，不修改系统 WDK 安装。也支持：

```powershell
.\Build-Driver.ps1 -ToolchainRoot '<已解压的工具链目录>'
```

该目录需要 `microsoft.windows.wdk.x64` 和 `microsoft.windows.wdk.x86`
（官方 NuGet，版本 10.0.26100.2454），`wdk-build`（微软 Visual Studio
`Microsoft.VisualStudio.WindowsDriverKit.Build` 17.14.36705.7 的解压目录）和
`spectre`（微软 VC 14.44 x64 Spectre 库解压目录）。脚本把系统 VS 2022 的
v170 构建规则复制到该目录的 `vc/`，叠加官方 WDK 构建规则后使用；原系统文件不修改。

## 签名前准备

构建成功后，将脚本打印的包目录传入：

```powershell
.\Prepare-Signing.ps1 -Package '<新构建的包目录>'
```

脚本校验构建记录哈希、INF 标识和 SYS 版本资源及来源备注，生成
`build/signing/Hhub-Audio-Unsigned.cab`。它只是签名前的提交材料，仍需按微软要求
进行组织签名与提交。修改 INF、重命名原 SYS 或借用旧 CAT 不能继承原签名。
参见[目录签名说明](https://learn.microsoft.com/en-us/windows-hardware/drivers/install/catalog-files)。

二进制开发包应同时提供匹配的源码归档。构建包和 CAB 均携带 `DRIVER-NOTICE.txt`、
`LICENSE` 和 `license.rtf`，本说明不取代许可证全文。

`signed-upstream/` 和 `../driver/payload/` 保留原签名文件；Git 属性禁止转换它们的
换行符，`shipping-payload-sha256.json` 用于检查现有分发载荷未被改写。

## 接入 EQ 插件的后续条件

取得匹配的微软签名包后，EQ 插件的安装、发现、启停与卸载必须只操作
`ROOT\HHUBVAD` / `HHUBVAD`。不能依赖可变的显示名称认领 FxSound 设备，也不能
自动重命名、更新、禁用或卸载 `ROOT\FXVAD`。需要替换插件内的安装资源，完成
FxSound 先启动、后启动、关闭及卸载时的同机音频测试，再发布更新后的插件。
这些改动属于 EQ 插件，主程序不需要增加音频驱动实现。
