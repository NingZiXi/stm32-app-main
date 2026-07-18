---
name: stm32-app-main
description: 把 STM32CubeMX + CMake 工程改造成「main/ 子模块」结构：业务代码放到 main/，通过 add_subdirectory(main) 纳入构建，不修改 cmake/ 或 Core/ 下任何文件。支持 FreeRTOS 与裸机。日志组件用独立 stm_log 库，由 CMake FetchContent 自动 clone 到工程内 Lib/stm_log/（SOURCE_DIR 指定）。Trigger：用户说「初始化 app_main」「把 app_main 纳入工程」「业务代码独立到 main/」「cube-mx app_main 改造」「add_subdirectory(main)」「裸机改造」「bare metal」。
---

# STM32CubeMX 工程 → main/ 子模块改造

## 适用 / 不适用

- ✅ STM32CubeMX 6.x CMake 工程（根目录有 `*.ioc` + `cmake/stm32cubemx/CMakeLists.txt`）
- ✅ FreeRTOS 或裸机（bare metal）工程
- ✅ 联网 build 环境（stm_log 由 CMake FetchContent 自动下载）
- ❌ MDK / IAR / Makefile / CubeIDE（`.cproject`）工程 → 先用 CubeMX 重新生成一次 CMake
- ❌ 已经在 `main/` 里写过业务、要加新功能 → 直接改 `main/app_main.c`，不需要本 skill

## 改造前后

```
改造前：业务散落在 Core/Src/main.c 或 cmake/stm32cubemx/CMakeLists.txt
        ├── CMakeLists.txt
        ├── Core/Src/main.c                  # CubeMX 生成的 main()
        ├── cmake/stm32cubemx/CMakeLists.txt # MX_Application_Src 里有业务文件
        └── ...

改造后：业务统一到 main/，日志用独立 stm_log 库
        ├── CMakeLists.txt                   # +FetchContent_Declare(stm_log, SOURCE_DIR=Lib/stm_log) +add_subdirectory(main)
        ├── Core/Src/main.c                  # CubeMX 生成的 main()，调用 app_main()
        ├── cmake/stm32cubemx/CMakeLists.txt # 不动
        ├── Lib/                             # 新增（FetchContent 把源码拉到这里）
        │   └── stm_log/                     # clone 自 https://gitee.com/nzxhg/stm_log @ v2.2.0
        ├── main/                            # 新增
        │   ├── CMakeLists.txt
        │   └── app_main.c
        └── ...
```

## 工作流（6 步）

### §0 探测

```bash
test -f <root>/*.ioc && echo OK || echo NO_IOC
test -f <root>/cmake/stm32cubemx/CMakeLists.txt && echo OK || echo NO_CMAKE_MODE
grep -q "add_subdirectory(cmake/stm32cubemx)" <root>/CMakeLists.txt && echo OK || echo NO_SUB
test -d <root>/main && echo EXISTS || echo NEW

# FreeRTOS vs 裸机（多源多数胜出）
test -f <root>/Core/Src/freertos.c && echo FREERTOS_HAS_FREERTOS_C
test -d <root>/Middlewares/Third_Party/FreeRTOS && echo FREERTOS_HAS_MW
grep -q "MX_FREERTOS_Init\|osKernelStart" <root>/Core/Src/main.c 2>/dev/null && echo FREERTOS_HAS_MAIN_INIT

# stm_log 库（FetchContent 拉到 <root>/Lib/stm_log/）
test -d <root>/Lib/stm_log && echo STM_LOG_LOCAL_OVERRIDE || echo STM_LOG_FETCH
```

判定：

| 信号 | 处理 |
|------|------|
| `NO_IOC` 或 `NO_CMAKE_MODE` | 中断：请用户先跑 CubeMX 生成 CMake |
| `EXISTS` 且非空 | 警告：列出文件，让用户选覆盖 / 跳过 / 合并 |
| 至少 2 个 `FREERTOS_HAS_*` | FreeRTOS 工程 → 用 `assets/app_main.c` |
| 全部 `FREERTOS_NO_*` | 裸机工程 → 用 `assets/app_main_bare.c` |

### §1 stm_log 依赖（FetchContent 拉取到工程内 `Lib/stm_log/`）

stm_log 库提供 `LOGI/LOGW/LOGE/...` 宏。**首次 build 时 CMake FetchContent 自动从 https://gitee.com/nzxhg/stm_log clone 到 `<root>/Lib/stm_log/`**（不是 `<build>/_deps/`），版本锁定 `v2.2.0`。源码落在工程目录内，便于 IDE 索引 / 版本管理 / 离线复用。

> GitHub 镜像：https://github.com/NingZiXi/stm_log（境外或 GitHub 直连环境下用）；改 `GIT_REPOSITORY` 那行即可，tag 与 commit 都同步。
>
> 如需离线 / 代理环境：手动 `git clone https://gitee.com/nzxhg/stm_log <root>/Lib/stm_log`，FetchContent 检测到目录已存在则跳过拉取。

