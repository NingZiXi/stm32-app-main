// app_log.h — 分级日志接口（LOGI/LOGW/LOGE/LOGD + ANSI 颜色）
//
//   static const char *TAG = "main";
//   LOGI(TAG, "boot, heap=%u", xPortGetFreeHeapSize());
//   LOGI(TAG, "no args ok");        // 零额外参数（GCC ##__VA_ARGS__）
//
// 输出：I (1234) main: boot, heap=12345  ← 绿/黄/红/灰
// 关色：CMake 加 -DAPP_LOG_COLORS=0

#ifndef APP_LOG_H
#define APP_LOG_H

#include <stdio.h>

extern UART_HandleTypeDef huart1;

#define APP_LOG_COLORS   1

#if APP_LOG_COLORS
#define _CLR_I  "\033[32m"
#define _CLR_W  "\033[33m"
#define _CLR_E  "\033[31m"
#define _CLR_D  "\033[90m"
#define _CLR_R  "\033[0m"
#else
#define _CLR_I  ""
#define _CLR_W  ""
#define _CLR_E  ""
#define _CLR_D  ""
#define _CLR_R  ""
#endif

// fmt 必须是字符串字面量（与前后 ANSI 控制码拼接）
#define LOGI(tag, fmt, ...)  printf(_CLR_I "I (%lu) %s: " fmt _CLR_R "\r\n", (unsigned long)HAL_GetTick(), tag, ##__VA_ARGS__)
#define LOGW(tag, fmt, ...)  printf(_CLR_W "W (%lu) %s: " fmt _CLR_R "\r\n", (unsigned long)HAL_GetTick(), tag, ##__VA_ARGS__)
#define LOGE(tag, fmt, ...)  printf(_CLR_E "E (%lu) %s: " fmt _CLR_R "\r\n", (unsigned long)HAL_GetTick(), tag, ##__VA_ARGS__)
#define LOGD(tag, fmt, ...)  printf(_CLR_D "D (%lu) %s: " fmt _CLR_R "\r\n", (unsigned long)HAL_GetTick(), tag, ##__VA_ARGS__)

#endif /* APP_LOG_H */