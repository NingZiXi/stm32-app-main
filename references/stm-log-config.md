# stm_log v3.0.0 配置参考

`stm_log` v3.0.0 是平台无关的 C 日志核心，不包含 HAL/CMSIS 头文件，也不初始化 UART。UART、RTT、SWO 和 USB CDC 都由应用提供输出回调。

依赖与日志开关的 CMake 示例统一见 [CMake 集成](CMake-integration.md)。v3 不使用 `STM_LOG_HAL_HEADER` 或 `STM_LOG_LINK_CUBEMX`；应用自己的 HAL 头文件仍须正确配置。

## 应用接入

先提供毫秒时钟，再绑定输出。回调必须在返回前完成发送或复制数据，不能保存传入的临时指针。

```c
static void uart_output(const char *data, uint16_t len)
{
    (void)HAL_UART_Transmit(&huart1, (uint8_t *)data, len, 100U);
}

stm_log_set_tick(HAL_GetTick);
stm_log_init_output(uart_output, STM_LOG_LVL_INFO);
```

RTT 只替换回调：

```c
static void rtt_output(const char *data, uint16_t len)
{
    SEGGER_RTT_Write(0, data, len);
}

SEGGER_RTT_Init();
stm_log_set_tick(HAL_GetTick);
stm_log_init_output(rtt_output, STM_LOG_LVL_INFO);
```

`stm_log_init(&huart1, level)` 已移除；不能用 `stm_log_set_output(NULL)` 恢复默认 UART，因为 v3 没有内置 UART 后端。

RTT 的获取、离线与变量说明见 [RTT 接入](rtt-setup.md)，只在使用 RTT 时读取。

## 常见错误

- `undefined reference to stm_log_init`：模板仍使用旧 v2 初始化接口，改为输出回调 + `stm_log_init_output`。
- `undefined reference to SEGGER_RTT_Init`：开启 `STM_LOG_WITH_RTT`，并确认 RTT 源可用。
- 日志库报 `stm32f4xx_hal.h not found`：检查实际 checkout 和编译路径，可能仍编译了旧版本。v3 不读取 `STM_LOG_HAL_HEADER`，仅保留该宏不会引发 include；应用自身的 HAL 依赖仍须正确配置。
