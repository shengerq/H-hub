H hub 1.1.0 整理安装目录，并新增简体中文和英文界面支持。

- 在设置中选择简体中文、英文或跟随系统；重新打开界面后生效。
- 应用语言包集中到 `languages`，界面运行库集中到 `runtime`，减少安装目录中的散落文件。
- 基础安装包不包含通用 EQ、Windows 设置、手机遥控三个官方插件；升级保留已经安装的插件、设置和配对资料。
- 保留托盘外设电量提示，并支持中英文托盘菜单。
- 保留最小窗口限制和主题切换时的原位更新，改善设置页留白。

适用于 Windows 11 x64。插件名称、用户配置和第三方插件页面使用其原有文字。


## Build

The `stable/1.1` branch contains the source for the 1.1 release. Development on `main` is retained separately.

On a Windows 11 x64 build host with Visual Studio C++ Build Tools, Windows SDK and Inno Setup:

```powershell
powershell -ExecutionPolicy Bypass -File source/build.ps1
powershell -ExecutionPolicy Bypass -File source/Stage-BaseRelease.ps1
ISCC.exe installer/Hhub.iss
```

`Hhub.exe` at the installation root preserves old shortcuts and exits after starting the interface. New shortcuts and the tray start `runtime/Hhub.exe` directly. Core services remain in the installation root. `languages/zh-CN.json` and `languages/en-US.json` contain the application text; the framework keeps its native MUI resources next to its DLLs under `runtime`.

The upgrade helper preserves installed optional components and their metadata, settings, EQ profiles and phone pairings. It removes only the old runtime files explicitly enumerated in the installer and only empty old language directories.
