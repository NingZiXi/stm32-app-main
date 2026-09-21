# Debug / Release 日志裁剪

`stm_log v3.0.0` 支持 `STM_LOG_ENABLED` 编译期开关。工程应同时给应用和 `stm_log` target 传递同一个值，确保 LOG 宏和库实现一致。

## CMake

```cmake
set(CONFIG_LOG_ENABLED ON CACHE STRING "Enable stm_log output (ON/OFF)")
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    CONFIG_LOG_ENABLED=$<BOOL:${CONFIG_LOG_ENABLED}>
)
target_compile_definitions(stm_log PUBLIC
    STM_LOG_ENABLED=$<BOOL:${CONFIG_LOG_ENABLED}>
)
```

需要 RTT 时，仍由 `stm_log` 的 `STM_LOG_WITH_RTT=ON` 管理依赖；工程不单独链接 `segger_rtt`。

## 应用模板

`stm_log.h` 始终包含。RTT 应用在日志开启时完成初始化：

```c
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
    LOGI("main", "Boot");
}
```

若工程希望在 `CONFIG_LOG_ENABLED=OFF` 时不包含 RTT 源，可用条件编译保护 `SEGGER_RTT.h` 和初始化代码，同时关闭 `STM_LOG_WITH_RTT`；不要条件删除 `stm_log.h`。

## 构建

```bash
cmake -S . -B build/Debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCONFIG_LOG_ENABLED=ON
cmake --build build/Debug
cmake -S . -B build/Release -G Ninja -DCMAKE_BUILD_TYPE=Release -DCONFIG_LOG_ENABLED=OFF
cmake --build build/Release
```

结合 `-ffunction-sections -fdata-sections` 和链接器 `--gc-sections`，关闭日志后未引用的格式化、输出和 RTT 代码会被回收。实际节省量应以 `.map` 和 `arm-none-eabi-size` 为准，不能按固定 KB 承诺。

## 验证

```bash
arm-none-eabi-nm build/Release/*.elf | Select-String stm_log
arm-none-eabi-size build/Debug/*.elf build/Release/*.elf
```

确认 Debug 有日志符号，Release 的日志调用被裁剪或不再产生输出。`STM_LOG_ENABLED` 必须只由 CMake 统一定义，避免宏重定义警告。

## 常见错误

- `STM_LOG_ENABLED redefined`：检查工程是否在其他头文件手动定义了该宏。
- `undefined reference to stm_log_init`：这是 v2 API；v3 使用输出回调、`stm_log_set_tick`、`stm_log_init_output`。
- `undefined reference to SEGGER_RTT_Init`：启用 `STM_LOG_WITH_RTT` 或移除 RTT 应用代码。
