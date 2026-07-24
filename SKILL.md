---
name: stm32-app-main
description: 把 STM32CubeMX + CMake 工程改造成「main/ 子模块」结构：业务代码放到 main/，通过 add_subdirectory(main) 纳入构建，不修改 cmake/ 或 Core/ 下任何文件。支持 FreeRTOS 与裸机。日志组件用独立 stm_log 库（v2.3.0+,stm_log_init_output 推荐 API），由 CMake FetchContent 自动 clone 到工程内 Lib/stm_log/（SOURCE_DIR 指定）。日志后端可在 UART 与 SEGGER RTT 之间选择：UART 走 stm_log 默认输出；RTT 走 FetchContent 拉的精简版 SEGGER RTT（在用户 fork 镜像仓库里加一份 CMakeLists_rtt.txt 把 segger_rtt target 包装出来），main/CMakeLists.txt 通过 if(TARGET segger_rtt) 条件 link。配套 .vscode/launch.json + .vscode/tasks.json 实现 VSCode + Cortex-Debug + J-Link F5 一键 build+烧录+RTT。Trigger：用户说「初始化 app_main」「把 app_main 纳入工程」「业务代码独立到 main/」「cube-mx app_main 改造」「add_subdirectory(main)」「裸机改造」「bare metal」「用 RTT 输出日志」「RTT 后端」「J-Link RTT」「SEGGER RTT 配 CMake」「stm_log_init_output」「F5 一键调试」「VSCode 调试 J-Link」。
---

# STM32CubeMX 工程 → main/ 子模块改造

## 适用 / 不适用

- ✅ STM32CubeMX 6.x CMake 工程（根目录有 `*.ioc` + `cmake/stm32cubemx/CMakeLists.txt`）
- ✅ FreeRTOS 或裸机（bare metal）工程
- ✅ UART 后端 — 联网 build 环境（stm_log 由 CMake FetchContent 自动下载）
- ✅ RTT 后端 — 通过 FetchContent 自动拉 SEGGER RTT 到 `Lib/segger_rtt/`（详见 §1.6 + [references/rtt-setup.md](references/rtt-setup.md)）
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
        │   └── stm_log/                     # clone 自 https://gitee.com/nzxhg/stm_log @ v2.3.1
        ├── main/                            # 新增
        │   ├── CMakeLists.txt
        │   └── app_main.c
        └── ...

改造后 — RTT 分支（§1.5 选 RTT 时）：
        ├── ...同上...
        ├── Lib/
        │   ├── stm_log/                     # 同上
        │   └── segger_rtt/                  # 新增，手动 git clone（见 references/rtt-setup.md）
        │       ├── RTT/
        │       │   ├── SEGGER_RTT.c
        │       │   ├── SEGGER_RTT.h
        │       │   ├── SEGGER_RTT_ASM_ARMv7M.S
        │       │   └── SEGGER_RTT_ConfDefaults.h
        │       └── Config/SEGGER_RTT_Conf.h
        ├── main/
        │   ├── CMakeLists.txt               # 用 assets/CMakeLists.txt 通用版（UART/RTT 都用它,内含 if(TARGET segger_rtt) 条件 link）
        │   └── app_main.c                   # 用 assets/app_main_*_rtt.c（SEGGER_RTT_Init + rtt_output）
        └── ...
```

## 工作流（8 步）

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
| 至少 2 个 `FREERTOS_HAS_*` | FreeRTOS 工程 → 用 `assets/app_main.c` 或 `app_main_rtt.c` |
| 全部 `FREERTOS_NO_*` | 裸机工程 → 用 `assets/app_main_bare.c` 或 `app_main_bare_rtt.c` |

> §0 只决定 **FreeRTOS / 裸机** 走哪个模板族,**日志后端(UART / RTT)** 在 §1.5 单独询问。

### §1 stm_log 依赖（FetchContent 拉取到工程内 `Lib/stm_log/`）

stm_log 库提供 `LOGI/LOGW/LOGE/...` 宏。**首次 build 时 CMake FetchContent 自动从 https://gitee.com/nzxhg/stm_log clone 到 `<root>/Lib/stm_log/`**（不是 `<build>/_deps/`），版本锁定 `v2.3.1`。源码落在工程目录内，便于 IDE 索引 / 版本管理 / 离线复用。

> GitHub 镜像：https://github.com/NingZiXi/stm_log（境外或 GitHub 直连环境下用）；改 `GIT_REPOSITORY` 那行即可，tag 与 commit 都同步。
>
> 如需离线 / 代理环境：手动 `git clone https://gitee.com/nzxhg/stm_log <root>/Lib/stm_log`，FetchContent 检测到目录已存在则跳过拉取。

