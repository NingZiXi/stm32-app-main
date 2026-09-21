---
name: stm32-app-main
description: 把 STM32CubeMX + CMake 工程整理为 main/ 子模块结构：业务代码放入 main/，通过 add_subdirectory(main) 构建，不修改 cmake/ 或 Core/ 的生成结构。支持 FreeRTOS 与裸机。日志使用 stm_log v3.0.0；核心不依赖 HAL，UART/RTT 都由应用回调接入，RTT 由 STM_LOG_WITH_RTT=ON 交给 stm_log 管理。用户提到初始化 app_main、业务代码独立到 main/、裸机改造、RTT 后端或 J-Link RTT 时使用。
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
├── Core/                # CubeMX 生成代码
├── Lib/stm_log/         # v3.0.0，SOURCE_DIR 指定
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
3. 在根 CMake 中添加 stm_log v3.0.0 和 `add_subdirectory(main)`。
4. 将 `app_main()` 放到正确的 USER CODE 区域。
5. 构建 Debug 固件，检查没有旧版 stm_log API 或 RTT 链接错误。

## stm_log v3.0.0 接入

```cmake
include(FetchContent)
# RTT 后端 ON；UART 后端改为 OFF，避免沿用旧缓存。
set(STM_LOG_WITH_RTT ON)
FetchContent_Declare(
    stm_log
    GIT_REPOSITORY https://gitee.com/nzxhg/stm_log.git
    GIT_TAG        v3.0.0
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

根 CMake 设置 `STM_LOG_WITH_RTT ON` 后，stm_log 会优先复用同级 `Lib/segger_rtt/` 或 `Lib/RTT/`，否则自动拉取固定 RTT 源码，创建 `segger_rtt` 并通过 PUBLIC 依赖传递。主工程不再声明 `FetchContent(segger_rtt)`，不再 include `CMakeLists_rtt.txt`，`main/CMakeLists.txt` 也不单独链接 `segger_rtt`。

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

构建完成后可用 Cortex-Debug + J-Link RTT 观察日志；`stm_log_set_tick(HAL_GetTick)` 后时间戳应随 HAL tick 增长。

按需阅读 [CMake 集成](references/CMake-integration.md) 和 [日志开关](references/release-build.md)。用户需要 F5 调试时合并 `assets/tasks.json`、`assets/launch.json`，核对实际 MCU、J-Link 路径、构建预设和 ELF 路径，不覆盖已有配置。CubeMX 重新生成后检查根 CMake 的用户扩展段仍完整。