### §2 写入 main/

| 源 | 目标 |
|----|------|
| `assets/CMakeLists.txt` | `<root>/main/CMakeLists.txt` |
| `assets/app_main.c` 或 `app_main_bare.c`（按 §0） | `<root>/main/app_main.c` |

⚠ 不再写 `app_log.h/c`（v1 的内置日志已废弃，统一走 stm_log 库）。

模板 `app_main.c` 关键改动：
```c
#include "stm_log.h"                          // ← 取代 #include "app_log.h"
static const char *TAG = "main";

void app_main(void) {
    stm_log_init(&huart1, STM_LOG_LVL_INFO);  // ← 新增：绑定 log UART + level
    LOGI(TAG, "Boot. Heap=%u", (unsigned)xPortGetFreeHeapSize());
    ...
}
```

⚠ **stm_log 宏使用约束**：`fmt` 必须是字符串字面量；`tag` 可为变量或字面量；依赖 GCC `##__VA_ARGS__`，所以工程需用 `gnu11` 或更高（STM32CubeMX 默认 GCC 12 + `-std=gnu11` 已满足）。

⚠ **stm_log 库默认 UART**：`stm_log_init(&huart1, ...)` 假设你的工程 UART1 是调试串口。如果用其他 UART，改成对应 huart。

⚠ **`huart1` 的来源（兼容两种 CubeMX 代码生成模式）**：模板顶部自带 `extern UART_HandleTypeDef huart1;`，**不要** `#include "usart.h"`。原因：CubeMX 在 Project Settings → Code Generator 下有一个开关 "Generate peripheral initialization as a pair of '.c/.h' files per peripheral"——
- **开启**：`huart1` 在 `usart.h` 里 extern（分离模式），我们的 extern 多余但合法
- **关闭**：`huart1` 是 `main.c` 的文件作用域变量，不在任何 `.h` 里（合并模式），模板自带的 extern 是唯一引用入口

所以模板自带 extern 是为了同时兼容这两种模式，无需根据用户工程判断。

⚠ **Release build 关 log**：在根 `CMakeLists.txt` 加：
```cmake
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE STM_LOG_ENABLED=0)
endif()
```

⚠ 不动 `main/` 里已有的 `lwesp_opts.h` / `lwesp_ll_stm32f407.c` 等 LwESP 文件。

### §3 改根 CMakeLists.txt

在 `add_subdirectory(cmake/stm32cubemx)` 后面加：

```cmake
include(FetchContent)
FetchContent_Declare(
    stm_log
    # 默认走 Gitee 镜像（国内访问快）；如需 GitHub，把下一行注释掉、放开下一行的下一行
    GIT_REPOSITORY https://gitee.com/nzxhg/stm_log.git
    #GIT_REPOSITORY https://github.com/NingZiXi/stm_log.git     # 备选：境外 / GitHub 直连
    GIT_TAG        v2.2.0
    SOURCE_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/Lib/stm_log   # 关键：落到工程内的 Lib/stm_log/
)
FetchContent_MakeAvailable(stm_log)

add_subdirectory(main)                      # 业务入口
```

关键点是 `SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Lib/stm_log`：FetchContent 会把源码 clone 到 `<root>/Lib/stm_log/` 而不是默认的 `<build>/_deps/stm_log-src/`。版本锁定 `v2.2.0`。如果 `Lib/stm_log/` 已存在，FetchContent 会跳过拉取直接复用。

**离线 / 代理环境**：手动 `git clone https://gitee.com/nzxhg/stm_log <root>/Lib/stm_log` 后，FetchContent 自动跳过拉取走复用流程。

`main/CMakeLists.txt` 会自动 link `stm_log` target（在模板里改好了）。

**唯一**会改的 CMake 文件。

### §4 接入口

#### A. FreeRTOS

`Core/Src/freertos.c` → `StartDefaultTask`：

```c
void StartDefaultTask(void *argument) {
  /* USER CODE BEGIN 5 */
  app_main();
  /* USER CODE END 5 */
}
```

#### B. 裸机

`Core/Src/main.c` → `USER CODE 2`（在 `MX_*_Init()` 全部完成后、`while (1)` 之前）：

```c
int main(void) {
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART1_UART_Init();

  /* USER CODE BEGIN 2 */
  app_main();          // ← 这里
  /* USER CODE END 2 */

  while (1) { /* 空，留给中断 / 后台 */ }
}
```

`app_main()` 内部 `for(;;)` 死循环，永远不会返回到 main 里的 `while`。`while (1)` 保留作为兜底（万一以后 `app_main` 改成可返回版本时仍能工作）。

两种工程下 `app_main()` 都**永不返回**。

### §5 业务文件搬迁（用户手动）

**本 skill 不改 `cmake/` 也不改 `Core/`**。如果用户老工程里业务文件散落在 `Core/Src/` 下：

