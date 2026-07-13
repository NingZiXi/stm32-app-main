// app_main.c — FreeRTOS 业务入口样板
// 挂载点：Core/Src/freertos.c 的 StartDefaultTask USER CODE 5
// 日志：通过 app_log.h（LOGI/LOGW/LOGE/LOGD 分级日志）

#include "main.h"
#include "cmsis_os.h"

#include "app_log.h"

static const char *TAG = "main";

void app_main(void) {
    LOGI(TAG, "Boot. Heap=%u", (unsigned)xPortGetFreeHeapSize());

    for (;;) {
        // TODO: 加你的业务（osDelay(ms) / xTaskCreate / lwesp_*）
        osDelay(1000);
    }
}