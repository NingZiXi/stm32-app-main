/** Bare-metal STM32CubeMX2 application, UART logging via HAL2.
 * Adapt the peripheral include and getter to the actual generated project.
 */
#include <stddef.h>
#include <stdint.h>
#include "main.h"
#include "mx_usart2.h"  /* Example only: select the user's UART peripheral. */
#include "stm_log.h"

#define TAG "main"

#if STM_LOG_ENABLED
static void uart_output(const char *data, uint16_t len)
{
    hal_uart_handle_t *uart = mx_usart2_uart_gethandle();
    if (uart != NULL) {
        (void)HAL_UART_Transmit(uart, data, (uint32_t)len, 100U);
    }
}
#endif

/* Called only after mx_system_init() has returned SYSTEM_OK. */
void app_main(void)
{
#if STM_LOG_ENABLED
    stm_log_set_tick(HAL_GetTick);
    stm_log_init_output(uart_output, STM_LOG_LVL_INFO);
#endif
    LOGI(TAG, "Boot (bare metal, v%s)", CONFIG_APP_VERSION);
    for (;;) {
        HAL_Delay(1000U);
    }
}

