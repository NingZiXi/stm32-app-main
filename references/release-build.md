# Debug / Release 日志裁剪

`stm_log v3.0.0` 支持 `STM_LOG_ENABLED` 编译期开关。工程应同时给应用和 `stm_log` target 传递同一个值，确保 LOG 宏和库实现一致。

CMake 开关的定义见 [CMake 集成](CMake-integration.md)。Debug/Release 可以分别覆盖 `CONFIG_LOG_ENABLED`，不要在应用头文件重复定义 `STM_LOG_ENABLED`。

## 应用模板

`stm_log.h` 始终包含；日志开启时的输出初始化见 [日志回调](stm-log-config.md) 和对应 assets 模板。

若工程希望在 `CONFIG_LOG_ENABLED=OFF` 时不包含 RTT 源，可用条件编译保护 `SEGGER_RTT.h` 和初始化代码，同时关闭 `STM_LOG_WITH_RTT`；不要条件删除 `stm_log.h`。

## 构建

以下示例要求工程已配置实际工具链；有 preset 时使用工程预设，并按其 binaryDir 替换构建与 ELF 路径。

```bash
cmake -S . -B build/Debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCONFIG_LOG_ENABLED=ON
cmake --build build/Debug
cmake -S . -B build/Release -G Ninja -DCMAKE_BUILD_TYPE=Release -DCONFIG_LOG_ENABLED=OFF
cmake --build build/Release
```

结合 `-ffunction-sections -fdata-sections` 和链接器 `--gc-sections`，关闭日志后未引用的格式化、输出和 RTT 代码会被回收。实际节省量应以 `.map` 和 `arm-none-eabi-size` 为准，不能按固定 KB 承诺。

## 验证

```powershell
arm-none-eabi-nm build/Release/*.elf | Select-String stm_log
arm-none-eabi-size build/Debug/*.elf build/Release/*.elf
```

确认 Debug 有日志符号，Release 的日志调用被裁剪或不再产生输出。`STM_LOG_ENABLED` 必须只由 CMake 统一定义，避免宏重定义警告。

## 常见错误

- `STM_LOG_ENABLED redefined`：检查工程是否在其他头文件手动定义了该宏。
- `undefined reference to stm_log_init`：这是 v2 API；v3 使用输出回调、`stm_log_set_tick`、`stm_log_init_output`。
- `undefined reference to SEGGER_RTT_Init`：启用 `STM_LOG_WITH_RTT` 或移除 RTT 应用代码。
