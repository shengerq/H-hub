# 原生首页监测重构

首页入口使用 WinUI 3 / C++/WinRT，控件实现在 NativeHomePage.inc。
原有 HomeWebPage.inc 不再参与首页编译；插件的 WebView2 宿主独立保留。

## 数据链路

核心的 monitor、open、snapshot、deviceScan、deviceSnapshot 和 monitorElevate 请求
通过 HomeMonitoring::build 返回 version=1 的 homeMonitoring 快照。
快照包含 fields、devices、采集状态和时间戳；原始 monitor/detected 接口继续兼容其他页面及手机端。
底层 LibreHardwareMonitor、Windows 补充采集和 HID 插件协议保持现有实现。

字段区分 available、initializing、stale、needsDriver、needsElevation 和 unavailable。
过期（5 秒）、非有限数和越界百分比不作为实时读数展示；0% 和 0 MB/s 是有效读数。
设备识别、容器合并、插件匹配和过滤继续采用原 DeviceReader / PluginTelemetry / PluginHidV2 算法。电量保留原采集器结果与低电量阈值；连接状态未知不隐藏有效读数，明确断开的无效电量继续剔除，保留 XInput 电量等级。

## 界面与刷新

系统卡片在宽、中、窄窗口使用 4、2、1 列布局，设备区域在窄窗口上下排列。
CPU、GPU、内存显示最近一分钟趋势；无效采样和大于 5 秒的间隔断开曲线。
定时更新现有控件，设备拓扑变化时重建页面；选择设备仅更新现有英雄卡片与列表选中状态，保留列表滚动位置。
异步请求记录页面代次，返回后不修改已经切换的页面控件。
核心连接失败时清空系统读数并显示等待更新，设备列表保留最近一次结果。

## PawnIO 与授权

Fetch-PawnIO.ps1 默认准备官方 PawnIO 2.2.0 安装包，校验固定 SHA256 和 namazso 发布者签名。
build.ps1 与 fetch-dependencies.ps1 均调用该脚本；已验证的包直接复用。
安装器默认选中 PawnIO，使用官方 -install -silent 参数；已有驱动则跳过。
驱动就绪后，每次核心会话最多自动请求一次 Windows 原生 UAC 授权，仅提升传感器采集进程。
虚拟机同样使用默认授权流程；取消后继续普通采集，首页授权按钮支持手动重试。

## 验证状态

2026-10-14 安装 Visual Studio 2022 Build Tools、Windows 11 SDK 10.0.26100 和 WinUI 构建组件。
Release x64 全部构建通过，93 项核心自检通过，现有采集器的 10 项契约测试通过。
程序已启动并通过原生窗口截图和辅助功能树检查：首页、实时趋势、空设备状态、监测详情入口。
官方 PawnIO 2.2.0 安装成功；实时 IPC 确认 pawnInstalled=true、pawnRunning=true、sensorElevated=true、homeStatus=live。
当前虚拟机没有外设直通，设备选择及电量尚需实际外设验证；CPU 温度和 GPU 指标取决于虚拟机回传能力。
构建日志、核心自检和实时状态报告保存在桌面新建文件夹中。
设备区按旧首页恢复 64:36 设备大图区 / 缩略图列表、连接状态、电量、配置、设备数量和当前场景栏，使用原生 WinUI 控件。设备缩略图及主图优先使用原插件图片，缺失时继续使用原资产示意图。

设备区按剩余视口高度布局，最大化窗口下已检查空设备提示和当前场景栏完整显示。

## 共享顶栏与跨显示器布局

所有页面共用 32 DIP 标题栏、12 DIP 页面顶部内边距，页面标题行最小高度为 40 DIP。
NavigationView 位于独立标题栏下方，关闭额外的 IsTitleBarAutoPaddingEnabled。
保留 PerMonitorV2 DPI 模式；初始窗口根据所在显示器 rcWork 和 DPI 计算尺寸并居中，避开任务栏。
首页采用原 208px 最小卡片高度和 20px 区块间距，同时保留新文字、主要读数和迷你趋势曲线。
构建通过，首页与 EQ 页面已通过截图检查；多页面布局日志的按钮裁切数为 0。
当前验证来自这台虚拟机，未声称已经在不同物理电脑和不同 DPI 显示器上完成实机验证。
## 监测入口与场景设备卡片
首页“系统监测”标题改为按钮，点击后加载监测内容；侧栏不单列系统监测入口。
取消首页“监测详情”和“授权采集”按钮，保留核心自动启动采集及自动请求权限流程；移除监测内容中的 Windows 设置快捷按钮。
场景设备卡片“新建配置”移至标题行右侧，修正配置网格与控件最小宽度冲突；长配置名称使用省略号，悬停显示全名。
Release x64 构建通过。用户要求仅允许页面截图，因此最终版本未通过自动操作窗口验证，未重启当前程序。
供用户手动检查的可执行文件为 app/Hhub.preview.exe，使用 app 目录中现有依赖。

