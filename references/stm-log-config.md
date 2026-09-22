# stm_log 回调 API 配置参考

`stm_log` v3 系列使用平台无关的 C 日志核心，不包含 HAL/CMSIS 头文件，也不初始化 UART。UART、RTT、SWO 和 USB CDC 都由应用提供输出回调。实际版本按 [版本选择](stm-log-version.md) 查询最新正式标签；若新版 API 变化，以该版本的公开头文件为准。

## 基本配置

下文 `v3.0.1` 仅为示例，填写本次查询到的正式标签。

```cmake
include(FetchContent)
FetchContent_Declare(
    stm_log
    GIT_REPOSITORY https://gitee.com/nzxhg/stm_log.git
    GIT_TAG        v3.0.1 # 示例：替换为本次查询到的正式标签
    SOURCE_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/Lib/stm_log
)
FetchContent_MakeAvailable(stm_log)

set(CONFIG_LOG_ENABLED ON CACHE STRING "Enable stm_log output (ON/OFF)")
target_compile_definitions(stm_log PUBLIC
    STM_LOG_ENABLED=$<BOOL:${CONFIG_LOG_ENABLED}>
)
```

不再设置 `STM_LOG_HAL_HEADER` 或 `STM_LOG_LINK_CUBEMX`；这两个旧版配置在 v3 中不存在。

## 应用接入

先提供毫秒时钟，再绑定输出。回调必须在返回前完成发送或复制数据，不能保存传入的临时指针。

```c
static void uart_output(const char *data, uint16_t len)
{
    (void)HAL_UART_Transmit(&huart1, (uint8_t *)data, len, 100U);
}

stm_log_set_tick(HAL_GetTick);
stm_log_init_output(uart_output, STM_LOG_LVL_INFO);
```

RTT 只替换回调：

```c
static void rtt_output(const char *data, uint16_t len)
{
    SEGGER_RTT_Write(0, data, len);
}

SEGGER_RTT_Init();
stm_log_set_tick(HAL_GetTick);
stm_log_init_output(rtt_output, STM_LOG_LVL_INFO);
```

`stm_log_init(&huart1, level)` 已移除；不能用 `stm_log_set_output(NULL)` 恢复默认 UART，因为 v3 没有内置 UART 后端。

## RTT 依赖

需要 RTT 时，在 `FetchContent_MakeAvailable(stm_log)` 之前设置：

```cmake
set(STM_LOG_WITH_RTT ON)
```

由 `stm_log` 创建并 PUBLIC 传递 `segger_rtt` target。工程的 `main/CMakeLists.txt` 只链接 `stm_log`，不单独 FetchContent、include 或链接 `segger_rtt`。

可选变量：

```cmake
set(STM_LOG_RTT_SOURCE_DIR "C:/path/to/RTT" CACHE PATH "")
set(STM_LOG_RTT_GIT_REPOSITORY "https://github.com/NingZiXi/RTT.git" CACHE STRING "")
set(STM_LOG_RTT_FETCH OFF CACHE BOOL "")
set(STM_LOG_RTT_CONFIG_DIR "C:/path/to/config" CACHE PATH "")
```

组件会优先使用同级 `Lib/segger_rtt/` 或 `Lib/RTT/`，否则按固定提交下载到 `Lib/segger_rtt/`。源码固定在 `Lib/`，编译产物仍在构建目录。

## 常见错误

- `undefined reference to stm_log_init`：模板仍使用旧 v2 初始化接口，改为输出回调 + `stm_log_init_output`。
- `undefined reference to SEGGER_RTT_Init`：开启 `STM_LOG_WITH_RTT`，并确认 RTT 源可用。
- 日志库报 `stm32f4xx_hal.h not found`：检查实际 checkout 和编译路径，可能仍编译了旧版本。v3 不读取 `STM_LOG_HAL_HEADER`，仅保留该宏不会引发 include；应用自身的 HAL 依赖仍须正确配置。
