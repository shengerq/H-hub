# Hhub 1.0

Windows 11 x64 原生设备控制中心，使用 WinUI 3 / C++/WinRT。

## 下载

[下载安装包](https://github.com/shengerq/H-hub/releases/latest)。基础安装包只包含主体、运行时、系统与设备监测、场景配置和插件管理。

## 两个官方插件库

- [外设插件库](https://github.com/shengerq/H-hub-plugins)：鼠标、键盘等外设插件及开发测试工具。
- [应用插件库](https://github.com/shengerq/H-hub-app-plugins)：手机遥控、通用 EQ、Windows 管理，当前均为 **1.0**。

插件市场默认注册两个库，可在页面中直接切换；升级保留用户自定义源。

## 源码边界

主程序保留系统监测、设备监测、场景切换、插件市场及管理。应用插件通过通用接口提供页面和后台；宿主负责加载、监测和强制停止。手机、EQ、Windows 的页面、算法、预设、后台、手机客户端及音频驱动源码均位于应用插件库，不在本仓库中构建或打包。

`source/` 是主体与通用插件接口；`driver/pawnio/` 是原生硬件监测所需依赖；`installer/` 是基础安装器；`updates/stable.json` 提供正式安装包地址与 SHA-256。设备契约、Unicode 页面加载和可选虚拟设备测试仍保留。

## 构建

Windows 11 x64、Visual Studio 2022 C++ Build Tools 与 Windows SDK，使用 `source/build.ps1` 构建，再通过 `source/Stage-BaseRelease.ps1` 准备发行目录。NuGet 依赖按项目文件恢复。传感器依赖和官方 PawnIO 安装器由原有获取流程准备。

三个应用插件分别在应用库的 `plugins/phone`、`plugins/eq`、`plugins/windows` 内执行 `build.ps1`，不依赖本仓库源码。

## 版本统一

本次主体和三个应用插件统一为 1.0。旧版用户首次需要手动安装新的基础包；安装器保留已有配置与已安装插件，应用插件可从新的官方库更新。新更新清单使用发行代际区分本次版本重置。

许可证见 LICENSE；第三方许可证随实际组件保留。
