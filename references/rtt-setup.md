# SEGGER RTT 与 stm_log v3.0.0

仅使用 RTT 时读取。根 CMake 的固定依赖声明见 [CMake 集成](CMake-integration.md)，输出初始化契约见 [日志回调](stm-log-config.md)，完整代码按裸机/FreeRTOS 选择对应 assets 模板。

在 `FetchContent_MakeAvailable(stm_log)` 前设置 `STM_LOG_WITH_RTT=ON`。stm_log 创建 segger_rtt 并通过 PUBLIC 依赖传递；应用只链接 stm_log，不单独 FetchContent、包装或链接 RTT。

## 路径和离线配置

推荐布局：

```text
Lib/
├── stm_log/       # v3.0.0
└── segger_rtt/    # 可选，本地 RTT 源码
```

离线构建可设置 `STM_LOG_RTT_SOURCE_DIR`；禁止下载则设置 `STM_LOG_RTT_FETCH=OFF`。自定义 `SEGGER_RTT_Conf.h` 所在目录通过 `STM_LOG_RTT_CONFIG_DIR` 传入。镜像通过 `STM_LOG_RTT_GIT_REPOSITORY` 指定。

v3.0.0 自动下载的 RTT 默认位于构建目录 `_deps/`；已有同级源码才会复用 `Lib/`，不能承诺首次下载自动落在 `Lib/`。显式源码目录须已含 `RTT/SEGGER_RTT.c`、`RTT/SEGGER_RTT.h` 和 `Config/SEGGER_RTT_Conf.h`。已有 target 优先于显式目录，显式目录优先于同级目录，最后才下载；本地版本由应用负责。

## 验证

确认 `SEGGER_RTT.c` 被编译、`SEGGER_RTT.h` 可被包含，并通过 J-Link RTT 或 Cortex-Debug 观察 `LOGI` 输出。`stm_log_set_tick(HAL_GetTick)` 后日志中的时间戳应随 HAL tick 增长。
