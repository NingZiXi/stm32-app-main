/**
 * @file    app_main.c
 * @author  宁子希 (1589326497@qq.com)
 * @brief   FreeRTOS 业务入口
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

#if STM_LOG_ENABLED
// 按工程实际调试串口替换。
extern UART_HandleTypeDef huart1;

/** @brief 同步发送日志，不保存输出缓冲指针。 */
static void uart_output(const char *data, uint16_t len) {
    (void)HAL_UART_Transmit(&huart1, (uint8_t *)data, len, 100U);
}
#endif

#define TAG "main"

/**
 * @brief 应用入口；FreeRTOS 默认任务里调用，永不返回
 *
 * @note    挂载点：Core/Src/freertos.c 的 StartDefaultTask USER CODE 5
 */
void app_main(void) {
#if STM_LOG_ENABLED
    stm_log_set_tick(HAL_GetTick);
    stm_log_init_output(uart_output, STM_LOG_LVL_INFO);
#endif
    LOGI(TAG, "Boot (v%s). Heap=%u", CONFIG_APP_VERSION, (unsigned)xPortGetFreeHeapSize());
    for (;;) {
        osDelay(1000);                                                // 1 Hz 业务心跳；按需替换
    }
}

