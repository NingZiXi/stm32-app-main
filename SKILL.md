---
name: stm32-app-main
description: 把 STM32CubeMX + CMake 工程整理为 main/ 子模块结构，并必须配置 VS Code 构建与调试入口。业务代码放入 main/，通过 add_subdirectory(main) 构建，不修改 cmake/ 或 Core/ 的生成结构。支持 FreeRTOS 与裸机。日志默认查询并使用 stm_log 最新正式版；核心不依赖 HAL，UART/RTT 由应用回调接入，RTT 由 STM_LOG_WITH_RTT=ON 管理。用户提到初始化 app_main、业务代码独立到 main/、裸机改造、RTT 后端或 J-Link RTT 时使用。
---

# STM32CubeMX 工程 → main/ 子模块

## 适用范围

- CubeMX 生成的 CMake 工程：根目录有 `.ioc` 和 `cmake/stm32cubemx/CMakeLists.txt`。
- FreeRTOS 或裸机工程。
- 需要将业务代码与 CubeMX 生成文件分开维护。

不用于 MDK、IAR、Makefile 或 CubeIDE 工程；这些工程先通过 CubeMX 生成 CMake。

## 改造后的布局

```text
工程/
├── CMakeLists.txt       # stm_log FetchContent + add_subdirectory(main)
├── .vscode/
│   ├── tasks.json       # 必须配置：调试前构建任务
│   └── launch.json      # 必须配置：匹配实际芯片和探针的调试入口
├── Core/                # CubeMX 生成代码
├── Lib/stm_log/         # 本次查询后锁定的最新正式版，SOURCE_DIR 指定
├── Lib/segger_rtt/      # 启用 RTT 时由 stm_log 自动下载或复用
└── main/
    ├── CMakeLists.txt
    └── app_main.c
```

只在 `Core/Src/main.c` 或 `freertos.c` 的 USER CODE 区域添加 `app_main()` 调用。不要修改 USER CODE 之外的生成代码，也不要把业务源文件放进 `cmake/`。

## 工作流

1. 检查 `.ioc`、CubeMX CMake 文件和 `main.c`；判断 FreeRTOS 或裸机。
   根据实际启动任务和 CMSIS-OS 版本选模板；沿用工程已有日志后端。已有 `main/` 时合并必要改动，保留业务，不直接覆盖。
2. 创建 `main/`，从 `assets/` 复制对应的 `CMakeLists.txt` 和 `app_main` 模板。
   UART：FreeRTOS 用 `app_main.c`，裸机用 `app_main_bare.c`；RTT 分别用 `app_main_rtt.c`、`app_main_bare_rtt.c`。目标文件均命名为 `main/app_main.c`，不新增头文件。注释遵循 [注释规范](references/code-comment-style.md)。
3. 按 [版本选择](references/stm-log-version.md) 查询并锁定 stm_log 最新正式标签，在根 CMake 中添加依赖和 `add_subdirectory(main)`。
4. 将 `app_main()` 放到正确的 USER CODE 区域。
5. **必须创建或合并 `.vscode/tasks.json` 和 `.vscode/launch.json`**，落实下文的调试配置要求；不以用户是否提到 F5 或调试为执行前提。
6. 运行调试配置对应的 Debug 构建任务，检查实际 ELF、入口和日志符号，完成构建与调试配置验证。仅编译通过不代表本技能已完成。

## stm_log 接入

先完成 [版本选择](references/stm-log-version.md)。以下 `v3.0.1` 仅为示例，执行时必须替换为本次查询到的最新正式标签；用户明确指定版本时使用指定值。

```cmake
include(FetchContent)
# RTT 后端 ON；UART 后端改为 OFF，避免沿用旧缓存。
set(STM_LOG_WITH_RTT ON)
FetchContent_Declare(
    stm_log
    GIT_REPOSITORY https://gitee.com/nzxhg/stm_log.git
    GIT_TAG        v3.0.1 # 示例：替换为本次查询到的正式标签
    SOURCE_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/Lib/stm_log
)
FetchContent_MakeAvailable(stm_log)

set(CONFIG_LOG_ENABLED ON CACHE STRING "Enable stm_log output (ON/OFF)")
set(APP_VERSION "1.0.0" CACHE STRING "Application firmware version")
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    CONFIG_APP_VERSION="${APP_VERSION}"
    CONFIG_LOG_ENABLED=$<BOOL:${CONFIG_LOG_ENABLED}>
)
target_compile_definitions(stm_log PUBLIC
    STM_LOG_ENABLED=$<BOOL:${CONFIG_LOG_ENABLED}>
)
add_subdirectory(main)
```

`main/CMakeLists.txt` 只链接 `stm_log`：

```cmake
target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/app_main.c)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${CMAKE_PROJECT_NAME} stm_log)
```

v3 不再使用 `STM_LOG_HAL_HEADER`、`STM_LOG_LINK_CUBEMX` 或 `stm_log_init(&huart1, ...)`。stm_log 不包含 HAL，也不内置 UART；HAL 只出现在应用自己的输出和 tick 回调中。

### UART

```c
static void uart_output(const char *data, uint16_t len)
{
    (void)HAL_UART_Transmit(&huart1, (uint8_t *)data, len, 100U);
}

stm_log_set_tick(HAL_GetTick);
stm_log_init_output(uart_output, STM_LOG_LVL_INFO);
```