### §1.5 询问日志后端（关键决策，影响 §2 / §3 模板选择）

> 默认走 **UART**（大多数 STM32 工程习惯）；如果工程以 J-Link / Ozone 调试为主,选 **RTT** 更顺手（F5 调试实时出 log,不占 UART、不需要 USB CDC）。

AskUserQuestion 询问,选项：

| 选项 | 后续路径 |
|------|----------|
| **UART（默认）** | §2A UART 模板：`app_main.c` / `app_main_bare.c`（**通用** `CMakeLists.txt`，只 link `stm_log`） |
| **SEGGER RTT** | §2B RTT 模板：`app_main_rtt.c` / `app_main_bare_rtt.c`（**通用** `CMakeLists.txt`，内含 `if(TARGET segger_rtt)` 条件 link） |

> **RTT 模板不需要 UART**：`stm_log_init_output(rtt_output, ...)` 一步完成 init + 切后端（stm_log v2.3.0+），不绑 UART；切回 UART 把那一行换成 `stm_log_init(&huart1, STM_LOG_LVL_INFO)` 即可。UART 模板仍走 `stm_log_init(&huart1, ...)`。
>
> **stm_log v2.3.1 新增可配置 HAL 头文件**：通过 `STM_LOG_HAL_HEADER` 在工程 CMake 中指定所属 STM32 家族的 HAL 头文件名（默认 `stm32f4xx_hal.h`）。详见 [references/stm-log-config.md](references/stm-log-config.md)。

### §1.6 RTT 分支专属：SEGGER RTT 库(关注点分离,FetchContent + 用户 fork 仓库提供 CMakeLists_rtt.txt)

SEGGER RTT 走与 stm_log 同模式的 FetchContent 路径 —— 根 `CMakeLists.txt` `FetchContent_Declare(segger_rtt, SOURCE_DIR <root>/Lib/segger_rtt)` 把源码 clone 到工程内;**首次 build 自动拉**(Gitee / GitHub 镜像二选一,默认 Gitee)。

但是 —— **关键的设计点**:

- **用户的 fork 仓库需要精简**(只保留 RTT/Config/LICENSE,删 Examples/Syscalls/.pdsc/gen_pack.sh)
- **用户的 fork 仓库需要加一份 `CMakeLists_rtt.txt`**(包装成 `segger_rtt` target)

这样 FetchContent 拉到的 `Lib/segger_rtt/` 自带 `CMakeLists_rtt.txt`,工程根 CMakeLists 用 `include(Lib/segger_rtt/CMakeLists_rtt.txt)` 引用它,定义 `segger_rtt` target 给 `main/CMakeLists.txt` link。

> 关注点分离:`main/CMakeLists.txt` 只 link `segger_rtt` target,不管 RTT 源路径;RTT 源全在 `Lib/segger_rtt/` 里。

**用户第一次用 skill 改造工程的准备**(只一次):

1. **精简 fork**:在自己的 fork 仓库 `nzxhg/RTT.git` 或 `NingZiXi/RTT` 删 Examples/、Syscalls/、SEGGER.RTT.pdsc、gen_pack.sh,commit + push
2. **加 CMakeLists_rtt.txt**:复制工程里的 `Lib/segger_rtt/CMakeLists_rtt.txt` 内容(包含 `add_library(segger_rtt STATIC ...)` + 头文件路径)到 fork 仓库根,commit + push
3. **(可选)在 fork 仓库打 tag**:跟 SEGGER 上游对齐 `V8.58`(对应 SHA `4d8feab3150f86f37a9d323ddc88d6cdf5673072`),让 FetchContent 用 tag 而非分支锁版本

**镜像没 V8.58 tag 时**:`GIT_TAG V8.58` 改成 `4d8feab3150f86f37a9d323ddc88d6cdf5673072`(SHA 锁定)。

**离线 / 代理环境**:手动 `git clone https://gitee.com/nzxhg/RTT.git <root>/Lib/segger_rtt` 把 fork 仓库克隆下来,FetchContent 检测到目录已存在则跳过拉取。

