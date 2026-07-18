# CMake `add_subdirectory(main)` + `add_subdirectory(Lib/stm_log)` 集成方法

## 本 skill 改 / 不改的 CMake 文件

| 文件 | 是否改 | 原因 |
|------|--------|------|
| 根 `CMakeLists.txt` | ✅ 加 2 行：`add_subdirectory(Lib/stm_log)` + `add_subdirectory(main)` | CubeMX 不重写它 |
| `cmake/stm32cubemx/CMakeLists.txt` | ❌ | CubeMX 整段重写 |
| `cmake/` 下其他文件 | ❌ | 同上 |
| `Core/` 下任何 `.c` / `.h` | ❌ | 同上 |

CubeMX 重生成覆盖的文件：
- `Core/Src/{main,freertos,stm32f4xx_it,stm32f4xx_hal_msp,stm32f4xx_hal_timebase_tim}.c`
- `Core/Inc/*.h`、`Drivers/STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal_conf.h`
- `cmake/stm32cubemx/CMakeLists.txt`、`*.ioc`

CubeMX 不碰：`CMakeLists.txt` 根文件、所有用户自建目录（`main/` / `Lib/stm_log/` 等）。

→ 根 `CMakeLists.txt` 是业务层唯一的稳定立足点。

## `add_subdirectory(main)` 的语义

```cmake
# 根 CMakeLists.txt
add_subdirectory(cmake/stm32cubemx)   # CubeMX 生成的

include(FetchContent)
FetchContent_Declare(
    stm_log
    GIT_REPOSITORY https://github.com/NingZiXi/stm_log.git
    GIT_TAG        v2.2.0
)
FetchContent_MakeAvailable(stm_log)   # 联网环境自动 clone

# 或：add_subdirectory(Lib/stm_log)  # 手动 clone（离线 / 代理环境）

add_subdirectory(main)                # ← 本 skill 加的
```

等价于：

1. 进入 `main/`
2. 执行 `main/CMakeLists.txt`
3. 里面的 `target_sources` / `target_include_directories` / `target_link_libraries` 作用到顶层 `${CMAKE_PROJECT_NAME}`
4. `target_link_libraries(... stm_log)` 让 `stm_log` target（FetchContent 下载的源码 / Lib/stm_log/）被链入

它**不**创建独立目标，只切换作用域。

FetchContent 详细机制：首次 configure 时把库源码 clone 到 `<build>/_deps/stm_log-src/`，后续 configure 复用已下载内容；离线 / 代理环境下 clone 失败 → 改用方式 B 手动 git clone 到 `Lib/stm_log/`。

## 入口挂载点

### FreeRTOS

`Core/Src/freertos.c` → `StartDefaultTask`：

```c
void StartDefaultTask(void *argument) {
  /* USER CODE BEGIN 5 */
  app_main();
  /* USER CODE END 5 */
}
```

模板 `assets/app_main.c`：`osDelay` + `xPortGetFreeHeapSize` + `stm_log_init(&huart1, ...)`。

### 裸机

`Core/Src/main.c` → `USER CODE 2`（`MX_*_Init()` 全部完成后、`while (1)` 之前）：

```c
int main(void) {
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART1_UART_Init();

  /* USER CODE BEGIN 2 */
  app_main();          // ← 应用入口，跟其他初始化对齐
  /* USER CODE END 2 */

  while (1) { /* 兜底 */ }
}
```

`app_main()` 内部 `for(;;)` 死循环，永远不会返回到 `while`。`while (1)` 保留作为兜底（万一以后 `app_main` 改成可返回版本时仍能工作）。

模板 `assets/app_main_bare.c`：`HAL_Delay`，不依赖 FreeRTOS。

### 自动判定

多源多数：

| 信号 | FreeRTOS |
|------|---------|
| `Core/Src/freertos.c` 存在 | +1 |
| `Middlewares/Third_Party/FreeRTOS` 存在 | +1 |
| `Core/Src/main.c` 含 `MX_FREERTOS_Init` 或 `osKernelStart` | +1 |

≥2 → FreeRTOS 模板；否则裸机模板。

## 验证

```bash
cmake -S <root> -B <root>/build -DCMAKE_BUILD_TYPE=Debug
cmake --build <root>/build
```

正常输出 `Configuring done` + `Build files have been written to: ...`。

错误对照：
- `main/ not loaded by top-level CMake. Skipping app_main build.` → 根 `CMakeLists.txt` 没加 `add_subdirectory(main)`
- `Cannot find source file: stm_log.c` → `Lib/stm_log/` 没克隆或路径错
- `undefined reference to app_main` → 根 CMakeLists.txt 没加 `add_subdirectory(main)`
- `undefined reference to stm_log_init` → `main/CMakeLists.txt` 没 link `stm_log`（模板已加，手改时容易漏）
- `undefined reference to osDelay` / `xPortGetFreeHeapSize` → 错把 FreeRTOS 模板塞到裸机工程
- `undefined reference to HAL_Delay` → 错把裸机模板塞到 FreeRTOS 工程

## 常见误区

### 业务文件放 `Core/Src/` 里

可以但不建议：

- CubeMX 每次重生成按字母序重排 `MX_Application_Src`，diff 心烦
- CubeMX 会提示"检测到未管理的源文件"

正确做法：业务文件全部在 `main/` 下。

### 在 `cmake/stm32cubemx/CMakeLists.txt` 末尾追加业务

可以追加（CubeMX 不覆盖文件末尾某些位置），但本 skill 不这么做：

- 路径基准变成 `cmake/stm32cubemx/`，业务文件必须放那里
- 调试时要进 cmake/ 子目录看配置，定位绕
- `main/` 复用给其他工程时带着 cmake 路径，麻烦

### `main/CMakeLists.txt` 是 CubeMX 的一部分

不是。`main/` 完全是你自己创建的目录，CubeMX 不知道它的存在。

## stm_log 库升级

```bash
cd Lib/stm_log && git pull
```

升级到新版本直接重编译即可（库的 ABI 向后兼容）。

## FreeRTOS / CMSIS-OS 兼容

FreeRTOS 模板：
- `#include "cmsis_os.h"` 由 CubeMX 自动加入 `Core/Inc/`
- `main/CMakeLists.txt` 的 `target_include_directories` 把 `main/` 加入 include path
- 可正常调用所有 CMSIS-OS API

裸机模板：
- 只依赖 `main.h`（HAL）和自身
- 不 include `cmsis_os.h`
- 用 `HAL_Delay` 替代 `osDelay`