# 选择最新正式版 stm_log

执行技能时先查询远端标签，选择最新正式版，再把实际标签写入工程。用户明确指定版本时遵从指定值。文档中的 `v3.0.1` 只是示例，不能代替本次查询。

## 查询和选择

默认查询 Gitee；不可用时使用 GitHub 镜像：

```bash
git ls-remote --tags https://gitee.com/nzxhg/stm_log.git
git ls-remote --tags https://github.com/NingZiXi/stm_log.git
```

- 只接受匹配 `refs/tags/v<major>.<minor>.<patch>` 的正式标签，例如 `v3.0.1`；排除 `rc`、`beta`、`alpha` 和其他预发布后缀。
- 将 major、minor、patch 转为整数，按三元组比较版本；不能按字符串排序，也不能取输出的最后一行。
- 注解标签的 `refs/tags/<tag>^{}` 行给出实际提交；轻量标签直接使用标签行的提交。记录选中的仓库、标签和提交。
- `GIT_REPOSITORY` 必须是本次已确认包含该标签的仓库。两端不同步时不要假定镜像存在同名版本。
- 无法查询或没有正式标签时，说明未能确认最新版；不要悄悄使用旧版本或把 `main` 当正式版。

## 写入工程

把本次选中的具体标签填入 `FetchContent_Declare(stm_log)` 的 `GIT_TAG`，源码固定到项目的 `Lib/stm_log/`。版本查询在执行技能时完成，不在每次 CMake 配置时自动升级；普通重编译使用已锁定的版本。

文档里的 `GIT_TAG v3.0.1` 必须随本次查询结果替换。再次执行技能或用户要求升级时重新查询；只修改文档中的示例值不算实现“使用最新版本”。

升级已有工程前检查依赖目录的本地修改并保留。阅读选中版本的 README、CMake 和公开头文件，确认回调 API、`STM_LOG_WITH_RTT` 及 RTT 下载位置；若接口变化，先适配模板再构建，不能默默退回旧版。

启用 RTT 时确认编译使用 `Lib/segger_rtt/`（或用户指定的本地目录）。`stm_log` 自 v3.0.1 起会把自动下载的 RTT 放在自身同级的 `segger_rtt/`；下载管理文件与编译产物仍在构建目录。

完成后报告实际使用的标签和提交，并确认生成的 CMake 与检出的源码版本一致。离线时只能使用已准备好的明确版本，不能声称已核实远端最新版。
