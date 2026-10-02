---
name: stm32-app-main
description: 为已有 STM32CubeMX1 (.ioc) CMake 工程接入独立 main/ 业务模块和 stm_log v3.0.0，按实际裸机或 FreeRTOS 入口及 UART/RTT 后端适配。用于 app_main 接入，不用于维护组件库、创建 CubeMX 工程或 CubeMX2/HAL2。
---

# CubeMX1 应用入口与日志接入

## 识别与边界

- 确认 `.ioc`、`cmake/stm32cubemx/CMakeLists.txt`、应用 target 和实际生成入口；仅适用于已有 CubeMX1 CMake 工程。MDK/IAR/Makefile/CubeIDE 或 CubeMX2 不直接套用本版本模板。
- 先读目标工程适用的指令与现有业务、依赖和构建配置。已有 `main/` 时合并，不覆盖；入口调用只进入生成代码的 USER CODE 区域，业务源码不放入生成目录。
- 沿用用户已选后端或工程明确的日志方案；无法确定时询问 UART/RTT 及可用外设或探针。模板里的 huart1、MCU、路径和 FreeRTOS API 必须按实际工程核对。
- 本版本 FetchContent 固定 stm_log v3.0.0。已有不同版本或同名 target 时先确认接入策略，不并行创建两个 stm_log。依赖版本事实查选用源码的公开头文件和 CMake；不要用总仓库 v2 说明替代 v3 接口。

## 流程与按需读取

1. 读取 [CMake 集成](references/CMake-integration.md)，确定根目标、依赖、日志开关与 `add_subdirectory(main)` 的位置。
2. 按裸机/FreeRTOS 与日志后端选择一个模板，适配为 `main/app_main.c`；`main/CMakeLists.txt` 参考 `assets/CMakeLists.txt`。同一 target 已有 plain 或 keyword 链接签名时保持一致，不混用。

   | 入口 | UART | RTT |
   | --- | --- | --- |
   | 裸机 | `assets/app_main_bare.c` | `assets/app_main_bare_rtt.c` |
   | FreeRTOS | `assets/app_main.c` | `assets/app_main_rtt.c` |

3. 读取 [日志回调](references/stm-log-config.md)并核对 HAL tick 和输出契约；仅选 RTT 时读取 [RTT 依赖](references/rtt-setup.md)。设置 `STM_LOG_WITH_RTT` 的实际 ON/OFF 值，避免继承不匹配缓存；应用只链接 stm_log，由它传递 RTT。
4. 在已有 USER CODE 声明区域添加 `void app_main(void);`。裸机在所有 `MX_*_Init()` 后的 `USER CODE 2` 调用；FreeRTOS 核对实际普通任务后调用，常见为 StartDefaultTask 的 USER CODE 5。保留已有内容，不重复挂载。模板入口永不返回，不从 ISR 调用；并发日志由应用串行化。
5. 修改日志裁剪时读取 [Debug/Release](references/release-build.md)，调整注释时读取[应用注释约定](references/code-comment-style.md)。不读取无关后端模板或参考资料。
6. 按工程实际工具链/preset 构建 Debug，核对 app_main 与日志源码参与编译、符号和 ELF。用户需要 F5/RTT 调试配置时，按实际环境合并 `assets/tasks.json` 与 `assets/launch.json`，核对工具、preLaunchTask、首构建 configure 与 ELF；示例值不能直接当作可用配置。

## 验证与交付

- v3 使用输出回调、`stm_log_set_tick` 和 `stm_log_init_output`，不依赖 HAL，也没有旧版内置 UART 初始化；实际 HAL 依赖属于应用。
- 复核修改只发生在允许的生成区域与应用文件；CubeMX 重生成后检查入口和根 CMake 接入仍完整。
- 报告选用的入口/后端、固定依赖版本、构建命令与产物、日志查看方式以及未验证项。编译通过、调试配置检查和真实板上日志观察分别报告；没有连接开发板时不声明实机调试成功。