详细路径约定 + 验证命令 + `CMakeLists_rtt.txt` 标准模板见 [references/rtt-setup.md](references/rtt-setup.md)。

### §2 写入 main/

按 §1.5 选的后端走 A 或 B。

#### §2A UART 分支

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

⚠ **stm_log 库默认 UART**：`stm_log_init(&huart1, ...)` 假设你的工程 UART1 是调试串口。如果用其他 UART，改成对应 huart。**stm_log v2.3.0+** 新增 `stm_log_init_output(cb, level)` 一步完成 init + 装 callback；RTT/SWO/USB CDC 等非 UART 后端推荐用新 API（不传 UART）。
>
> ⚠ **stm_log v2.3.1 新增可配置 HAL 头文件**：通过 `STM_LOG_HAL_HEADER` 在工程 CMake 中指定所属 STM32 家族的 HAL 头文件名（默认 `stm32f4xx_hal.h`）。STM32G0/STM32H7/STM32U5 等非 F4 工程必须显式覆盖。详见 [references/stm-log-config.md](references/stm-log-config.md)。

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

#### §2B RTT 分支

| 源 | 目标 |
|----|------|
| `assets/CMakeLists.txt`(通用版,UART/RTT 共用) | `<root>/main/CMakeLists.txt` |
| `assets/app_main_rtt.c` 或 `app_main_bare_rtt.c`(按 §0) | `<root>/main/app_main.c` |

> **通用模板**：`assets/CMakeLists.txt` 一个文件覆盖 UART 和 RTT 两种情况，靠 `if(TARGET segger_rtt)` 条件 link。UART 分支下 `segger_rtt` target 不存在,跳过;RTT 分支下 target 存在,自动 link。
>
> **segger_rtt target 来源**：`Lib/segger_rtt/CMakeLists_rtt.txt`(用户 fork 仓库自带,FetchContent 拉 SEGGER RTT 时一起拉下来)。

模板 `app_main_rtt.c` 关键改动（相对 UART 版本）：
```c
#include "stm_log.h"
#include "SEGGER_RTT.h"                                            // ← RTT 后端

static void rtt_output(const char *buf, uint16_t len) {
    SEGGER_RTT_Write(0, buf, len);                                 // channel 0 = 默认 terminal
}

void app_main(void) {
    SEGGER_RTT_Init();                                             // ← 第一件事：建 RTT control block
    stm_log_init_output(rtt_output, STM_LOG_LVL_INFO);             // ← v2.3.0+：一步完成 init + 切 RTT，跳过 UART 绑定
    ...
}
```

⚠ **顺序铁律**：`SEGGER_RTT_Init()` 必须在 `stm_log_init_output(...)` **之前**。否则回调指向未初始化的 control block，LOGx 写入全丢。

⚠ **stm_log 版本要求**：RTT 模板用 `stm_log_init_output()`，要求 stm_log **v2.3.0+**。如果你工程的 FetchContent 拉的是 v2.2.0（旧），模板会编译失败（`undefined reference to stm_log_init_output`）。两种解法：
- 把根 `CMakeLists.txt` 的 `GIT_TAG v2.2.0` 改成 `v2.3.1`（推荐）
- 或者临时回退到 UART 模板：`stm_log_init(&huart1, level)` + `stm_log_set_output(rtt_output)`（v2.2.0 兼容）

⚠ **关注点分离**：`main/CMakeLists.txt` 不加 RTT 源、不加 RTT include path —— RTT 源 + 头文件路径全部由 `Lib/segger_rtt/CMakeLists_rtt.txt`(用户 fork 仓库自带)封装到 `segger_rtt` target,main 只 link target。

⚠ **SEGGER RTT 库就位检查**：§1.6 由 FetchContent 自动处理（前提是用户 fork 镜像仓库里有 `CMakeLists_rtt.txt`）。本节假设 `Lib/segger_rtt/RTT/SEGGER_RTT.c` 等文件已由 FetchContent 拉到。

⚠ **RTT 切回 UART**：把 `app_main.c` 里 `stm_log_init_output(rtt_output, ...)` 那行换成 `stm_log_init(&huart1, STM_LOG_LVL_INFO)`，再加 `#include "usart.h"`（如果还没 extern huart1）。或者保持 callback 不变，单纯切 UART：`stm_log_set_output(NULL)` 强制回默认（需要 huart1 已经 init）。

