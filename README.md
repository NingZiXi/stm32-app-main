# stm32-app-main

将已有 STM32CubeMX1 的 CMake 工程接入独立 `main/app_main.c` 和 stm_log，支持裸机及经实际生成代码确认的 FreeRTOS 入口。本版本固定使用 stm_log v3.0.0；不负责创建 CubeMX 工程，也不声明 CubeMX2/HAL2 支持。

## 安装与使用

将整个技能目录放到 Agent 客户端支持的技能位置，确保目录下直接有 `SKILL.md`，并保留 references/assets。是否需要重新加载、能否自动发现，取决于客户端；执行前确认实际加载的目录和版本。

例如：使用 stm32-app-main 在现有 `.ioc` + CMake 工程中接入独立 main/，保留业务代码，选择 UART 或 RTT 输出并构建核验。

执行要求见 [SKILL.md](SKILL.md)。技能读取目标工程的指令并按实际接口适配；模板中的串口、芯片、调试器路径和注释元数据需要按工程调整。

## 阅读与资源

| 材料 | 职责 |
| --- | --- |
| [SKILL.md](SKILL.md) | 识别工程、选择模板、接入入口与交付验证 |
| [CMake 集成](references/CMake-integration.md) | 固定依赖、源码位置、入口声明及构建 |
| [日志配置](references/stm-log-config.md) | 输出回调与 v3 API 边界 |
| [RTT](references/rtt-setup.md) | 仅 RTT 后端的依赖、离线与观察方法 |
| [日志裁剪](references/release-build.md) | 开关及 Debug/Release 验证 |
| [注释](references/code-comment-style.md) | 目标应用约定的适配 |
| assets/ | 裸机/FreeRTOS、UART/RTT 和可选 VS Code 示例 |

这些材料按任务读取，不需要将全部模板和参考文档放入常驻上下文。Skill 的 FetchContent 依赖与 stm32-hal-lib 总仓库的固定组件组合分别管理，不能混用两版 stm_log 的接口或重复创建同名 target。

## 许可证

[MIT](LICENSE)。本技能在独立仓库维护；在其他仓库中作为子模块分发时，修改与引用更新分别管理。
