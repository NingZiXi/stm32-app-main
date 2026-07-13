// app_log.h — 分级日志接口
// 用法：
//   static const char *TAG = "main";
//   LOGI(TAG, "boot, heap=%u", xPortGetFreeHeapSize());   // tag 可为变量，fmt 必须是字符串字面量
//   LOGW(TAG, "mqtt refused, status=%u", status);
//   LOGE(TAG, "init failed: %s", err);
//   LOGD(TAG, "rx %u bytes", n);
//   LOGI(TAG, "no args ok");                              // 零额外参数也支持（GCC ##__VA_ARGS__）
//
// 输出格式（默认带 ANSI 颜色）：
//   I (1234) main: boot, heap=12345    ← 绿
//   W (1234) main: mqtt refused         ← 黄
//   E (1234) main: init failed          ← 红
//   D (1234) main: rx 12 bytes          ← 灰
//
// 关闭颜色（XCOM / SSCOM 等不支持 ANSI 的终端会显示成乱码）：
//   CMake 加 target_compile_definitions(... APP_LOG_COLORS=0)

#ifndef APP_LOG_H
#define APP_LOG_H

#include <stdio.h>

extern UART_HandleTypeDef huart1;
// stdout/stderr 重定向通过 _write（app_log.c），不用 fputc —
// newlib-nano 的 printf 不走 fputc。

// 默认开 ANSI 颜色。要关掉在 CMake 加 -DAPP_LOG_COLORS=0。
#define APP_LOG_COLORS   1

#if APP_LOG_COLORS
#define _CLR_I  "\033[32m"   // 绿
#define _CLR_W  "\033[33m"   // 黄
#define _CLR_E  "\033[31m"   // 红
#define _CLR_D  "\033[90m"   // 灰
#define _CLR_R  "\033[0m"    // reset
#else
#define _CLR_I  ""
#define _CLR_W  ""
#define _CLR_E  ""
#define _CLR_D  ""
#define _CLR_R  ""
#endif

/* 分级日志：LEVEL (tick) TAG: fmt [args...]
 * 约束：
 *   - tag   可为 const char* 变量或字符串字面量（作为 %s 参数）
 *   - fmt   必须是字符串字面量（与前后 ANSI 控制码做字面量拼接）
 *   - 依赖 GCC ##__VA_ARGS__ 扩展处理零额外参数情况
 */
#define LOGI(tag, fmt, ...)  printf(_CLR_I "I (%lu) %s: " fmt _CLR_R "\r\n", (unsigned long)HAL_GetTick(), tag, ##__VA_ARGS__)
#define LOGW(tag, fmt, ...)  printf(_CLR_W "W (%lu) %s: " fmt _CLR_R "\r\n", (unsigned long)HAL_GetTick(), tag, ##__VA_ARGS__)
#define LOGE(tag, fmt, ...)  printf(_CLR_E "E (%lu) %s: " fmt _CLR_R "\r\n", (unsigned long)HAL_GetTick(), tag, ##__VA_ARGS__)
#define LOGD(tag, fmt, ...)  printf(_CLR_D "D (%lu) %s: " fmt _CLR_R "\r\n", (unsigned long)HAL_GetTick(), tag, ##__VA_ARGS__)

#endif /* APP_LOG_H */