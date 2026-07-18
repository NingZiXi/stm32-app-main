// app_main.c — FreeRTOS 业务入口样板
// 挂载点：Core/Src/freertos.c 的 StartDefaultTask USER CODE 5
// 日志：stm_log 库（https://github.com/NingZiXi/stm_log）

#include "main.h"
#include "cmsis_os.h"

#include "stm_log.h"

static const char *TAG = "main";

void app_main(void) {
    stm_log_init(&huart1, STM_LOG_LVL_INFO);                             /* 绑定调试 UART + 全局 level */
    LOGI(TAG, "Boot. Heap=%u", (unsigned)xPortGetFreeHeapSize());

    for (;;) {
        // TODO: 加你的业务（osDelay(ms) / xTaskCreate / lwesp_*）
        osDelay(1000);
    }
}