### RTT

根 CMake 设置 `STM_LOG_WITH_RTT ON` 后，stm_log 会优先复用同级 `Lib/segger_rtt/` 或 `Lib/RTT/`，否则自动拉取固定 RTT 源码到 `Lib/segger_rtt/`，创建 `segger_rtt` 并通过 PUBLIC 依赖传递。主工程不再声明 `FetchContent(segger_rtt)`，不再 include `CMakeLists_rtt.txt`，`main/CMakeLists.txt` 也不单独链接 `segger_rtt`。

```c
static void rtt_output(const char *data, uint16_t len)
{
    SEGGER_RTT_Write(0, data, len);
}

SEGGER_RTT_Init();
stm_log_set_tick(HAL_GetTick);
stm_log_init_output(rtt_output, STM_LOG_LVL_INFO);
```

详细 RTT 路径和离线配置见 [references/rtt-setup.md](references/rtt-setup.md)；配置变量见 [references/stm-log-config.md](references/stm-log-config.md)。

## 日志开关

始终让调用方包含 `stm_log.h`，用 `STM_LOG_ENABLED=0` 关闭宏：

```cmake
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    CONFIG_LOG_ENABLED=$<BOOL:${CONFIG_LOG_ENABLED}>
)
target_compile_definitions(stm_log PUBLIC
    STM_LOG_ENABLED=$<BOOL:${CONFIG_LOG_ENABLED}>
)
```

如果使用 RTT，关闭日志时仍可保留 `STM_LOG_WITH_RTT`；链接器会在没有引用时回收 RTT 代码。需要彻底避免 RTT 源时可将该选项关闭，并去掉 RTT 专用应用代码。

## 入口位置

FreeRTOS：`Core/Src/freertos.c` 的 `StartDefaultTask`：

先在该文件现有的 USER CODE 声明区域添加 `void app_main(void);`，再添加调用。裸机同理在 `main.c` 的 USER CODE PFP 声明。保留已有内容，避免重复插入。`app_main()` 永不返回，只从一个普通任务或裸机入口调用；多任务日志需由应用串行化，不在 ISR 中阻塞输出。

```c
/* USER CODE BEGIN 5 */
app_main();
/* USER CODE END 5 */
```

裸机：`Core/Src/main.c` 在所有 `MX_*_Init()` 后的 USER CODE 2：

```c
/* USER CODE BEGIN 2 */
app_main();
/* USER CODE END 2 */
```

## VS Code 调试配置（必做）

每次执行本技能都必须检查并补齐 VS Code 构建与调试入口。已有正确配置时保留并验证；缺失时创建，有误时修正。这是固定交付步骤，不能作为可选后续事项。

- 根据现有工程和调试环境识别实际 MCU、探针、调试后端及工具路径。`assets/tasks.json`、`assets/launch.json` 仅作为结构模板；其中的芯片型号、工具路径、预设和目标文件必须适配当前工程。
- 在 `.vscode/tasks.json` 中配置可执行的 Debug 构建任务；需要先配置 CMake 时加入配置任务和依赖关系，确保首次启动也能构建。
- 在 `.vscode/launch.json` 中提供本次目标的可用调试配置，明确真实 ELF、启动停点和调试器参数；`preLaunchTask` 必须指向实际存在的任务标签。合并所需项，不覆盖无关的已有配置。
- 使用 RTT 时配置对应的 RTT 终端和通道，并核对控制块定位；使用 UART 或关闭日志时仍必须配置调试入口。
- 双核或多镜像工程要核对各核 ELF 和启动依赖，配置必要的固件加载及目标核心，保留既有启动同步流程；不能直接假定顶层 CMake 目标就是可调试 ELF。
- 必要参数缺失时先查现有配置和本机环境，仍无法确定再询问用户；不得用错误的模板值占位后宣称配置完成。

## 验证

```bash
cmake -S <root> -B <root>/build/Debug -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake
cmake --build <root>/build/Debug
```

- `undefined reference to stm_log_init`：仍使用 v2 API，改成输出回调、`stm_log_set_tick` 和 `stm_log_init_output`。
- `undefined reference to SEGGER_RTT_Init`：启用 `STM_LOG_WITH_RTT`，确认本地或自动获取的 RTT 源码存在。
- `HAL` 头文件找不到：移除旧 `STM_LOG_HAL_HEADER` 配置，HAL 头只由应用包含。
- `undefined reference to app_main`：确认根 CMake 有 `add_subdirectory(main)`，且入口调用位于正确 USER CODE 区域。

完成前必须确认 `.vscode/tasks.json` 和 `.vscode/launch.json` 的本次目标配置可解析，任务关联正确，工具路径可解析，构建产物与调试 ELF 一致，并已运行相同的构建命令。若这些检查未通过，必须修复或明确报告未完成项，不能只凭编译成功宣称全部完成。

交付时说明 VS Code 调试配置名称、启动停点和日志查看位置。配置与构建验证、实际板上调试分别报告；未执行实机调试时明确说明，不能声称已连接成功。使用 RTT 且已实机验证时，检查 `LOGI` 输出和 HAL tick 时间戳。

按需阅读 [CMake 集成](references/CMake-integration.md) 和 [日志开关](references/release-build.md)。CubeMX 重新生成后检查根 CMake 的用户扩展段及 VS Code 调试配置仍完整。
