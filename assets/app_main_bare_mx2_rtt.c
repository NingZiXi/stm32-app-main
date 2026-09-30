/** Bare-metal STM32CubeMX2 application, J-Link RTT logging.
 * Check the selected stm_log release and target RTT setup before use.
 */
#include <stdint.h>
#include "main.h"
#include "stm_log.h"

#define TAG "main"

#if STM_LOG_ENABLED
#include "SEGGER_RTT.h"

static void rtt_output(const char *data, uint16_t len)
{
    (void)SEGGER_RTT_Write(0, data, len);
}
#endif

/* Called only after mx_system_init() has returned SYSTEM_OK. */
void app_main(void)
{
#if STM_LOG_ENABLED
    SEGGER_RTT_Init();
    stm_log_set_tick(HAL_GetTick);
    stm_log_init_output(rtt_output, STM_LOG_LVL_INFO);
#endif
    LOGI(TAG, "Boot (bare metal, RTT, v%s)", CONFIG_APP_VERSION);
    for (;;) {
        HAL_Delay(1000U);
    }
}