> 请手动 `git mv` 把它们移到 `main/`，然后在 `main/CMakeLists.txt` 的 `target_sources` 里加上。

### §6 验证

```bash
cmake -S <root> -B <root>/build -DCMAKE_BUILD_TYPE=Debug
cmake --build <root>/build
```

### §7 业务代码注释规范

skill 生成的所有业务文件**严格按用户注释规范**，详见 [references/code-comment-style.md](references/code-comment-style.md)。要点速览：

| 项 | 规则 |
|---|------|
| 文件头 | `@file / @author / @brief / @date / @copyright` 五件套；`app_main.c` 额外 `@version 0.1` |
| 函数注释 | `@brief` 写**做了什么 + 约束/副作用**；`@param` 写语义不机械重复参数名；`@return` 有返回值时写 |
| 宏 / 全局变量 | 注释**同行尾**，与右值或类型对齐 |
| 行内注释 | 单行 `//`，写**为什么 / 注意点**，不重复代码语义 |
| 红线 | 不写教学型多行注释；不用 Doxygen 重型标记（`@defgroup` / `@code` 等）；不机械翻译 `@param` |

模板 [`assets/app_main.c`](assets/app_main.c) / [`assets/app_main_bare.c`](assets/app_main_bare.c) 已按此规范实现；用户在 `main/` 下加新文件时请继续遵守。注释"为什么这样设计"的答案一律放在 [references/code-comment-style.md](references/code-comment-style.md) / 用户的 memory 中，**不放代码注释里**。

## 失败分流

| 现象 | 根因 | 处理 |
|------|------|------|
| `add_subdirectory given source ... which is not a directory` | `main/CMakeLists.txt` 没生成 | 重跑 §2 |
| `Failed to clone ... gitee.com/nzxhg/stm_log.git` | Gitee / 代理异常 | 手动 `git clone https://gitee.com/nzxhg/stm_log <root>/Lib/stm_log`，FetchContent 检测到目录已存在会自动跳过拉取；或切回 GitHub：把 `GIT_REPOSITORY` 改 `https://github.com/NingZiXi/stm_log.git` |
| `Lib/stm_log/ is not empty` / FetchContent 跳过 clone 后报红 | 手动 clone 的目录不是干净仓库（如带 `.git/` 之外的文件） | 清空 `Lib/stm_log/` 或 `rm -rf Lib/stm_log && git clone ...` 重来 |
| `undefined reference to stm_log_init` | `main/CMakeLists.txt` 没 link `stm_log` | 检查模板（应该自动 link） |
| `undefined reference to app_main` | 根 CMakeLists.txt 没加 `add_subdirectory(main)` | 重跑 §3 |
| `undefined reference to osDelay` / `xPortGetFreeHeapSize` | 用错模板：FreeRTOS 模板跑到了裸机工程 | 重跑 §0 探测 + §2 选对模板 |
| `undefined reference to HAL_Delay` | 裸机模板跑到了 FreeRTOS 工程 | 同上 |
| `app_main` 跑了但系统卡死 | `app_main` 没有死循环，或挂载点错 | 确认模板 `for(;;)` 存在 + 挂载点选 A/B 正确 |
| LOG 输出乱码 / 全 0 | `stm_log_init(&huart1, ...)` 的 huart 跟实际接线不一致 | 改成实际调试 UART 的 handle |
| CubeMX 重生成后 main/ 消失 | 不可能 — main/ 不在 CubeMX 管理范围 | 检查 `.gitignore` 是否误屏蔽 |
| `Lib/stm_log/` 在 `.gitignore` 里被忽略 | 用户把 `Lib/` 当成 build 产物 | 把 `Lib/stm_log/` 加入版本控制（或单独 `.gitignore` 例外） |

## 详细参考

- [references/CMake-integration.md](references/CMake-integration.md) — `add_subdirectory` 方法论、与 CubeMX 重生成的兼容性
- [references/code-comment-style.md](references/code-comment-style.md) — 业务代码注释规范（文件头 Doxygen 模板 / 函数 `@brief` / 同行尾注释 / 红线）
- [assets/CMakeLists.txt](assets/CMakeLists.txt) — `main/CMakeLists.txt` 模板（自动 link stm_log）
- [assets/app_main.c](assets/app_main.c) — FreeRTOS 版入口
- [assets/app_main_bare.c](assets/app_main_bare.c) — 裸机版入口
- 外部库：https://gitee.com/nzxhg/stm_log（GitHub 镜像：https://github.com/NingZiXi/stm_log）

## 交接

- 加 Wi-Fi / MQTT / ESP-AT → 引导看 `main/lwesp_opts.h` 等 LwESP 集成
- 改 CubeMX 配置 / 加 BSP → `stm32-hal-development`
- 构建 / 烧录 → `build-cmake` / `flash-openocd`
- 日志用法 / 改输出后端（RTT / SWO） → https://gitee.com/nzxhg/stm_log/blob/main/README.md（GitHub：https://github.com/NingZiXi/stm_log/blob/main/README.md）