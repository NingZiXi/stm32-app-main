# stm32-app-main

`stm32-app-main` 是面向编程 Agent 的 STM32 应用接入 Skill：在**已有的 STM32CubeMX 1（`.ioc`）或 STM32CubeMX2（`.ioc2`）CMake 工程**中，将业务代码放入独立 `main/`，接入 `stm_log`，并按实际工程配置和验证应用入口。它支持裸机工程；使用 FreeRTOS 时，必须先确认目标工程实际生成的任务和 API。

```text
已有的 CubeMX 1 / CubeMX2 CMake 工程
                 ↓
          stm32-app-main
                 ↓
    main/app_main.c + stm_log
                 ↓
      构建、日志与调试验证
```

本 Skill **不负责创建 `.ioc` / `.ioc2` 工程**，也不适用于未适配的 MDK、IAR、Makefile 或 CubeIDE 工程。创建或修改 CubeMX2 配置可另行使用相应工具；本 Skill 从现有 CMake 工程接手。

## To 人类

将本仓库的**整个目录**安装到项目级 `.agents/skills/stm32-app-main/`，或用户级 `~/.agents/skills/stm32-app-main/`，确保该目录下直接存在 `SKILL.md`。例如，可以告诉 Agent：

> 将 https://github.com/NingZiXi/stm32-app-main 安装到当前工程的 `.agents/skills/`，并确认 `stm32-app-main/SKILL.md` 可以被识别。

安装后如客户端尚未识别新 Skill，重新加载会话或 IDE 扩展。之后直接描述目标工程，例如：

> 在这个 STM32CubeMX 1 生成的 CMake 工程中，使用 `stm32-app-main` 将业务逻辑接入独立 `main/`，配置 `stm_log`，保留已有用户代码，并构建核验。

> 在这个 STM32CubeMX2 的 `.ioc2` CMake 工程中，使用 `stm32-app-main` 接入 `main/app_main.c` 与 `stm_log`，检查重生成风险，使用实际预设构建并报告验证结果。

**两种路径都会先询问你 `stm_log` 使用 UART 串口还是 RTT**，不会仅凭板上有串口或调试器自行决定。选择 UART 时核对实际可用的串口；选择 RTT 时核对探针和调试方式。

## To Agent

执行要求以 [SKILL.md](SKILL.md) 为准；README 仅介绍使用方式，不替代操作约束。简要流程：

1. 确认工程确实是 CubeMX1 或 CubeMX2 生成的 CMake 工程，检查已有业务代码、日志与构建配置。
2. 主动询问 UART / RTT，再选对应路径和模板；查询并锁定 `stm_log` 正式版本。
3. 根据实际生成的入口、HAL/RTOS API 与 CMake 目标接入 `main/`，合并而非覆盖用户修改。
4. 使用工程实际工具链或 CMake preset 构建，分别报告构建、调试配置、日志与实机验证的结果。

## 主要能力

- 识别 CubeMX1 与 CubeMX2 的工程结构，分别接入裸机应用入口；FreeRTOS 入口以实际生成代码为准。
- 将 `app_main.c` 和 `stm_log` 纳入现有 CMake 工程，按选择配置 UART 或 RTT 输出。
- 保留已有业务代码，按后端处理 VS Code：**UART 沿用已有或默认配置，不因日志接入新建或改写 `tasks.json`、`launch.json`；RTT 才按实际环境补齐或合并调试配置。**
- 核验应用源码参与编译、日志符号参与链接、构建产物及相关配置，并说明尚未验证的硬件功能。

## 执行边界

- 不把 MX1 的 `Core/`、`USER CODE` 和 HAL1 模板直接套进 MX2；MX2 的根 `main.c`、HAL2 接口和 CMake 链接方式必须按工程核对。
- 模板只是移植起点：MX2 裸机示例基于 STM32C532CCT6，UART 外设、句柄、工具链和构建预设不能照搬到其他工程。
- MX2 的根 `main.c` 属用户文件，但重生成时仍可能因冲突处理被覆盖；重生成前后要检查入口调用和根 CMake 接入。
- 编译成功不代表日志已实际输出，也不代表 VS Code 调试、烧录或硬件功能已经验证；没有实测时明确报告。

## 仓库结构

```text
SKILL.md                     Skill 入口、共同约束与 MX1/MX2 分流
assets/                      MX1、MX2 应用与 CMake 示例；VS Code 结构示例
references/CMake-integration.md  CubeMX1 接入细节
references/CubeMX2.md            CubeMX2 接入与重生成注意事项
references/stm-log-version.md    stm_log 正式版本选择
references/                    日志、RTT、构建等按需阅读的细节
```

先读 [SKILL.md](SKILL.md)；仅在对应任务需要时再读相关 `references/`。`assets/` 中的源码和配置需要按目标工程改写，不是可不经检查直接复制的成品。

## 许可证

本仓库采用 [MIT 许可证](LICENSE)。
