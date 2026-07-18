// app_main.c — 裸机业务入口样板
// 挂载点：Core/Src/main.c 的 USER CODE 2（MX_*_Init 之后、while 之前）
// 日志：stm_log 库（https://github.com/NingZiXi/stm_log）

#include "main.h"

#include "stm_log.h"

static const char *TAG = "main";

void app_main(void) {
    stm_log_init(&huart1, STM_LOG_LVL_INFO);                             /* 绑定调试 UART + 全局 level */
    LOGI(TAG, "Boot (bare metal)");

    for (;;) {
        // TODO: 加你的业务（HAL_Delay(ms) / 中断回调等）
        HAL_Delay(1000);
    }
}