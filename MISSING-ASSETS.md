# 仓库缺失文件清单

本仓库当前缺少若干构建/发布/安装环节所需的文件。
这些文件**不在版本控制中**，需要单独获取后放入对应位置，仓库才能完整构建与打包。

## 影响构建（必须补）

| 文件 | 期望路径 | 被谁引用 | 说明 |
| --- | --- | --- | --- |
| （无）| — | — | 源码构建所需依赖均可由脚本获取，见下 |

> 说明：`app/sensors/LibreHardwareMonitorLib.dll` 曾被视为缺失，
> 实际由 `source/fetch-dependencies.ps1` 自动下载并校验（LibreHardwareMonitor 0.9.6，
> SHA-256 校验见该脚本），**无需手动放入**。

## 影响打包/安装（必须补）

| 文件 | 期望路径 | 被谁引用 | 说明 |
| --- | --- | --- | --- |
| PawnIO_setup.exe | driver/pawnio/PawnIO_setup.exe | installer/Hhub.iss 第 46 行 | PawnIO 驱动安装器，用于 CPU 温度/功耗读取 |
| RELEASE-NOTES.md | 仓库根目录 | installer/Hhub.iss 第 57 行 | 版本发布说明 |
| 使用说明-1.0.9.md | 仓库根目录 | installer/Hhub.iss 第 58 行 | 会改名打包为 app/使用说明.md |

## 影响更新分发（必须补）

| 文件 | 期望路径 | 被谁引用 | 说明 |
| --- | --- | --- | --- |
| stable.json | updates/stable.json | app/update-source.json 指向其 GitHub raw 链接 | 更新清单（版本、下载地址、SHA-256） |

## 已补齐

| 文件 | 状态 |
| --- | --- |
| source/Hhub.sln | 已新增（串联 4 个 vcxproj，Release|x64） |
| .gitignore | 已新增 |
| .gitattributes | 已新增 |
| source/README.md | 已新增（构建说明） |

## 备注

- 以上路径均相对仓库根目录。
- 打包时若缺少 driver/pawnio/PawnIO_setup.exe，安装器相关行会失败。
- updates/stable.json 未放入仓库是发行规范（RELEASE-POLICY.md）的一部分，
  但仍需由发布者维护在对应位置/分支供客户端拉取。
