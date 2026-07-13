---
name: stm32-app-main
description: 把 STM32CubeMX + CMake 工程改造成「main/ 子模块」结构：业务代码放到 main/，通过 add_subdirectory(main) 纳入构建，不修改 cmake/ 或 Core/ 下任何文件。支持 FreeRTOS 与裸机。Trigger：用户说「初始化 app_main」「把 app_main 纳入工程」「业务代码独立到 main/」「cube-mx app_main 改造」「add_subdirectory(main)」「裸机改造」「bare metal」。
---

# STM32CubeMX 工程 → main/ 子模块改造

## 适用 / 不适用

- ✅ STM32CubeMX 6.x CMake 工程（根目录有 `*.ioc` + `cmake/stm32cubemx/CMakeLists.txt`）
- ✅ FreeRTOS 或裸机（bare metal）工程
- ❌ MDK / IAR / Makefile / CubeIDE（`.cproject`）工程 → 先用 CubeMX 重新生成一次 CMake
- ❌ 已经在 `main/` 里写过业务、要加新功能 → 直接改 `main/app_main.c`，不需要本 skill

## 改造前后

```
改造前：业务散落在 Core/Src/main.c 或 cmake/stm32cubemx/CMakeLists.txt
        ├── CMakeLists.txt
        ├── Core/Src/main.c                  # CubeMX 生成的 main()
        ├── cmake/stm32cubemx/CMakeLists.txt # MX_Application_Src 里有业务文件
        └── ...

改造后：业务统一到 main/
        ├── CMakeLists.txt                   # +add_subdirectory(main)
        ├── Core/Src/main.c                  # CubeMX 生成的 main()，调用 app_main()
        ├── cmake/stm32cubemx/CMakeLists.txt # 不动
        ├── main/                            # 新增
        │   ├── CMakeLists.txt
        │   └── app_main.c
        └── ...
```

## 工作流（5 步）

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
```

判定：

| 信号 | 处理 |
|------|------|
| `NO_IOC` 或 `NO_CMAKE_MODE` | 中断：请用户先跑 CubeMX 生成 CMake |
| `EXISTS` 且非空 | 警告：列出文件，让用户选覆盖 / 跳过 / 合并 |
| 至少 2 个 `FREERTOS_HAS_*` | FreeRTOS 工程 → 用 `assets/app_main.c` |
| 全部 `FREERTOS_NO_*` | 裸机工程 → 用 `assets/app_main_bare.c` |

### §1 写入 main/

| 源 | 目标 |
|----|------|
| `assets/CMakeLists.txt` | `<root>/main/CMakeLists.txt` |
| `assets/app_log.h` | `<root>/main/app_log.h` |
| `assets/app_log.c` | `<root>/main/app_log.c` |
| `assets/app_main.c` 或 `app_main_bare.c`（按 §0） | `<root>/main/app_main.c` |

`app_log.h` 提供分级日志宏（`LOGI/LOGW/LOGE/LOGD`，输出 `LEVEL (tick) TAG: msg`），`app_log.c` 通过 `_write` 系统调用把 stdout/stderr 重定向到 USART1。两个文件一起被 `main/CMakeLists.txt` 的 `target_sources` 纳入构建。

⚠ **日志宏使用约束**：`fmt` 必须是字符串字面量（如 `"boot, heap=%u"`），不能是变量；`tag` 可为变量或字面量；依赖 GCC `##__VA_ARGS__` 扩展，所以工程需用 `gnu11` 或更高（STM32CubeMX 默认 GCC 12 + `-std=gnu11` 已满足）。

⚠ **为什么用 `_write` 而不是 `fputc`**：工程用 `--specs=nano.specs`（newlib-nano，STM32CubeIDE / STM32CubeCLT 默认）的 printf 绕过 stdio buffer 直接调 `_write`，经典 `fputc` 重定向完全不生效。`_write` 强符号会覆盖 CubeMX 生成的 `Core/Src/syscalls.c` 里的 `__attribute__((weak)) _write` 默认实现，无链接冲突。如果换用 Keil AC5 / IAR 等工具链需要单独适配（IAR 用 `__write`，Keil AC5 同时支持 `fputc` 和 `_write`）。

⚠ 不动 `main/` 里已有的 `lwesp_opts.h` / `lwesp_ll_stm32f407.c` 等 LwESP 文件。

### §2 改根 CMakeLists.txt

在 `add_subdirectory(cmake/stm32cubemx)` 后面加：

```cmake
add_subdirectory(main)
```

**唯一**会改的 CMake 文件。

### §3 接入口

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

### §4 业务文件搬迁（用户手动）

**本 skill 不改 `cmake/` 也不改 `Core/`**。如果用户老工程里业务文件散落在 `Core/Src/` 下：

> 请手动 `git mv` 把它们移到 `main/`，然后在 `main/CMakeLists.txt` 的 `target_sources` 里加上。

### §5 验证

```bash
cmake -S <root> -B <root>/build -DCMAKE_BUILD_TYPE=Debug
cmake --build <root>/build
```

## 失败分流

| 现象 | 根因 | 处理 |
|------|------|------|
| `add_subdirectory given source ... which is not an existing directory` | `main/CMakeLists.txt` 没生成 | 重跑 §1 |
| `undefined reference to app_main` | 根 CMakeLists.txt 没加 `add_subdirectory(main)` | 重跑 §2 |
| `undefined reference to osDelay` / `xPortGetFreeHeapSize` | 用错模板：FreeRTOS 模板跑到了裸机工程 | 重跑 §0 探测 + §1 选对模板 |
| `undefined reference to HAL_Delay` | 裸机模板跑到了 FreeRTOS 工程 | 同上 |
| `app_main` 跑了但系统卡死 | `app_main` 没有死循环，或挂载点错 | 确认模板 `for(;;)` 存在 + 挂载点选 A/B 正确 |
| CubeMX 重生成后 main/ 消失 | 不可能 — main/ 不在 CubeMX 管理范围 | 检查 `.gitignore` 是否误屏蔽 |

## 详细参考

- [references/CMake-integration.md](references/CMake-integration.md) — `add_subdirectory` 方法论、与 CubeMX 重生成的兼容性
- [assets/CMakeLists.txt](assets/CMakeLists.txt) — `main/CMakeLists.txt` 模板
- [assets/app_log.h](assets/app_log.h) / [assets/app_log.c](assets/app_log.c) — 分级日志
- [assets/app_main.c](assets/app_main.c) — FreeRTOS 版入口
- [assets/app_main_bare.c](assets/app_main_bare.c) — 裸机版入口

## 交接

- 加 Wi-Fi / MQTT / ESP-AT → 引导看 `main/lwesp_opts.h` 等 LwESP 集成
- 改 CubeMX 配置 / 加 BSP → `stm32-hal-development`
- 构建 / 烧录 → `build-cmake` / `flash-openocd`