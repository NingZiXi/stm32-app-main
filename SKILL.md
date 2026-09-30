---
name: stm32-app-main
description: 将 STM32CubeMX 1 (.ioc) 或 STM32CubeMX2 (.ioc2) 的 CMake 工程接入独立 main/ 业务模块、stm_log 和 VS Code 构建调试。根据生成器和 HAL API 分流，支持裸机与经实际工程确认的 FreeRTOS 入口；执行前询问串口或 RTT 日志后端。
---

# STM32CubeMX CMake 工程的应用入口

目标：业务代码在独立的 `main/`，应用启动入口只做必要的调用，使用 `stm_log`；RTT 模式交付对应的 VS Code 构建/调试配置，UART 模式沿用已有或默认的 VS Code 配置。不要修改不相关业务代码，不要覆盖已有 `main/`。

## 识别与提问（两个路径都必须执行）

1. 识别工程：`.ioc` + `cmake/stm32cubemx/CMakeLists.txt` 属 MX1；`.ioc2` + `CMakePresets.json` + `generated/` 属 MX2。若混合或路径分离，确认实际生成输出目录和根目标后再编辑。只适用于 CMake；MDK/IAR/Makefile/CubeIDE 不按此流程直接修改。
2. 检查芯片、HAL 版本、是否 FreeRTOS、实际 UART 外设与引脚、现有日志后端和 `main/`，保留已有业务和正确的配置。
3. **在 MX1 和 MX2 两种路径下都主动询问用户：`stm_log` 本次使用串口 UART 还是 RTT？** 不以现有串口、J-Link、模板默认值代替询问。未收到明确选择前，不选模板、不下载 RTT、不改项目的日志依赖或输出回调。用户选 UART 时核对实际可用于日志的串口；选 RTT 时检查探针与 RTT 调试方式。如果用户要求暂不配置日志，先说明这与本技能默认 `stm_log` 交付要求不同，再按其指示处理并如实标注未完成项。
4. 两种路径均按 [版本选择](references/stm-log-version.md) 查询并锁定 `stm_log` 正式版本，确认所选版本 API；通过 `FetchContent` 接入。`STM_LOG_WITH_RTT`：RTT 为 `ON`，UART 为 `OFF`；不要由上次的 CMake 缓存决定后端。库本身不依赖 HAL；应用提供输出回调和 tick。

## 分流与入口

| | CubeMX 1 | CubeMX2 |
|---|---|---|
| 详情 | [MX1 CMake 与入口](references/CMake-integration.md) | [MX2 CMake、HAL2 与重生成](references/CubeMX2.md) |
| 工程结构 | `Core/`, `cmake/stm32cubemx/`, `.ioc` | 根 `main.c`, `generated/`, `user_modifiable/`, `.ioc2` |
| 裸机入口 | `Core/Src/main.c` 的 `USER CODE 2`，完成所有 `MX_*_Init()` 后 | 根 `main.c` 的 `mx_system_init() == SYSTEM_OK` 成功分支，调用 `app_main()`；生成代码没有 `USER CODE` 区 |
| FreeRTOS | 检查实际默认任务；常见为 `Core/Src/freertos.c` 的 `StartDefaultTask` / `USER CODE 5` | 先识别实际生成的调度器、任务创建与入口，不假定 MX1 的 `freertos.c` 或 CMSIS-OS 文件存在；无明确挂载点时询问，不硬套裸机入口 |
| 应用模板 | UART：`assets/app_main.c` / `assets/app_main_bare.c`；RTT：`assets/app_main_rtt.c` / `assets/app_main_bare_rtt.c` | 裸机 UART：`assets/app_main_bare_mx2_uart.c`；裸机 RTT：`assets/app_main_bare_mx2_rtt.c`；RTOS 从实际生成的任务 API 适配，不复制 MX1 模板 |

把选中的应用模板复制为 `<root>/main/app_main.c`；MX1 将 `assets/CMakeLists.txt` 用作 `main/CMakeLists.txt`；MX2 参照 `assets/CMakeLists_mx2.txt`（生成的 `components.cmake` 使用 plain `target_link_libraries` 签名）。实际工程先检查同一 target 已用的签名，不能混用。先检查存在的文件并合并，不覆盖用户代码。模板中的 UART 实例名称、类型、头文件、输出 API、时钟 API、RTOS API 以目标芯片实际生成的头文件为准；不能把 HAL1 的 `extern UART_HandleTypeDef huart1` 用在 HAL2。

在根 `CMakeLists.txt` 的目标建立后，按 [MX1 集成](references/CMake-integration.md) 或 [MX2 集成](references/CubeMX2.md) 增加依赖、版本与 `add_subdirectory(main)`。不要将 MX1 的 `add_subdirectory(cmake/stm32cubemx)` 复制进 MX2；也不要将业务源文件塞进生成目录。如果 app target 不存在，应配置失败，不静默跳过业务构建。

## VS Code 配置（按日志后端分流）

- **UART（MX1/MX2）**：使用已有或默认的 VS Code 工程配置。日志从串口监视器查看；**不因为接入 `stm_log` 而新建或改写 `.vscode/tasks.json`、`.vscode/launch.json`**。构建依然按工程实际工具链或 CMake preset 在命令行验证；若用户另行要求 VS Code 调试，再按需处理。
- **RTT（MX1/MX2）**：检查并按需创建/合并 `.vscode/tasks.json`、`.vscode/launch.json`；保留无关的已有配置。`assets/tasks.json` 和 `assets/launch.json` 仅作结构示例，其中芯片、探针、工具路径和 `build/Debug` 不可原样复制。根据实际 MCU、探针、GDB server、构建预设和 ELF 配置 `preLaunchTask`、RTT 通道和终端；首构建需要能够 configure。探针/软件无法确定时询问用户，不能伪造可用的调试配置。
## 验证与交付

- 用实际 CMake preset/工具链运行 configure 和 Debug build（MX2 优先 `cmake --preset <实际预设>`、`cmake --build --preset <实际预设>`）；查实际 ELF、`app_main` 和日志符号。没有固定 `build/Debug` 路径或固定 toolchain 文件。检查 UART 或 RTT 编译配置一致，`main/app_main.c` 确实被编译并链接。
- RTT 模式校验 VS Code 任务关联、工具路径、芯片与 ELF；UART 模式不新增/修改 VS Code 任务或启动配置，报告沿用默认配置。区分构建通过、调试配置可用、实际连接开发板三种结果。没有实机验证时不可称“调试成功”。
- MX1 复核 `USER CODE` 改动；MX2 重生成前后复核根 `main.c` 和 `CMakeLists.txt` 的调用及接入。MX2 的 `main.c` 属用户文件，**不是不可覆盖文件**；冲突处理选择可能覆盖，详见 [MX2 重生成](references/CubeMX2.md)。
- 交付说明所选 UART/RTT 后端、`stm_log` 锁定版本、构建预设、所用（或未改动）的 VS Code 配置、日志查看方式和未验证事项。

日志开关见 [release-build.md](references/release-build.md)，RTT 依赖与离线配置见 [rtt-setup.md](references/rtt-setup.md)，接口与回调见 [stm-log-config.md](references/stm-log-config.md)。