## Windows 应用身份与进程分组
主程序、Core、Windows worker、Phone、Update、Catalog 和 Sensors 的嵌入 manifest 统一声明 Hhub.Desktop / Hhub 应用身份。
使用 external location 身份包，build.ps1 自动生成未签名身份包；普通构建不创建证书，不改信任库。
完整 Release x64 构建及 7 个嵌入身份静态检查通过，产物位于桌面新建文件夹/Hhub-grouped。
注册需为本地开发签名建立当前用户 TrustedPeople 信任。自动审批拒绝未明确授权的持久信任变更，注册尚未执行，正在等待用户授权。
尚未通过运行进程身份及任务管理器截图确认分组效果，不将静态身份检查报告为实际分组已完成。资源占用没有改变。
用户已授权当前用户 H hub 开发签名信任；证书及签名完成。Windows 身份注册返回 0x800B0109，当前用户 TrustedPeople 不足以满足此机部署信任验证。本机 TrustedPeople 尚未修改，等待单独授权。
用户另行授权同一张 H hub 证书加入本机 TrustedPeople 后，统一身份包注册成功：Hhub.Desktop_9hyarznvzybzr，Status=Ok。当前产物及注册外部路径为桌面新建文件夹/Hhub-grouped。未自动启动、关闭或操作窗口；实际任务管理器分组仍需在用户启动新版后截图确认。
无窗口身份探针使用相同嵌入 manifest，从注册外部目录启动后 Windows 返回 PackageStatus=0、ApplicationStatus=0、AUMID=Hhub.Desktop_9hyarznvzybzr!Hhub，运行时统一身份已确认。此检查不等同于实际任务管理器合组截图。
侧栏使用统一 56 DIP 宽度、28 DIP 中心线、48 DIP 图标单元和对称 4 DIP 底板留白。导航项高度44 DIP，取消重复外层留白；Logo开关保持原有功能。Release构建通过，已更新 Hhub-grouped/Hhub.exe 文件，保留旧文件备份，未关闭或操作正在运行的窗口。最终运行画面尚未截图验证。
场景我的设备改为左侧56 DIP设备简图和名称状态、中央右对齐配置卡片、最右侧固定新建配置。配置卡片与新建按钮均156x40 DIP，新建配置保留原保存流程并追加到按钮左侧。配置区可横向滚动并在加载时滚至最新配置；宽度低于620 DIP时设备信息单独一行，按钮仍在右侧。Release编译通过，已更新 Hhub-grouped/Hhub.exe，未操作或重启窗口，实际画面尚未截图验证。
场景页恢复自动切换按钮已删除，设置页新增场景自动切换CheckBox。选中调用unlockScene，取消调用lockScene仅锁定当前场景而不重新应用配置，沿用sceneLocked持久化；状态回读更新选框不重绘页面。Release UI/Core编译通过，94项核心自检全部通过。新版UI及Core文件已更新至Hhub-grouped，未操作窗口或结束进程，用户需从托盘完全退出后重开以加载新的后台操作。
首页系统监测下方状态小字已删除，保留标题按钮。统一 pageTitle 为24 DIP半粗，系统监测、我的设备、场景及公共页面标题（插件市场、插件管理、Windows设置、EQ、手机遥控等）统一字号。Release UI编译通过，已更新Hhub-grouped/Hhub.exe，未操作或重启窗口。
UI内存审计见UI-MEMORY-REVIEW.md：修复动态图表重建、完整传感器趋势缓存、动态市场列表事件记录积累、图片强引用和过大解码、重复异步刷新及空闲WebView环境引用。95项核心自检通过，最终600次切页和670次40项列表重建通过资源残留检查。UI/Core新版文件已更新，未操作或关闭用户窗口；小时级实机长期增长尚未复现，不宣称已经全部解决。
H图标裁切修复：此前外层TitleBar及shell行高32 DIP，而WinUI默认ExpandedHeight模板内部48 DIP。现同时覆盖TitleBarCompactHeight、TitleBarExpandedHeight为32，LeftPaddingWidth为0。隔离无前台激活几何检查通过：header32/template32/logoY2/logoHeight28/logoCenterX28/railCenterX28。新版文件已更新，未操作当前窗口。
16处ContentDialog统一通过showDialog设置主题和XamlRoot，打开弹窗实时同步主题，弹窗期间暂缓页面重建以保留输入及事件。场景卡片和主操作按钮补齐悬停、按下色彩资源，修复浅色主题白底白字。Release编译通过；隔离测试六种场景按钮状态（深浅各正常/悬停/按下）全部可读，弹窗浅色切深色同步且输入保留；四张原生渲染截图已检查并保存至桌面新建文件夹/theme-review。新版文件已更新，未操作用户窗口。
