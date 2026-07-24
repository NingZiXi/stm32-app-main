# stm32-app-main

Claude Code skill —— 把 STM32CubeMX + CMake 工程改造成业务独立的 `main/` 子模块结构。

调用一次 `/stm32-app-main`，skill 会自动：
- 检测工程基线（FreeRTOS vs 裸机）
- 注入 `main/` 目录（业务入口 + 日志接口）
- 在根 `CMakeLists.txt` 加一行 `add_subdirectory(main)`
- 把 `app_main()` 接到合适的入口（CubeMX 生成的 `freertos.c` 或 `main.c`）
- **不修改** `cmake/` 或 `Core/` 下任何文件，CubeMX 重生成代码不会冲掉业务

## 特性

- 🚫 **零侵入**：唯一改的工程文件是根 `CMakeLists.txt`（加一行）
- 🔌 **双模式**：FreeRTOS 与裸机工程自动适配
- 📝 **分级日志**：`LOGI/LOGW/LOGE/LOGD` + ANSI 颜色 + tick 时间戳
- 🛡 **newlib-nano 兼容**：用 `_write` 系统调用而不是失效的 `fputc` 重定向
- 📦 **轻量**：skill 模板总共 7 个文件、约 400 行

## 适用

- STM32CubeMX 6.x 生成的 CMake 工程（根目录有 `*.ioc` + `cmake/stm32cubemx/CMakeLists.txt`）
- arm-none-eabi-gcc + newlib-nano（STM32CubeIDE / STM32CubeCLT 默认工具链）
- FreeRTOS 或裸机（bare metal）

不适用：
- MDK / IAR / Makefile 工程（先用 CubeMX 重新生成 CMake）
- STM32CubeIDE（`.cproject`）工程

## 安装

```bash
# 把整个仓库克隆到全局 skills 目录
git clone https://github.com/NingZiXi/stm32-app-main.git \
  ~/.claude/skills/stm32-app-main
```

Windows (Git Bash)：

```bash
git clone https://github.com/NingZiXi/stm32-app-main.git \
  /c/Users/<you>/.claude/skills/stm32-app-main
```

安装完成后重启 Claude Code（或新开一个会话），`stm32-app-main` 会出现在可用 skill 列表里。

## 使用

### 斜杠命令

```
/stm32-app-main
```

### 自然语言触发

skill 会按这些说法自动加载：

- 初始化 app_main
- 把 app_main 纳入工程
- 业务代码独立到 main/
- cube-mx app_main 改造
- add_subdirectory(main)
- 裸机改造
- bare metal

### 带参数

```
/stm32-app-main 在 D:/new_project 里跑
/stm32-app-main 这是裸机工程，强制走 USER CODE 2 入口
```

## 工作流（6 步）

| § | 动作 |
|---|------|
| §0 探测 | 多源证据判定 FreeRTOS vs 裸机 |
| §1 stm_log 依赖 | FetchContent 自动 clone 到 `Lib/stm_log/`（SOURCE_DIR 指定） |
| §2 写入 | 拷贝 3 个 assets 模板到工程的 `main/` 目录 |
| §3 改根 CMake | 在根 `CMakeLists.txt` 加 `FetchContent_Declare(stm_log)` + `add_subdirectory(main)` |
| §4 接入口 | FreeRTOS → `freertos.c` USER CODE 5 / 裸机 → `main.c` USER CODE 2 |
| §5 业务搬迁 | 老工程 `Core/Src/` 里的业务文件由用户手动移到 `main/` |
| §6 验证 | `cmake -S/-B + cmake --build` |

## 改造前后

```
改造前（典型 CubeMX 工程）：
├── CMakeLists.txt
├── Core/Src/main.c                  CubeMX 生成的 main()
├── cmake/stm32cubemx/CMakeLists.txt 业务文件可能混在 MX_Application_Src
└── ...

改造后：
├── CMakeLists.txt                   +FetchContent_Declare(stm_log, SOURCE_DIR=Lib/stm_log)
│                                    +add_subdirectory(main)
├── Core/Src/freertos.c              +app_main();  USER CODE 5（FreeRTOS）
│   或 Core/Src/main.c               +app_main();  USER CODE 2（裸机）
├── cmake/stm32cubemx/CMakeLists.txt 不动 ✓
├── Lib/                             新增（FetchContent 把源码拉到这里）
│   └── stm_log/                     clone 自 https://gitee.com/nzxhg/stm_log @ v2.3.1（GitHub 镜像：NingZiXi/stm_log）
└── main/                            新增
    ├── CMakeLists.txt
    └── app_main.c                   业务入口
```

