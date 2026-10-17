# 原生插件安装与远程库回归检查

原生插件由 GitHub 仓库 shengerq/H-hub-plugins 发布。生产构建不复制 native-plugin-library；正常下载缓存与已安装设置仍保存在用户目录。

测试前设置 HHUB_NATIVE_TEST_PACKAGES 为该插件仓库的绝对检出路径，提供 EQ、Phone Remote 和 Windows Settings 的 .hhub 包。然后运行 Hhub.Core.exe --self-test <报告绝对路径>。测试不依赖用户安装目录中的本地插件库。

UI 检查使用独立 LOCALAPPDATA、HHUB_TEST_PROFILE=ui-memory-tests：
- HHUB_NATIVE_PLUGIN_UI_TEST=1：源迁移、自定义源保留和安装卸载导航检查。
- HHUB_NATIVE_INSTALL_MEMORY_TEST=1：四轮逐个安装、卸载，输出 native-install-memory.csv。停止后台轮询，测试真实 Model 导入及插件管理卡片，不启动设备服务。

新的原生插件安装不自动设置 eqEnabled 或 phoneEnabled；升级保留已有启用偏好。首次手机插件安装没有残留数据时，不启动清理辅助进程。市场下载仅更新当前按钮，完成后更新插件卡片状态，不重建整页。
