# 业务代码注释规范

适用于 stm32-app-main skill 生成的所有业务文件（`main/app_main.c`、`main/app_main_bare.c` 等）。约定源自用户长期偏好，**所有模板都按此规范写**；用户在 `main/` 下加新文件时请继续遵守。

## 决策前提

- 项目**不用 Doxygen** 生成文档
- 注释**不要太臃肿**，强调代码可读性
- 注释**写在右边**（同行尾），不要写在**上面**

## 规范

### 文件头

```c
/**
 * @file    xxx.c
 * @author  宁子希 (1589326497@qq.com)
 * @brief   一句话简介
 * @date    2026-07-XX
 *
 * @copyright Copyright (c) 2026
 *
 */
```

- `app_main.c` 多保留一行 `@version 0.1`（仅此一处）
- 其他业务文件不加 `@version`

### 函数定义

```c
/**
 * @brief 一句话：做什么 + 关键约束/副作用
 *
 * @param  aaa   参数语义（不是机械翻译参数名）
 * @param  bbb   参数语义
 * @return 返回值说明
 */
static void foo(int aaa, int bbb) { ... }
```

- `@brief` 写**做了什么 + 关键约束 / 副作用**，不写"是什么"（函数名已说清）
- `@param` 写参数**语义**，不机械重复参数名

### 宏 / 全局变量

```c
#define WIFI_SSID                        "your-ssid"     // Wi-Fi SSID（2.4GHz）

static volatile uint8_t   g_wifi_connected;              // Wi-Fi 关联成功标志
```

注释**同行尾** `// xxx`，与宏右值或变量类型对齐。

### 行内注释

```c
// 必须在 lwesp_init() 之前注册系统事件回调
if (lwesp_evt_register(...) != lwespOK) { ... }
```

单行 `//`，写**为什么 / 注意点**，不重复代码语义。

## 绝对不要

| ❌ 不要 | 原因 |
|---|---|
| 章节分隔横线 `/*===== Section =====*/` | 臃肿 |
| 教学型多行 `/* */` 或 `//` 注释块 | 臃肿 |
| `@defgroup @{ @}` `@code @endcode` 等 Doxygen 重型标记 | 不用 Doxygen，标记是死重量 |
| 函数注释里机械翻译 `@param aaa 参数 aaa` | 复制签名而已，无信息量 |
| 把变量/宏的注释写在**上方** | 偏好同行尾 |

## 唯一保留的多行注释场景

一行 `//` 装不下关键技术约束时，可在宏上方保留一行 `//`：

```c
// fmt 必须是字符串字面量；依赖 GCC ##__VA_ARGS__ 处理零额外参数
#define LOGI(tag, fmt, ...)  ...
```

## 注释体量红线

- 单行 `//` 注释**不超过一行**（80 字符内能写下就一行）
- 写**一句话事实**（"关 ESP-AT 回显"），**不写讲解**
- 上下文 / 原因 / 历史背景**放 memory / 本文档**，**不放代码注释里**
- 用户硬规则：注释"不要出现讲解型"，所有"为什么这样设计"的答案在本文件能查到

## 模板示例对照

模板 [`assets/app_main.c`](../assets/app_main.c) 与 [`assets/app_main_bare.c`](../assets/app_main_bare.c) 已按本规范生成，新文件请直接参照。