## 模板文件

| 文件 | 大小 | 作用 |
|------|------|------|
| [assets/CMakeLists.txt](assets/CMakeLists.txt) | 12 行 | `main/CMakeLists.txt` 模板 |
| [assets/app_main.c](assets/app_main.c) | 18 行 | FreeRTOS 版业务入口 |
| [assets/app_main_bare.c](assets/app_main_bare.c) | 17 行 | 裸机版业务入口 |
| [references/CMake-integration.md](references/CMake-integration.md) | — | `add_subdirectory` + FetchContent 方法论 |
| [SKILL.md](SKILL.md) | — | skill 主入口（被 Claude Code 自动识别） |

## 工具链兼容

| 工具链 | `_write` | `fputc` | 备注 |
|--------|---------|---------|------|
| arm-none-eabi-gcc + newlib-nano | ✅ | ❌ | **推荐**（STM32CubeIDE / STM32CubeCLT 默认） |
| Keil AC5 (armcc) | ✅ | ✅ | skill 默认 `_write` 也工作 |
| IAR EWARM | ❌（需用 `__write`） | ❌ | 需手动适配 |
| Clang for ARM | ✅ | ✅ | `_write` 工作 |

## 使用约束

⚠ **日志宏 `fmt` 必须是字符串字面量**，不能是变量：

```c
LOGI(TAG, "boot, heap=%u", xPortGetFreeHeapSize());  // ✅
char buf[] = "boot, heap=%u";
LOGI(TAG, buf, xPortGetFreeHeapSize());               // ❌ 编译失败
```

理由：宏内部用 token 拼接 ANSI 控制码，前后必须是相邻字符串字面量（C99 6.4.4p5）。

⚠ 依赖 GCC `##__VA_ARGS__` 扩展处理零额外参数情况，工程需用 `-std=gnu11` 或更高（STM32CubeMX 默认 GCC 12 + `-std=gnu11` 已满足）。

⚠ 强符号 `_write` 覆盖 CubeMX `Core/Src/syscalls.c` 里的 `__attribute__((weak)) _write` 默认实现，无链接冲突。

## 关闭日志颜色

默认开 ANSI 颜色（绿/黄/红/灰），适用于 `idf.py monitor`、VSCode 串口插件、Linux minicom 等。

XCOM / SSCOM 等不支持 ANSI 的终端会显示成乱码，要关闭：

```cmake
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE STM_LOG_COLORS=0)
```

## stm_log 源码位置

通过 `FetchContent_Declare(stm_log, SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Lib/stm_log)`，首次 cmake configure 时：

- 工程内无 `Lib/stm_log/` → FetchContent 自动 clone 到 `<root>/Lib/stm_log/`
- 工程内已有 `Lib/stm_log/` → FetchContent 跳过拉取复用现有内容（适合离线 / 代理环境手动 `git clone`）

源码落在工程内而非 `<build>/_deps/`，便于 IDE 索引 / 加入版本控制 / 离线复用。

升级：

```bash
cd Lib/stm_log && git pull
```

## 失败分流

| 现象 | 处理 |
|------|------|
| `add_subdirectory given source...` | `main/CMakeLists.txt` 没拷好，重跑 §2 |
| `Failed to clone ... gitee.com/nzxhg/stm_log.git` | Gitee / 代理异常；手动 `git clone https://gitee.com/nzxhg/stm_log <root>/Lib/stm_log`，FetchContent 会跳过拉取复用；或切回 GitHub：把 `GIT_REPOSITORY` 改 `https://github.com/NingZiXi/stm_log.git` |
| `undefined reference to app_main` | 根 CMakeLists.txt 没加 `add_subdirectory(main)` |
| `undefined reference to osDelay` / `HAL_Delay` | 模板选错（FreeRTOS ↔ 裸机），重跑 §0 探测 |
| printf 输出全无（最常见） | `_write` 没生效，检查 stm_log 源码是否被 `target_sources` 加入（FetchContent 自动处理） |
| 系统卡死 | `app_main` 没死循环，或挂载点选错（USER CODE 5 / USER CODE 2） |

## 文档

- [SKILL.md](SKILL.md) —— skill 主入口，Claude Code 自动读取
- [references/CMake-integration.md](references/CMake-integration.md) —— `add_subdirectory` 方法论与 CubeMX 重生成兼容性详解

## 贡献

欢迎 PR：
- 适配更多 MCU 系列（F1 / H7 / G4 / L4 等）
- 增加更多日志后端（SWO / ITM / RTT）
- 增加单元测试框架集成（CMock / Unity）

## License

MIT —— 见 [LICENSE](LICENSE)。