// app_log.c — stdout/stderr → USART1
// 用 _write 而不是 fputc（newlib-nano 的 printf 直接调 _write）
// 强符号覆盖 Core/Src/syscalls.c 里的 __attribute__((weak)) _write

#include "main.h"
#include "app_log.h"
#include <errno.h>

int _write(int fd, const void *buf, int len) {
    if ((fd == 1 || fd == 2) && len > 0) {
        HAL_UART_Transmit(&huart1, (const uint8_t*)buf, (uint16_t)len, HAL_MAX_DELAY);
        return len;
    }
    errno = EBADF;
    return -1;
}