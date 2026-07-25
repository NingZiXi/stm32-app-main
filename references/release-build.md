# Debug / Release 构建优化（RTT + stm_log 整链路裁剪）

调试阶段需要看日志（哪个中断触发了、串口收到了什么数据），出货阶段只需要最小固件。把"日志链路"做成**编译期开关**能省 **~13 KB FLASH + ~2 KB RAM**，几乎免费。

## 核心思想

| 资源 | Debug（带日志） | Release（无日志） |
|---|---|---|
| **stm_log 库** | 链入（~1.5 KB） | 链入但 `--gc-sections` 剔除未用函数（净 0 KB） |
| **SEGGER RTT 库** | 链入（~3 KB） | 链入但 GC 剔除（净 0 KB） |
| **newlib printf** | 链入（~1.5 KB） | GC 剔除（净 0 KB） |
| **HAL 默认代码** | `-O0` 全部编译 | `-Os` + `NDEBUG` 砍 ~30% |

**关键机制**：
- `target_link_libraries(... stm_log segger_rtt)` **始终链**（不条件）
- `STM_LOG_ENABLED=0` 让 `LOGx` 宏变 `do{}while(0)` 空操作
- 链接器 `--gc-sections`（CubeMX 默认开）把没引用的函数全删

---

## 前提：stm_log 版本 ≥ v2.3.1

`stm_log` v2.3.1+ 的 `stm_log_config.h` 已经把 `STM_LOG_ENABLED` 用 `#ifndef` 保护：

```c
#ifndef STM_LOG_ENABLED
#define STM_LOG_ENABLED 1
#endif
```

这是关键——避免命令行 `-DSTM_LOG_ENABLED=0` 与文件内 `#define` 触发 redefinition 警告。

> 如果 FetchContent 拉到的版本 < v2.3.1，需要**手动改** `Lib/stm_log/stm_log_config.h` 加 `#ifndef`，或升级 tag。**升级到 v2.3.1 是首选**。

---

## 三步接入

### ① 根 `CMakeLists.txt` 加 LOG_ENABLED 开关

```cmake
# 调试日志开关：默认按 build type 自动（Debug=ON / Release=OFF），可用 -DLOG_ENABLED=ON/OFF 覆盖
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(_LOG_ENABLED_DEFAULT ON)
else()
    set(_LOG_ENABLED_DEFAULT OFF)
endif()
option(LOG_ENABLED "Enable RTT + stm_log debug logging" ${_LOG_ENABLED_DEFAULT})

# 联动两个宏：LOG_ENABLED（业务开关）+ STM_LOG_ENABLED（stm_log 库开关）
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    LOG_ENABLED=$<BOOL:${LOG_ENABLED}>
    STM_LOG_ENABLED=$<BOOL:${LOG_ENABLED}>
)
target_compile_definitions(stm_log PUBLIC STM_LOG_ENABLED=$<BOOL:${LOG_ENABLED}>)

# 始终链（让链接器 GC 自动剔除不用的）
target_link_libraries(${CMAKE_PROJECT_NAME} stm_log)
if(TARGET segger_rtt)
    target_link_libraries(${CMAKE_PROJECT_NAME} segger_rtt)
endif()
```

### ② `main/app_main.c` 任何位置都能写 LOGx

```c
#include "main.h"
#include "stm_log.h"                  /* 永远 include，宏由 STM_LOG_ENABLED 控制 */

#if LOG_ENABLED
#include "SEGGER_RTT.h"               /* SEGGER_RTT_Init() 等用 */
#endif

#if LOG_ENABLED
static const char *TAG = "main";

static void rtt_output(const char *buf, uint16_t len) {
    SEGGER_RTT_Write(0, buf, len);
}
#endif

void app_main(void) {
    /* 业务初始化（不带日志） */
    
#if LOG_ENABLED
    SEGGER_RTT_Init();
    stm_log_init_output(rtt_output, STM_LOG_LVL_INFO);
#endif
    
    LOGI(TAG, "Boot ...");             /* 任何位置都能写 */
    
    for (;;) {
        LOGI(TAG, "tick=%lu", HAL_GetTick());    /* 不会被 link error */
        HAL_Delay(10);
    }
}
```

**关键**：不要在 LOG_ENABLED=OFF 时手动 `#include "stm_log.h"` 外面加 `#if`——让它**永远 include**，让 `STM_LOG_ENABLED=0` 把宏变 no-op。