### §3 改根 CMakeLists.txt

在 `add_subdirectory(cmake/stm32cubemx)` 后面加：

```cmake
include(FetchContent)
FetchContent_Declare(
    stm_log
    # 默认走 Gitee 镜像（国内访问快）；如需 GitHub，把下一行注释掉、放开下一行的下一行
    GIT_REPOSITORY https://gitee.com/nzxhg/stm_log.git
    #GIT_REPOSITORY https://github.com/NingZiXi/stm_log.git     # 备选：境外 / GitHub 直连
    GIT_TAG        v2.3.1
    SOURCE_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/Lib/stm_log   # 关键：落到工程内的 Lib/stm_log/
)
FetchContent_MakeAvailable(stm_log)

add_subdirectory(main)                      # 业务入口
```

关键点是 `SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Lib/stm_log`：FetchContent 会把源码 clone 到 `<root>/Lib/stm_log/` 而不是默认的 `<build>/_deps/stm_log-src/`。版本锁定 `v2.3.1`。如果 `Lib/stm_log/` 已存在，FetchContent 会跳过拉取直接复用。

**离线 / 代理环境**：手动 `git clone https://gitee.com/nzxhg/stm_log <root>/Lib/stm_log` 后，FetchContent 自动跳过拉取走复用流程。

`main/CMakeLists.txt` 会自动 link `stm_log` target;RTT 分支还 link `segger_rtt`(条件 link,见模板)。

**唯一**会改的 CMake 文件。

> **RTT 分支**：根 CMakeLists 加 `FetchContent_Declare(segger_rtt, ...)` + `FetchContent_MakeAvailable(segger_rtt)` + `include(Lib/segger_rtt/CMakeLists_rtt.txt)`。

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

### §6.5 VSCode + Cortex-Debug 调试配置(可选)

> **适用**：你用 **VSCode + Cortex-Debug 扩展** + **J-Link** 在本地调试。其他 IDE(Ozone / CubeIDE / STM32CubeIDE for VSCode / IAR / Keil)跳过本步。

| 源 | 目标 |
|----|------|
| `assets/tasks.json` | `<root>/.vscode/tasks.json` |
| `assets/launch.json` | `<root>/.vscode/launch.json` |

⚠ **`serverpath` 必须改成你本机 `JLinkGDBServerCL.exe` 路径**,模板默认 `D:/SEGGER/JLink/JLinkGDBServerCL.exe`(用户机器实际位置)。

⚠ **`device`** 必须改成你的 MCU 型号(模板默认 `STM32F407VE`,匹配 STM32F407VET6 / LQFP100)。

按 F5 后行为:`build-debug` task 先跑 `cmake --build build/Debug` → 启 `JLinkGDBServerCL.exe` 烧录 + reset → 停 main → RTT Console 自动显示 `LOGx` 输出。RTT 后端详情见 [references/rtt-setup.md](references/rtt-setup.md)。

### §7 业务代码注释规范

skill 生成的所有业务文件**严格按用户注释规范**，详见 [references/code-comment-style.md](references/code-comment-style.md)。要点速览：

| 项 | 规则 |
|---|------|
| 文件头 | `@file / @author / @brief / @date / @copyright` 五件套；`app_main.c` 额外 `@version 0.1` |
| 函数注释 | `@brief` 写**做了什么 + 约束/副作用**；`@param` 写语义不机械重复参数名；`@return` 有返回值时写 |
| 宏 / 全局变量 | 注释**同行尾**，与右值或类型对齐 |
| 行内注释 | 单行 `//`，写**为什么 / 注意点**，不重复代码语义 |
| 红线 | 不写教学型多行注释；不用 Doxygen 重型标记（`@defgroup` / `@code` 等）；不机械翻译 `@param` |

模板 [`assets/app_main.c`](assets/app_main.c) / [`assets/app_main_bare.c`](assets/app_main_bare.c)（UART 后端）与 [`assets/app_main_rtt.c`](assets/app_main_rtt.c) / [`assets/app_main_bare_rtt.c`](assets/app_main_bare_rtt.c)（RTT 后端）已按此规范实现；用户在 `main/` 下加新文件时请继续遵守。注释"为什么这样设计"的答案一律放在 [references/code-comment-style.md](references/code-comment-style.md) / 用户的 memory 中，**不放代码注释里**。

