# 插件库管理

在插件市场标题旁点击设置按钮，进入独立的插件库管理页。每个插件库可以单独命名、编辑、启停和删除；停用或删除库不卸载已经安装的插件。原单库配置会保留。

支持的地址：

- GitHub 的 `owner/repository`、仓库 URL，以及 blob/raw 清单链接。
- Gitee 和 GitLab 仓库 URL；可填写分支与清单路径。GitLab 自托管地址请选择 GitLab 类型。
- HTTPS JSON 清单直链，包括不以 `.json` 结尾的接口地址。
- 本地 JSON 文件的完整路径或 `file:///` 地址；相对插件包路径限定在该清单目录内。

清单沿用 `schemaVersion: 1` 和 `plugins` 数组，插件包仍须提供 SHA-256、ID、作者与版本。远程包允许使用独立的 HTTPS CDN 地址。下载后校验摘要和身份，再由用户审阅权限。

同步仅处理已启用的库；每个库独立保存缓存。失败时使用该库与当前地址匹配的缓存，并在管理页显示原因。不同库中 ID 相同的插件不会静默覆盖；作者冲突阻止更新，相同 ID、作者、版本及摘要的副本在市场合并显示。

默认注册两个官方远程来源：

- 外设插件库：`https://raw.githubusercontent.com/shengerq/H-hub-plugins/main/index.json`
- 应用插件库：`https://raw.githubusercontent.com/shengerq/H-hub-app-plugins/main/index.json`

市场通过“外设插件库 / 应用插件库”切换显示，两个索引的 relatedLibraries 互相连接。一次性迁移保留自定义来源，之后尊重用户的停用和删除操作。基础安装包不携带应用插件。

手机遥控、EQ、Windows 管理当前均为 1.0；页面、预设和后台由应用插件库独立构建，通过通用插件接口加载。主程序负责插件管理、状态监测、强制停止和原生系统监测、设备监测、场景配置。

卸载先停止本插件后台并清理其资源。插件更新保留配置；手机卸载会清理配对与证书。独立安装的共享音频驱动不随插件卸载删除。Windows 插件卸载不重置用户已应用的系统偏好。

回归检查：`python PluginCatalogTests.py <Hhub.Catalog.exe 路径> [插件库 index.json 路径]`。通用原生插件契约另由核心 `--self-test` 检查。
