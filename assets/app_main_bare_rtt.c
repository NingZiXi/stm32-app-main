/**
 * @file    app_main.c
 * @author  宁子希 (1589326497@qq.com)
 * @brief   裸机业务入口，日志走 RTT 后端
 * @version 0.1
 * @date    2026-07-XX
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "main.h"

#include "stm_log.h"
#define TAG "main"
#if STM_LOG_ENABLED
#include "SEGGER_RTT.h"

/**
 * @brief RTT 输出 callback — stm_log 整条 log 写到 RTT up channel 0
 *
 * @param  buf  已格式化好的 log 字符串
 * @param  len  有效字节数，包含配置的换行
 */
static void rtt_output(const char *buf, uint16_t len) {
    SEGGER_RTT_Write(0, buf, len);                                  // channel 0 = 默认 terminal
}
#endif

/**
 * @brief 业务入口；main.c 调用，永不返回
 *
 * @note    挂载点：Core/Src/main.c USER CODE 2（MX_*_Init 之后、while 之前）
 */
void app_main(void) {
#if STM_LOG_ENABLED
    SEGGER_RTT_Init();
    stm_log_set_tick(HAL_GetTick);
    stm_log_init_output(rtt_output, STM_LOG_LVL_INFO);
#endif

    LOGI(TAG, "Boot (bare metal → RTT, v%s)", CONFIG_APP_VERSION);

    for (;;) {
        LOGI(TAG, "tick=%lu", (unsigned long)HAL_GetTick());         // 1 Hz 心跳
        HAL_Delay(1000);
    }
}