## 失败分流

| 现象 | 根因 | 处理 |
|------|------|------|
| `add_subdirectory given source ... which is not a directory` | `main/CMakeLists.txt` 没生成 | 重跑 §2 |
| `Failed to clone ... gitee.com/nzxhg/stm_log.git` | Gitee / 代理异常 | 手动 `git clone https://gitee.com/nzxhg/stm_log <root>/Lib/stm_log`，FetchContent 检测到目录已存在会自动跳过拉取；或切回 GitHub：把 `GIT_REPOSITORY` 改 `https://github.com/NingZiXi/stm_log.git` |
| `Lib/stm_log/ is not empty` / FetchContent 跳过 clone 后报红 | 手动 clone 的目录不是干净仓库（如带 `.git/` 之外的文件） | 清空 `Lib/stm_log/` 或 `rm -rf Lib/stm_log && git clone ...` 重来 |
| `Failed to clone ... gitee.com/nzxhg/RTT.git` | Gitee / 代理异常 | 切回 GitHub：把 `GIT_REPOSITORY` 改 `https://github.com/NingZiXi/RTT.git`；或手动 `git clone https://gitee.com/nzxhg/RTT.git <root>/Lib/segger_rtt` 让 FetchContent 跳过 |
| RTT 镜像没 `V8.58` tag | Gitee 镜像同步时没 fetch tag | 把 `GIT_TAG V8.58` 改成 SHA `4d8feab3150f86f37a9d323ddc88d6cdf5673072`；或在自己镜像上 `git push origin V8.58` 同步 tag |
| `undefined reference to stm_log_init` | `main/CMakeLists.txt` 没 link `stm_log` | 检查模板（应该自动 link） |
| `undefined reference to app_main` | 根 CMakeLists.txt 没加 `add_subdirectory(main)` | 重跑 §3 |
| `undefined reference to osDelay` / `xPortGetFreeHeapSize` | 用错模板：FreeRTOS 模板跑到了裸机工程 | 重跑 §0 探测 + §2 选对模板 |
| `undefined reference to HAL_Delay` | 裸机模板跑到了 FreeRTOS 工程 | 同上 |
| `app_main` 跑了但系统卡死 | `app_main` 没有死循环，或挂载点错 | 确认模板 `for(;;)` 存在 + 挂载点选 A/B 正确 |
| LOG 输出乱码 / 全 0 | `stm_log_init(&huart1, ...)` 的 huart 跟实际接线不一致 | 改成实际调试 UART 的 handle |
| CubeMX 重生成后 main/ 消失 | 不可能 — main/ 不在 CubeMX 管理范围 | 检查 `.gitignore` 是否误屏蔽 |
| `Lib/stm_log/` 在 `.gitignore` 里被忽略 | 用户把 `Lib/` 当成 build 产物 | 把 `Lib/stm_log/` 加入版本控制（或单独 `.gitignore` 例外） |
| `undefined reference to SEGGER_RTT_Init` | `segger_rtt` target 没链进工程；常见原因：用户 fork 镜像仓库没 `CMakeLists_rtt.txt`,或 root CMakeLists 漏 `include(Lib/segger_rtt/CMakeLists_rtt.txt)` | 检查 fork 镜像 `CMakeLists_rtt.txt` 是否存在 + root CMakeLists `include` 行 |
| `fatal error: SEGGER_RTT.h: No such file` | `CMakeLists_rtt.txt` 里 `target_include_directories` 用了 `CMAKE_CURRENT_SOURCE_DIR`(include() 不切这个变量) | 用 `CMAKE_CURRENT_LIST_DIR`,见 [references/rtt-setup.md](references/rtt-setup.md) |
| `fatal error: SEGGER_RTT_Conf.h: No such file` | 同上 | 同上 |
| `Cannot find source file: RTT/SEGGER_RTT.c`(`add_library` 报错) | `CMakeLists_rtt.txt` 里 `add_library` 用了 `RTT/SEGGER_RTT.c`(相对路径) | 改用 `${CMAKE_CURRENT_LIST_DIR}/RTT/SEGGER_RTT.c`,见 [references/rtt-setup.md](references/rtt-setup.md) |
| F5 → `Failed to launch gdb-server` | `launch.json` 里 `serverpath` 路径错（不是 `JLinkGDBServerCL.exe` 实际位置） | 改 `serverpath` 指向 `JLinkGDBServerCL.exe` 实际路径（Windows 通常 `D:\SEGGER\JLink\`） |
| F5 → `Device 'XXX' not found` | `launch.json` 里 `device` 名错（J-Link DLL 内部 ID,大小写敏感） | 改成 J-Link Commander `?` 命令查到的精确名字（如 `STM32F407VE`） |
| F5 → `Couldn't find launch target` | CMakeTools 没识别 build target | 打开 CMakeTools 面板手动 Build 一次,或 `cmake --preset Debug` 跑过 |
| F5 → 没 RTT log（RTT Viewer 看到,Console 空白） | `launch.json` `rttConfig` 配错 | `rttConfig.enabled: true` + `address: "auto"` + `decoders` schema 是 `{label, port, type:"console"}` 不是 `name/mode` |
| F5 看不到 RTT log（PC 端） | 固件没调 `stm_log_init_output(rtt_output, ...)`，或顺序错（`SEGGER_RTT_Init` 在 `init_output` 之后） | 检查 `app_main()` 顺序：Init → init_output |
| `undefined reference to stm_log_init_output` | RTT 模板要求 stm_log **v2.3.0+**，工程 FetchContent 还停在 v2.2.0 | 根 `CMakeLists.txt` 把 `GIT_TAG v2.2.0` 改成 `v2.3.1`，`rm -rf Lib/stm_log && cmake --preset Debug` 重拉 |
| RTT log 在 JLinkRTTViewer 看得到，VSCode RTT Console 空白 | `.vscode/launch.json` `rttConfig` 配错 | `rttConfig.enabled: true` + `address: "auto"` + `decoders` schema 正确 |

