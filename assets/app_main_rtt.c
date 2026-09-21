/**
 * @file    app_main.c
 * @author  宁子希 (1589326497@qq.com)
 * @brief   FreeRTOS 业务入口，日志走 RTT 后端
 * @version 0.1
 * @date    2026-07-XX
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "main.h"
#include "cmsis_os.h"
#include "FreeRTOS.h"

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
 * @brief 业务入口；FreeRTOS 默认任务里调用，永不返回
 *
 * @note    挂载点：Core/Src/freertos.c 的 StartDefaultTask USER CODE 5
 */
void app_main(void) {
#if STM_LOG_ENABLED
    SEGGER_RTT_Init();
    stm_log_set_tick(HAL_GetTick);
    stm_log_init_output(rtt_output, STM_LOG_LVL_INFO);
#endif

    LOGI(TAG, "Boot (FreeRTOS → RTT, v%s). Heap=%u", CONFIG_APP_VERSION, (unsigned)xPortGetFreeHeapSize());
    for (;;) {
        osDelay(1000);                                              // 1 Hz 心跳
    }
}
