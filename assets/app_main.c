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

#include "stm_log.h"

/* main.c 里以文件作用域定义，不在 main.h 里 extern — app_main 需要直接引用 */
extern UART_HandleTypeDef huart1;

static const char *TAG = "main";

/**
 * @brief 应用入口；FreeRTOS 默认任务里调用，永不返回
 *
 * @note    挂载点：Core/Src/freertos.c 的 StartDefaultTask USER CODE 5
 */
void app_main(void) {
    stm_log_init(&huart1, STM_LOG_LVL_INFO);                         // 绑定调试 UART + 全局 level
    LOGI(TAG, "Boot. Heap=%u", (unsigned)xPortGetFreeHeapSize());

    for (;;) {
        osDelay(1000);                                                // 1 Hz 业务心跳；按需替换
    }
}
