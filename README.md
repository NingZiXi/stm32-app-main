# stm32-app-main

在 STM32CubeMX 1（`.ioc`）或 STM32CubeMX2（`.ioc2`）生成的 CMake 工程中，将业务逻辑接入独立 `main/`，配置 `stm_log`；RTT 模式按需配置 VS Code 构建/调试入口，UART 模式沿用默认设置。**动手选择日志模板之前，MX1 与 MX2 均须主动询问用户使用 UART 还是 RTT。**

- [SKILL.md](SKILL.md)：识别、提问、流程与共同验收。
- [MX1 CMake 集成](references/CMake-integration.md)：`Core/Src` 的 USER CODE 接入。
- [MX2 集成](references/CubeMX2.md)：根 `main.c`、HAL2、CMake preset 和重生成冲突处理。
- [版本选择](references/stm-log-version.md)：执行时确认并锁定正式版本；文档标签只是示例。

应用实现：`main/app_main.c`。`assets/app_main.c`、`app_main_bare.c`、`app_main_rtt.c`、`app_main_bare_rtt.c` 是 MX1/HAL1 模板；`assets/app_main_bare_mx2_uart.c`、`app_main_bare_mx2_rtt.c` 是依据 STM32C532CCT6 HAL2 工程写的 MX2 **裸机示例**，UART 外设需按实际工程修改。MX2 FreeRTOS 任务入口及 API 应检查实际产物，不直接复用 HAL1/旧 CMSIS-OS 模板。

`assets/CMakeLists.txt` 为 MX1 的 `main/` 模板；MX2 先核对生成目标的链接签名，再参照 `assets/CMakeLists_mx2.txt`；`assets/tasks.json`、`launch.json` 只提供结构示意，含 MX1 风格占位值，切勿原样用于其他芯片、工具链、探针或 MX2 preset。RTT 模式按实际工程配置并验证 VS Code；UART 模式不因日志接入而新建或改写 `.vscode/tasks.json`、`launch.json`。无法确认硬件时明确报告未验证。

CubeMX2 的 `main.c` 是用户文件，但重新生成时的冲突处理仍可以覆盖它；入口调用和根 CMake 扩展应在重生成前备份，之后检查差异并验证。详见 [MX2 重生成](references/CubeMX2.md)。

License: MIT，见 [LICENSE](LICENSE)。


