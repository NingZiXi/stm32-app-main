# CMake `main/` + stm_log v3 集成

## 稳定边界

| 文件 | 处理 |
|---|---|
| 根 `CMakeLists.txt` | 添加 `stm_log` FetchContent、配置日志选项、`add_subdirectory(main)` |
| `main/CMakeLists.txt` | 添加 `app_main.c`，只链接 `stm_log` |
| `cmake/`、`Core/` | 不改 CubeMX 生成区域；入口调用只放在 USER CODE 区域 |
| `.vscode/tasks.json`、`.vscode/launch.json` | 每次执行都必须创建、合并或验证；保留正确的已有配置 |

## 根 CMake

先按 [版本选择](stm-log-version.md) 查询最新正式标签。下文 `v3.0.1` 仅为示例，生成工程时替换为实际选中值。

```cmake
add_subdirectory(cmake/stm32cubemx)

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
target_compile_definitions(stm_log PUBLIC
    STM_LOG_ENABLED=$<BOOL:${CONFIG_LOG_ENABLED}>
)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    CONFIG_APP_VERSION="${APP_VERSION}"
    CONFIG_LOG_ENABLED=$<BOOL:${CONFIG_LOG_ENABLED}>
)
add_subdirectory(main)
```

v3 核心不包含 HAL，也不需要 `STM_LOG_HAL_HEADER` 或 `STM_LOG_LINK_CUBEMX`。RTT 由 `STM_LOG_WITH_RTT=ON` 触发并通过 `stm_log` 的 PUBLIC 依赖传递；根工程不再单独声明 `segger_rtt`。

## main/CMakeLists.txt

```cmake
if(TARGET ${CMAKE_PROJECT_NAME})
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/app_main.c)
    target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(${CMAKE_PROJECT_NAME} stm_log)
endif()
```

## 应用初始化

UART 和 RTT 都是应用回调：

```c
static void output(const char *data, uint16_t len)
{
    (void)HAL_UART_Transmit(&huart1, (uint8_t *)data, len, 100U);
}

stm_log_set_tick(HAL_GetTick);
stm_log_init_output(output, STM_LOG_LVL_INFO);
```

RTT 回调改用 `SEGGER_RTT_Write`，并在此之前调用 `SEGGER_RTT_Init()`。`stm_log_init(&huart1, ...)` 已移除。

## FetchContent 路径

`SOURCE_DIR` 将源码放在 `<root>/Lib/stm_log/`，RTT 自动下载到同级的 `<root>/Lib/segger_rtt/`，编译产物仍在构建目录。目录存在不代表跳过 Git 更新。离线时准备好与锁定标签一致的源码，再显式设置本地覆盖：

```bash
cmake --preset Debug -DFETCHCONTENT_SOURCE_DIR_STM_LOG="C:/path/to/Lib/stm_log"
```

## 验证与错误

先按 `SKILL.md` 的必做调试配置步骤关联构建任务与调试入口，再运行同一组 Debug 构建命令。只有编译成功、没有有效的 VS Code 调试配置时，任务仍未完成。

```bash
cmake -S <root> -B <root>/build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build <root>/build
```

- `undefined reference to app_main`：缺少 `add_subdirectory(main)`。
- `undefined reference to stm_log_init`：使用了 v2 接口，改用输出回调和 `stm_log_init_output`。
- `undefined reference to SEGGER_RTT_Init`：启用 `STM_LOG_WITH_RTT`，确认 RTT 源可用。
- `HAL` 头文件错误：删除旧的 `STM_LOG_HAL_HEADER` 配置，HAL 只由应用包含。
