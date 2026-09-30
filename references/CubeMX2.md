# STM32CubeMX2 CMake 接入

使用条件：确认 `.ioc2`、实际导出的 CMake 根目录、`CMakePresets.json`、`generated/` 和应用入口；不要假定 `.ioc2` 与工程根目录一定相同。CubeMX2 的生成代码通常没有 `USER CODE` 标记；不要编辑 `generated/` 中的代码。以下为 STM32C532CCT6 / CubeMX2 1.1.1 工程的已核实例，其他器件以实际产物为准。

## 应用入口与重生成

- 当前实例的根 `main.c` 先调用 `mx_system_init()`，成功分支进入循环。在成功分支的循环前声明并调用 `app_main()`，业务实现在独立 `main/app_main.c`，不要在弱 `post_system_init_hook()` 里调用永不返回的入口：它会阻止初始化流程返回 `SYSTEM_OK`。若实际生成入口不同，先分析调用顺序再决定。
- `main.c` 是 CubeMX2 管理的用户文件，不等同 MX1 的 `USER CODE` 自动合并：当前实例的 `.settings/exported-files.json` 将 `main.c` 标成 `user_code` 并记录 checksum，`CMakeLists.txt` 标为 `project_file`。**这不是覆盖保护保证**。重生成前通过版本控制或备份保留这两个文件；有冲突时查看差异，选择保留现有文件或备份后合并，不使用无备份的“Overwrite all”来处理未合并的用户改动；重生成后验证 `app_main()` 及 `add_subdirectory(main)` 仍存在。
- 对 FreeRTOS，先定位真实任务与启动顺序，在已完成外设初始化、符合调度规则的任务上下文中调用 `app_main()`；其循环延时需用项目所用 RTOS API，而非裸机 `HAL_Delay`。缺少确证时不臆造 `Core/Src/freertos.c` 或 CMSIS-OS 路径。

## CMake 与配置

CubeMX2 实例的根文件先 `include(cmake/target.cmake)`、`include(cmake/${TOOLCHAIN_FILE})`，然后 `project(...)`、`add_executable(...)`，再 include `cmake/files.cmake`、`flags.cmake`、`components.cmake`。在目标建立后、目标相关设置完成后加入 `FetchContent(stm_log)` 及 `add_subdirectory(main)`；只在用户可编辑根文件添加扩展，不改生成的 `cmake/` 文件。参考 SKILL.md 的通用依赖配置和版本选择规则；不要复制 MX1 根 CMake 模板。`main/CMakeLists.txt` 仅向现有目标添加 app 源码和 `stm_log` 链接，找不到目标时失败。当前生成的 `cmake/components.cmake` 对主 target 使用无关键字 `target_link_libraries`，故 `main/` 也必须使用 plain 签名，参见 `assets/CMakeLists_mx2.txt`；不要直接复制 MX1 的 `PRIVATE stm_log` 链接模板。

示例接入片段（追加在现有根目标定义之后，实际版本和日志后端按询问结果填写；不要替换 CubeMX2 原有的 `include(cmake/*.cmake)`）：

```cmake
include(FetchContent)
set(STM_LOG_WITH_RTT OFF) # UART: OFF；RTT: ON
FetchContent_Declare(stm_log
    GIT_REPOSITORY https://gitee.com/nzxhg/stm_log.git
    GIT_TAG        <本次确认的正式标签>
    SOURCE_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/Lib/stm_log
)
# 此实例将 C/C++ 编译器标为 FORCED，CMake 无法自动发现 compile features；
# 已确认工具链 GCC 14.3.1 支持 C11 后，向 stm_log 显式提供所需 feature。
if(NOT CMAKE_C_COMPILE_FEATURES)
    set(CMAKE_C_COMPILE_FEATURES c_std_11)
endif()
FetchContent_MakeAvailable(stm_log)
# 外部 target 不继承主 target 的 Cortex-M 编译选项；否则链接出现 ABI/架构冲突。
target_compile_options(stm_log PRIVATE ${CPU_FLAGS})
set(CONFIG_LOG_ENABLED ON CACHE BOOL "Enable stm_log output")
set(APP_VERSION "1.0.0" CACHE STRING "Application firmware version")
target_compile_definitions(stm_log PUBLIC STM_LOG_ENABLED=$<BOOL:${CONFIG_LOG_ENABLED}>)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    CONFIG_APP_VERSION="${APP_VERSION}"
    CONFIG_LOG_ENABLED=$<BOOL:${CONFIG_LOG_ENABLED}>
)
add_subdirectory(main)
```
读取真实 `CMakePresets.json` 的 configure/build preset、`binaryDir` 和工具链文件名。示例工程 preset 为 `debug_GCC_STM32C532CCT6`，二进制目录 `build/${presetName}`；这是实例，不是模板默认值。优先：

```sh
cmake --preset <实际配置预设>
cmake --build --preset <实际构建预设>
```

若预设中已有对应 build preset，不要猜测 `build/Debug`、`cmake/gcc-arm-none-eabi.cmake`；必要时先查系统 CMake 版本是否满足根 CMake 最低要求。

## HAL2 UART 与 RTT

当前 STM32C5 的 `generated/hal/mx_usart2.h` 暴露 `hal_uart_handle_t *mx_usart2_uart_gethandle(void)`；句柄在 `.c` 内是 `static`，不能 `extern huart2`。芯片 HAL2 的 `HAL_UART_Transmit(hal_uart_handle_t *, const void *, uint32_t, uint32_t)` 可用于同步 UART 输出。仅在用户选 UART 且该串口适合日志时，以实际外设名/头文件改 `assets/app_main_bare_mx2_uart.c`。`HAL_GetTick()` 和 `HAL_Delay()` 在当前芯片头文件中可用；迁移其他芯片仍应检查接口。

用户选 RTT 且调试探针/服务器支持 RTT 时使用 `assets/app_main_bare_mx2_rtt.c`，`STM_LOG_WITH_RTT=ON`；不要额外向应用链接 RTT target，RTT 依赖由已锁定版本的 `stm_log` 管理。两个后端的 `stm_log_set_tick(HAL_GetTick)` 与 `stm_log_init_output(...)` 调用需与选定版本的公开头文件核对。

官方参考：[STM32CubeMX2 IDE project generation](https://dev.st.com/stm32cube-docs/stm32cubemx2/1.1.0/en/docs/markup/CubeMX2_UserManual/CubeMX2_UserManual_IDE_ProjectGen.html)（用户文件分类、重生成冲突规则和目录结构）；[FAQ](https://dev.st.com/stm32cube-docs/stm32cubemx2/1.1.0/en/docs/markup/CubeMX2_FAQ.html)（用户代码与生成代码隔离）。




VS Code 规则以 SKILL.md 为准：选择 UART 时沿用已有/默认配置，不新建或改写 tasks/launch；选择 RTT 时才按实际调试环境补齐。

