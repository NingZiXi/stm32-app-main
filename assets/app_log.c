// app_log.c — 日志后端：stdout/stderr → USART1
// 依赖：USART1 已被 CubeMX 在 main.c 初始化（默认 115200）

// 为什么用 _write 而不是 fputc？
// newlib-nano（-specs=nano.specs，STM32CubeIDE / STM32CubeCLT 默认）的 printf
// 绕过 stdio buffer 层，直接调 _write 系统调用。fputc 永远不被调用，导致 printf 输出全无。
// _write 在 GCC ARM / Keil AC5 / IAR 主流工具链下都被支持（Keil AC5 兼容）。
// 强符号 _write 会覆盖 syscalls.c 里的 __attribute__((weak)) _write 默认实现。

#include "main.h"     // HAL_UART_Transmit、huart1 类型
#include "app_log.h"
#include <errno.h>

int _write(int fd, const void *buf, int len) {
    if ((fd == 1 || fd == 2) && len > 0) {   // stdout / stderr
        HAL_UART_Transmit(&huart1, (const uint8_t*)buf, (uint16_t)len, HAL_MAX_DELAY);
        return len;
    }
    errno = EBADF;
    return -1;
}