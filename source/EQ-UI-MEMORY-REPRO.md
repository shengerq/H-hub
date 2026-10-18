# EQ 安装与页面内存复现、修复记录

2026-10-15。目标口径是 **UI 主进程的私有工作集**，不包含 Core、Phone、Sensors、共享 DLL 和显卡显存。下列 MB 均按 1024² 字节计算。

## 最终发布构建实测

使用完整状态副本、相同远程插件包及真实下载、摘要/身份校验和安装确认。窗口可见；冷启动 EQ 测试每秒触发实际绘制，编辑器实际尺寸为 1126 × 640 DIP。隔离模型不连接用户 Core，不安装驱动、不改变音频路由。预览导出与内存测试分开执行。

| 阶段 | 私有工作集 MB | 私有提交 MB |
| --- | ---: | ---: |
| 市场页，安装前 | 27.43 | 37.86 |
| 安装后尚未打开 EQ | 29.50 | 39.68 |
| 首次打开 EQ | 29.36 | 39.21 |
| 首次离开 EQ | 30.56 | 40.45 |
| 十轮打开/离开后等待 5 秒 | 23.46 | 41.16 |
| 单独冷启动 EQ，持续绘制后 | 25.63 | 32.50 |
| 冷启动 EQ 后转插件管理页 | 31.02 | 39.33 |

完整安装/切页采样范围 **23.46–37.06 MB**，不是所有页面、所有时刻都锁定在 25 MB。前一轮同等测试冷启动 EQ 为 26.22 MB，十轮后市场页为 25.53 MB。系统驻留页回收会影响私有工作集，因此不能只选最低读数判断泄漏；同时保留私有提交、对象数量、窗口存活及每次采样数据。

同样十轮流程的私有提交：原始版本最终 **64.58 MB**，仅删除隐藏控件的中间版本 **58.61 MB**，当前最终构建 **41.16 MB**，相对原始版本减少约 **36%**。旧版本 CSV 没有私有工作集列，不将旧提交值与新工作集值直接比较。

## 改动

- EQ 编辑器改为按页面创建的原生子窗口、五个滑块、两个选择框及有尺寸上限的双缓冲绘图。离页立即销毁 HWND、位图、DC 和字体。曲线、频率旋钮、五项 FxSound 效果、预设与输出选择保留；参数依旧通过原有 Core 接口预览、提交和回读。
- 12 段增益、频率、效果、前级及保护开关使用数值状态；删除用作状态存储的隐藏控件及已失效的旧绘图片段。未保存草稿只保存数值，不保存页面控件。
- 平面侧栏只构造当前可见的按钮行，保留 56/216 DIP 宽度、窄窗覆盖模式、设置入口、插件安装后显示和卸载后移除。删除旧 NavigationView 模板树遍历和无效事件。
- 市场搜索使用可释放的原生输入框；安装确认改为系统 TaskDialog；通知改为轻量 XAML 视图。保留权限、作者、来源和摘要检查，安装仍需用户点击确认。确认框的定时自动点击只在隔离复现模式中启用。
- 对原生控件实现 DPI 布局、滚动裁剪、弹窗隐藏和侧栏遮挡处理。EQ 调节期间禁止页面纵向滚动。
- 未修改 DSP、音频缓冲、Core 或虚拟驱动；未清空工作集、降低内存优先级、强制卸载 COM 模块或增加隐藏渲染进程。试验过的堆整理没有明显收益，未保留。

## 验证与边界

- Release 构建成功，无编译警告。
- 最终构建 25 项 UI 检查全部通过：安装/卸载导航、实际按钮 Invoke、Unicode 搜索、输入长度与键盘切焦、曲线拖动、效果滑块、预设/输出回调、参数/草稿回读、离页窗口及 GDI/USER 资源释放。
- 十轮返回市场页后，旧页面、EQ 宿主和 EQ 原生子窗口存活数全部为 0；事件数在对应页面保持固定。GDI/USER 数量未随轮次持续增长。该证据支持已修复的页面资源路径，不等于证明整个程序绝无任何泄漏。
- 标题栏模板高度、图标中心与窄侧栏对齐检查通过；浅深色场景按钮、打开中弹窗主题切换检查通过。另行导出并审查 EQ 和外壳图像。
- 测试未启动实际驱动音频流。约 25 MB 已在冷启动可见 EQ 达到；复杂页面与切页短时峰值仍更高，不能承诺全场景 25 MB 上限。

## 已部署版本复核

已替换 Hhub-grouped/Hhub.exe 并打开真实 EQ 页面，UI PID 5692，Core PID 15176 保持不变。对实际进程连续五次只读采样，EQ 原生窗口均可见、进程正常响应：私有工作集 **24.953 MB**，私有提交 **32.207 MB**。这是当前页面的驻留值，不能当作所有操作的峰值上限；总工作集另为 111.77 MB，包含共享页，不能与本次约 25 MB 的目标混用。

部署前后插件、settings、eqTemplate、eqRealPrefs 保持一致；Core 文件未更改，部署文件与已验证构建的 SHA-256 一致。源码、旧 UI 与原始配置备份在 eq-native-memory-release-backup；发布验证见 eq-native-memory-deployment.json、eq-deployed-ui-memory.json。

## 测试证据文件

最终构建：eq-release-install-memory-data/HhubUiMemoryTests/eq-install-repro.csv、eq-release-cold-memory-data/HhubUiMemoryTests/eq-install-repro.csv、eq-release-functional-data/HhubUiMemoryTests/native-plugin-ui-check.json。

复核轮：eq-lean-install-memory-data/HhubUiMemoryTests/eq-install-repro.csv、eq-lean-cold-memory-data/HhubUiMemoryTests/eq-install-repro.csv。主题/标题栏：eq-lean-theme-data/HhubUiMemoryTests/theme-check.txt、eq-lean-chrome-data/HhubUiMemoryTests/chrome-check.txt。独立视觉检查：eq-lean-visual-data/HhubUiMemoryTests/native-eq-preview.png 与 shell-preview.png；XAML 截图不包含原生子窗口，所以两张分别检查。

## 重现

使用独立 LOCALAPPDATA，设置 HHUB_TEST_PROFILE=ui-memory-tests、HHUB_EQ_INSTALL_REPRO=1、HHUB_EQ_INSTALL_REPRO_STATE=<状态副本绝对路径>、HHUB_EQ_INSTALL_REPRO_FEED=<含完整 HTTPS packageUrl 的清单路径>，启动 Hhub.exe --layout-page=market。安装复现会实际下载并验证远程包，然后在隔离模型中完成确认及十轮切页。

设置 HHUB_EQ_COLD_VIEW=1 并以 --layout-page=eq 启动可测冷启动；内存测量时应关闭 HHUB_EQ_NATIVE_PREVIEW。所有复现模型与定时确认均要求 UI 隔离测试配置，普通运行不启用。状态副本可能含个人配置，不应上传。

