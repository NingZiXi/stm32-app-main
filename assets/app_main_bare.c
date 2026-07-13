// app_main.c — 裸机业务入口样板
// 挂载点：Core/Src/main.c 的 USER CODE 2（MX_*_Init 之后、while 之前）
// 日志：通过 app_log.h（LOGI/LOGW/LOGE/LOGD 分级日志）

#include "main.h"

#include "app_log.h"

static const char *TAG = "main";

void app_main(void) {
    LOGI(TAG, "Boot (bare metal)");

    for (;;) {
        // TODO: 加你的业务（HAL_Delay(ms) / 中断回调等）
        HAL_Delay(1000);
    }
}