# SEGGER RTT 与 stm_log v3 接入

RTT 是 `stm_log` 的可选传递依赖，不由主工程单独声明。需要 RTT 时，在拉取 `stm_log` 前设置：

先按 [版本选择](stm-log-version.md) 查询最新正式标签；以下 `v3.0.1` 只是示例，执行时替换为本次选中的标签。

```cmake
include(FetchContent)
set(STM_LOG_WITH_RTT ON)
FetchContent_Declare(
    stm_log
    GIT_REPOSITORY https://gitee.com/nzxhg/stm_log.git
    GIT_TAG v3.0.1 # 示例：替换为本次查询到的正式标签
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Lib/stm_log
)
FetchContent_MakeAvailable(stm_log)
```

`stm_log` 会优先寻找同级 `Lib/segger_rtt/` 或 `Lib/RTT/`，否则按组件中固定的 RTT 提交自动下载到 `Lib/segger_rtt/`。它创建 `segger_rtt` 静态库并通过 `stm_log` 的 PUBLIC 链接关系传给最终应用。`main/CMakeLists.txt` 只写：

```cmake
target_link_libraries(${CMAKE_PROJECT_NAME} stm_log)
```

不再使用根 CMake 的 `FetchContent_Declare(segger_rtt)`、`CMakeLists_rtt.txt` 或 `if(TARGET segger_rtt)`。

## 应用代码

```c
#include "main.h"
#include "stm_log.h"
#include "SEGGER_RTT.h"

static void rtt_output(const char *data, uint16_t len)
{
    SEGGER_RTT_Write(0, data, len);
}

void app_main(void)
{
    SEGGER_RTT_Init();
    stm_log_set_tick(HAL_GetTick);
    stm_log_init_output(rtt_output, STM_LOG_LVL_INFO);
    LOGI("main", "RTT ready");
    for (;;) { HAL_Delay(1000U); }
}
```

HAL 只出现在应用：`stm_log` 不需要 `STM_LOG_HAL_HEADER`，也不链接 `stm32cubemx`。若关闭 `STM_LOG_ENABLED`，仍可保留头文件；日志宏会变为空操作。

## 路径和离线配置

推荐布局：

```text
Lib/
├── stm_log/       # 本次查询后锁定的最新正式版
└── segger_rtt/    # 自动下载或复用的 RTT 源码
```

离线构建可设置 `STM_LOG_RTT_SOURCE_DIR`；禁止下载则设置 `STM_LOG_RTT_FETCH=OFF`。自定义 `SEGGER_RTT_Conf.h` 所在目录通过 `STM_LOG_RTT_CONFIG_DIR` 传入。

stm_log 自 v3.0.1 起将自动下载的 RTT 源码放在自身同级的 `segger_rtt/`；按本技能布局即 `Lib/segger_rtt/`。只有下载管理文件和编译产物位于构建目录，清理 `build/` 不会删除 RTT 源码。显式源码目录须已含 `RTT/SEGGER_RTT.c`、`RTT/SEGGER_RTT.h` 和 `Config/SEGGER_RTT_Conf.h`。已有 target 优先于显式目录，显式目录优先于同级目录，最后才下载；本地版本由应用负责。

## 验证

确认 `SEGGER_RTT.c` 被编译、`SEGGER_RTT.h` 可被包含，并通过 J-Link RTT 或 Cortex-Debug 观察 `LOGI` 输出。`stm_log_set_tick(HAL_GetTick)` 后日志中的时间戳应随 HAL tick 增长。
