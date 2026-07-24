# stm_log 配置参考

`stm_log` 是 skill 通过 FetchContent 拉取的依赖库，工程内可独立配置。
本节列出 skill 默认值与覆盖方式，遇到非 STM32F4 工程必须读这里。

## STM_LOG_HAL_HEADER（v2.3.1+ 必须配置）

`stm_log` 依赖 STM32 HAL 提供 `UART_HandleTypeDef` / `HAL_UART_Transmit` / `HAL_GetTick`。
不同 STM32 系列的 HAL 头文件名不同，必须在工程 CMake 中显式指定。

STM32CubeMX CMake 工程在 [cmake/stm32cubemx/CMakeLists.txt](cmake/stm32cubemx/CMakeLists.txt)
中自动写入子型号宏（如 `STM32G030xx`），但不暴露家族宏给 skill，所以**skill 默认按 F4 写**。

非 F4 工程必须覆盖。

### 家族对应表

| STM32 家族 | `STM_LOG_HAL_HEADER` |
|---|---|
| STM32F0  | `"stm32f0xx_hal.h"`  |
| STM32F1  | `"stm32f1xx_hal.h"`  |
| STM32F2  | `"stm32f2xx_hal.h"`  |
| STM32F3  | `"stm32f3xx_hal.h"`  |
| STM32F4  | `"stm32f4xx_hal.h"`  |  ← 默认
| STM32F7  | `"stm32f7xx_hal.h"`  |
| STM32G0  | `"stm32g0xx_hal.h"`  |
| STM32G4  | `"stm32g4xx_hal.h"`  |
| STM32H5  | `"stm32h5xx_hal.h"`  |
| STM32H7  | `"stm32h7xx_hal.h"`  |
| STM32L0  | `"stm32l0xx_hal.h"`  |
| STM32L1  | `"stm32l1xx_hal.h"`  |
| STM32L4  | `"stm32l4xx_hal.h"`  |
| STM32L5  | `"stm32l5xx_hal.h"`  |
| STM32U5  | `"stm32u5xx_hal.h"`  |
| STM32WB  | `"stm32wbxx_hal.h"`  |
| STM32WL  | `"stm32wlxx_hal.h"`  |
| STM32C0  | `"stm32c0xx_hal.h"`  |

### 工程根 CMake 写法

在 `FetchContent_MakeAvailable(stm_log)` 之后追加：

```cmake
FetchContent_MakeAvailable(stm_log)
target_compile_definitions(stm_log PUBLIC
    "STM_LOG_HAL_HEADER=\"stm32g0xx_hal.h\""
)
```

注意 CMake 转义：`STM_LOG_HAL_HEADER` 在头文件中通过
`#include STM_LOG_HAL_HEADER` 展开为 `#include "stm32g0xx_hal.h"`，所以宏的
值必须是带引号的字符串字面量。CMake 中 `target_compile_definitions` 会剥一层
引号，所以传入 `"STM_LOG_HAL_HEADER=\"stm32g0xx_hal.h\""`。最终编译命令
里宏的实际定义为 `STM_LOG_HAL_HEADER="stm32g0xx_hal.h"`。

### 用 CACHE 写法（可选）

```cmake
set(STM_LOG_HAL_HEADER "stm32g0xx_hal.h" CACHE STRING "" FORCE)
FetchContent_MakeAvailable(stm_log)
```

这种写法 `set(...)` 必须放在 `FetchContent_MakeAvailable()` 之前才生效。

### 失败信号

如果忘了覆盖，构建会爆：

```text
stm_log_config.h:24:#include STM_LOG_HAL_HEADER
stm_log.h: fatal error: stm32f4xx_hal.h: No such file or directory
```

或者：

```text
fatal error: stm_log: HAL header not found.
```

## STM_LOG_LINK_CUBEMX（v2.3.1+ 默认 ON）

库默认自动 link CubeMX 生成的 `stm32cubemx` target。
非 CubeMX 工程、自定义 HAL target 命名时可关：

```cmake
set(STM_LOG_LINK_CUBEMX OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(stm_log)
target_link_libraries(${CMAKE_PROJECT_NAME} stm_log your_hal_target)
```

## 必读警告

如果你的工程是 STM32G0/STM32H7/STM32U5 等非 F4 系列，**默认模板在 §3 之后必须
追加本节 STM_LOG_HAL_HEADER 配置**，否则构建失败。
