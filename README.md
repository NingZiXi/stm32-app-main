# stm32-app-main

把 STM32CubeMX + CMake 工程整理为独立的 `main/` 业务子模块。技能会识别 FreeRTOS 或裸机入口，将 `app_main()` 接到 CubeMX 的 USER CODE 区域，并保持 `cmake/` 与 `Core/` 的生成结构稳定。

日志默认查询并使用 **stm_log 最新正式版**，将查询到的具体标签锁定在工程中。模板使用平台无关的输出回调 API；RTT 由 `STM_LOG_WITH_RTT=ON` 交给 stm_log 管理，源码放在 `Lib/segger_rtt/`。

## 目录和流程

```text
工程/
├── CMakeLists.txt       # FetchContent(stm_log) + add_subdirectory(main)
├── Core/                # CubeMX 生成代码
├── Lib/stm_log/         # SOURCE_DIR 指定的组件源码
├── Lib/segger_rtt/      # stm_log 自动下载或复用的 RTT 源码
└── main/
    ├── CMakeLists.txt
    └── app_main.c
```

执行步骤：探测工程 → 创建 `main/` → 查询并锁定 stm_log 最新正式版 → 配置依赖 → 接入入口 → 构建验证。FreeRTOS 使用 `StartDefaultTask` 的 USER CODE 5；裸机使用 `main.c` 的 USER CODE 2。

## stm_log 接入

先按 [版本选择规则](references/stm-log-version.md) 查询远端正式标签，再填写 `GIT_TAG`。以下 `v3.0.1` 只是示例，不是固定默认版本；普通重编译不会自动升级已锁定的依赖。

```cmake
include(FetchContent)
set(STM_LOG_WITH_RTT ON) # UART 后端改为 OFF
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
target_compile_definitions(stm_log PUBLIC STM_LOG_ENABLED=$<BOOL:${CONFIG_LOG_ENABLED}>)
add_subdirectory(main)
```

`main/CMakeLists.txt` 只需 `target_link_libraries(${CMAKE_PROJECT_NAME} stm_log)`。RTT 的 `segger_rtt` target 和头文件路径由 stm_log 的 PUBLIC 依赖传递；不再在根 CMake 单独 FetchContent 或包装 RTT。

v3 中不再使用 `STM_LOG_HAL_HEADER`、`STM_LOG_LINK_CUBEMX` 或 `stm_log_init(&huart1, ...)`。

### UART 模板

```c
static void uart_output(const char *data, uint16_t len)
{
    (void)HAL_UART_Transmit(&huart1, (uint8_t *)data, len, 100U);
}

stm_log_set_tick(HAL_GetTick);
stm_log_init_output(uart_output, STM_LOG_LVL_INFO);
```

### RTT 模板

```cmake
set(STM_LOG_WITH_RTT ON)
```

```c
static void rtt_output(const char *data, uint16_t len)
{
    SEGGER_RTT_Write(0, data, len);
}

SEGGER_RTT_Init();
stm_log_set_tick(HAL_GetTick);
stm_log_init_output(rtt_output, STM_LOG_LVL_INFO);
```

完整模板位于 `assets/`，详细依赖说明见 [references/stm-log-config.md](references/stm-log-config.md) 和 [references/rtt-setup.md](references/rtt-setup.md)。

## 源码位置和升级

`SOURCE_DIR` 指定源码存放位置，不代表已有目录会跳过 Git 更新；FetchContent 仍管理固定 tag。离线时先准备匹配版本的完整源码，并设置本地覆盖：

```bash
cmake --preset Debug -DFETCHCONTENT_SOURCE_DIR_STM_LOG="C:/path/to/Lib/stm_log"
```

本地覆盖时由应用维护源码版本；不要通过删除依赖目录处理升级，先检查并保留未提交修改。

## 验证

```bash
cmake -S . -B build/Debug -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake
cmake --build build/Debug
```

若看到 `undefined reference to stm_log_init`，说明业务代码仍使用旧 v2 API；改为输出回调、`stm_log_set_tick` 和 `stm_log_init_output`。若 RTT 符号缺失，确认设置了 `STM_LOG_WITH_RTT=ON`。

## 模板文件

| 文件 | 用途 |
|---|---|
| `assets/CMakeLists.txt` | `main/` 的构建入口，只链接 stm_log |
| `assets/app_main.c` | FreeRTOS + UART |
| `assets/app_main_bare.c` | 裸机 + UART |
| `assets/app_main_rtt.c` | FreeRTOS + RTT |
| `assets/app_main_bare_rtt.c` | 裸机 + RTT |
| `references/CMake-integration.md` | CMake 和 CubeMX 边界 |
| `references/release-build.md` | 日志编译开关 |

## License

MIT —— 见 [LICENSE](LICENSE)。