## 详细参考

- [references/CMake-integration.md](references/CMake-integration.md) — `add_subdirectory` 方法论、与 CubeMX 重生成的兼容性
- [references/stm-log-config.md](references/stm-log-config.md) — `STM_LOG_HAL_HEADER`（HAL 家族头） / `STM_LOG_LINK_CUBEMX` 配置
- [references/code-comment-style.md](references/code-comment-style.md) — 业务代码注释规范（文件头 Doxygen 模板 / 函数 `@brief` / 同行尾注释 / 红线）
- [references/rtt-setup.md](references/rtt-setup.md) — SEGGER RTT 库就位步骤（GitHub clone / 离线 ZIP / SES 安装目录）+ 路径约定 + 验证命令
- [assets/CMakeLists.txt](assets/CMakeLists.txt) — `main/CMakeLists.txt` 模板，通用版（UART 仅 link stm_log；RTT 额外条件 link `segger_rtt` target）
- [assets/app_main.c](assets/app_main.c) — FreeRTOS UART 版入口
- [assets/app_main_bare.c](assets/app_main_bare.c) — 裸机 UART 版入口
- [assets/app_main_rtt.c](assets/app_main_rtt.c) — FreeRTOS RTT 版入口（含 SEGGER_RTT_Init + rtt_output + set_output）
- [assets/app_main_bare_rtt.c](assets/app_main_bare_rtt.c) — 裸机 RTT 版入口
- [assets/launch.json](assets/launch.json) — VSCode + Cortex-Debug + J-Link 调试配置（servertype: jlink + rttConfig.enabled）
- [assets/tasks.json](assets/tasks.json) — VSCode build-debug task（cmake --build build/Debug）
- 外部库：https://gitee.com/nzxhg/stm_log（GitHub 镜像：https://github.com/NingZiXi/stm_log）

## 交接

- 加 Wi-Fi / MQTT / ESP-AT → 引导看 `main/lwesp_opts.h` 等 LwESP 集成
- 改 CubeMX 配置 / 加 BSP → `stm32-hal-development`
- 构建 / 烧录 → `build-cmake` / `flash-openocd` / `flash-jlink`
- 日志用法 / 改输出后端 / per-tag 级别控制 / HEX buffer → https://gitee.com/nzxhg/stm_log/blob/main/README.md（GitHub：https://github.com/NingZiXi/stm_log/blob/main/README.md）
- SEGGER RTT 库升级 / 重装 / 改 buffer size → [references/rtt-setup.md](references/rtt-setup.md)