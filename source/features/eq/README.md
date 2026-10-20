# Hhub 内置 EQ

本模块包含 EQ 页面、曲线控件、音效、DSP、工厂预设、配置迁移和音频驱动安装器。随 Hhub 1.0 集成版提供，主程序仍通过隔离组件承载界面和后台。

完整安装包包含 Hhub.EQ.dll、HhubAudioDriverSetup.exe 与官方签名驱动资源。默认勾选安装/修复 EQ 驱动，并命名为 H hub Audio。已有正常 FXVAD 驱动时复用；驱动文件与硬件身份不改动，显示名称在安装后设置并校验。详见 SHARED-DRIVER.md。

构建使用 Visual Studio C++ Build Tools 与 Windows SDK，运行 build.ps1。模块或驱动安装器改变后，需要通过主工程 Build-BuiltinFeatures.ps1 更新组件摘要，并重新编译主程序。

本轮验证不安装驱动、不改变现有音频路由。真实耳机播放及重启后的端点显示仍需在实际安装时确认。