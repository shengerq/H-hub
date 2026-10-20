# 设备插件提供远程参数适配

手机插件通过 `ui.remoteAdapterVersion/remoteControls` 识别能力，按声明校验参数后创建授权任务。设备插件的 `hhubProfileAdapter.patch/remoteValues` 实现具体参数映射；手机 Worker 不再引用雷柏协议常量或搜索插件源码识别型号。

本次与 VT3S MAX 0.5.3 同步更新。手机包保持 1.0，可通过导入本次 `.hhub` 替换已安装的 1.0。请同步更新主程序、VT3 和手机包。Android 的原有参数界面保持不变。

27 项边界测试：`RemoteControlSchemaTests.vcxproj`，运行生成的程序并传入 VT3 插件包路径。测试使用包内参数声明，不连接 HID。迁移不代表手机真机及真实鼠标回归已完成。