### ③ 不需要手动改 `stm_log_config.h`

上游 `stm_log` v2.3.1+ 已经包含 `#ifndef` 保护。FetchContent 拉到本地后即可使用。

---

## 触发方式

### IDE（STM32CubeIDE for VSCode）

1. 底部状态栏 CMake: [Debug ▼] → 切到 **Release**
2. Ctrl+Shift+P → `CMake: Delete Cache and Reconfigure`
3. Ctrl+Shift+B 构建

### 命令行

```bash
# 默认按 build type
cmake --build build/Debug      # LOG=ON
cmake --build build/Release    # LOG=OFF

# 手动覆盖
cmake -S . -B build/Debug -DLOG_ENABLED=OFF   # Debug 但不打印
cmake -S . -B build/Release -DLOG_ENABLED=ON  # Release 但带日志
```

---

## 预期 FLASH 对比

| 配置 | RAM | FLASH |
|---|---|---|
| Debug + LOG=ON（默认） | ~5.3 KB | ~27 KB |
| Debug + LOG=OFF + `-Os` | ~3 KB | ~18 KB |
| **Release + LOG=OFF（默认）** | **~2.9 KB** | **~13 KB** |
| Release + LOG=OFF + `-Os` | ~2.5 KB | ~11 KB |

---

## 验证步骤

### 1. 检查宏是否生效

```bash
# Release build 后的预处理结果应包含：
grep "STM_LOG_ENABLED" build/Release/CMakeFiles/stm32_pm3009_modbus.dir/main/app_main.cpp.obj.d 2>/dev/null
# 或直接看 .map 文件
cat build/Release/stm32_pm3009_modbus.map | grep -E "stm_log|SEGGER_RTT"
```

OFF 时应该看到 `stm_log_*` / `SEGGER_RTT_*` 函数**不在 .map**（被 GC）。

### 2. 验证 LOG_ENABLED 切换有效

```bash
# Debug build（默认 ON）
cmake -S . -B build/Debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/Debug
arm-none-eabi-size build/Debug/stm32_pm3009_modbus.elf
# 预期：~27 KB FLASH

# 切到 OFF
cmake -S . -B build/Debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DLOG_ENABLED=OFF
cmake --build build/Debug
arm-none-eabi-size build/Debug/stm32_pm3009_modbus.elf
# 预期：~18-22 KB FLASH（LOG 砍掉 ~5 KB）
```

### 3. 验证 .elf 里 stm_log 函数被 GC

```bash
arm-none-eabi-nm build/Debug/stm32_pm3009_modbus.elf | grep stm_log
# OFF 时应该为空
# ON 时应该看到 stm_log、stm_log_init_output、stm_log_hex 等
```

---

## 失败分流

| 现象 | 原因 | 修复 |
|---|---|---|
| `undefined reference to stm_log_init_output` | stm_log 版本 < v2.3.0 | 升级 `GIT_TAG v2.2.0` → `v2.3.1` |
| `'STM_LOG_ENABLED' redefined` 警告 | `stm_log_config.h` 没有 `#ifndef` 保护 | 升级到 v2.3.1+，或手动加保护 |
| `'LOGI' was not declared in this scope` | 没 `#include "stm_log.h"` | 在任何用 LOGx 的文件里 include |
| `redefinition of '_write'` | newlib 重定义 syscalls | 检查 `syscalls.c` 和 `Core/Src/syscalls.c` 是否有冲突 |
| Release 不省 FLASH | `-Os` 没生效 | CMakeCache 里 `CMAKE_C_FLAGS_RELEASE` 应该是 `-O3 -DNDEBUG` 或 `-Os -DNDEBUG` |

---

## 关键不变式

不管 `LOG_ENABLED` 是 ON 还是 OFF：

1. **`stm_log` 和 `SEGGER_RTT` 始终链入工程**（让 GC 处理）
2. **`stm_log.h` 始终 include**（让宏总是有定义）
3. **LOGx 宏永远可写**（OFF 时变空操作，编译不报错）
4. **`SEGGER_RTT_Init()` 等仍需 `#if LOG_ENABLED`**（OFF 时根本不调这些函数，链接器 GC 干净）

这条不变式让代码**不需要为 Release 改任何东西**——只要 CMake 切一下，FLASH 自动下来。