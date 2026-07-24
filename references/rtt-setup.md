# SEGGER RTT 库接入说明

RTT 后端 (`stm_log_init_output(rtt_output, level)`, stm_log v2.3.0+) 依赖 SEGGER 官方的 RTT Target Sources —— 本 skill **通过 CMake FetchContent 拉取**（与 stm_log 同模式），源码落在 `<root>/Lib/segger_rtt/`。

## 关注点分离（设计要点）

```
Lib/segger_rtt/                          ← SEGGER RTT 源路径
├── CMakeLists_rtt.txt                   ← 把 RTT 包装成 add_library(segger_rtt STATIC ...) target
├── RTT/                                 ← 编译时实际 link 的源
│   ├── SEGGER_RTT.c / .h / .S / _ConfDefaults.h / _printf.c
├── Config/SEGGER_RTT_Conf.h             ← 空文件,默认配置走 ConfDefaults.h
└── LICENSE.md / README.md

main/                                    ← 业务模块
├── CMakeLists.txt                       ← 只 link stm_log + segger_rtt(target),不管 RTT 源路径
└── app_main.c                           ← 含 rtt_output callback + stm_log_init_output
```

**关键**:RTT 源全部在 `Lib/segger_rtt/` 里,`main/CMakeLists.txt` 只 link `segger_rtt` target,不提 RTT 源 — 改 RTT 版本只动 `CMakeLists_rtt.txt` 一处。

## 一次性准备:用户 fork 仓库的精简

`NingZiXi/RTT` 或 `nzxhg/RTT.git`(用户的 fork 仓库)需要做两件事:

### ① 精简:删掉不用的 SEGGER 工程文件

SEGGER RTT 上游仓库(`SEGGERMicro/RTT`)含多个不需要的部分,精简到只保留核心:

```
RTT/                     ← 全留
  ├── SEGGER_RTT.c
  ├── SEGGER_RTT.h
  ├── SEGGER_RTT_ConfDefaults.h
  ├── SEGGER_RTT_ASM_ARMv7M.S
  └── SEGGER_RTT_printf.c
Config/SEGGER_RTT_Conf.h  ← 全留(空文件即可)
LICENSE.md                ← 全留(license 要求)
README.md                 ← 全留(可选)
```

**删除**(不进 build,不进工程仓库):
```
Examples/                 ← 4 个示例 .c + 1 个_menu 等
Syscalls/                 ← 4 个 *_Syscalls_*.c(把 printf 重定向到 RTT 用的,我们用 stm_log 不需要)
SEGGER.RTT.pdsc           ← CMSIS-Pack 配置
gen_pack.sh               ← pack 生成脚本
```

### ② 加 `CMakeLists_rtt.txt`

把下列内容放到 fork 仓库的 `CMakeLists_rtt.txt`(仓库根):

```cmake
cmake_minimum_required(VERSION 3.22)

set(_segger_rtt_dir ${CMAKE_CURRENT_LIST_DIR})

add_library(segger_rtt STATIC
    ${_segger_rtt_dir}/RTT/SEGGER_RTT.c
    ${_segger_rtt_dir}/RTT/SEGGER_RTT_ASM_ARMv7M.S
)

target_include_directories(segger_rtt PUBLIC
    ${_segger_rtt_dir}/RTT
    ${_segger_rtt_dir}/Config
)
```

> ⚠ **必须用 `CMAKE_CURRENT_LIST_DIR`,不能用 `CMAKE_CURRENT_SOURCE_DIR`**。`include()` 不切换 `CMAKE_CURRENT_SOURCE_DIR`(只有 `add_subdirectory()` 才切),所以相对路径 `RTT/SEGGER_RTT.c` 会找不到。`CMAKE_CURRENT_LIST_DIR` 总是反映**当前正在处理的文件**所在目录,与调用方式无关。

### ③ commit + push

```bash
cd <fork-checkout>
git rm -r Examples Syscalls SEGGER.RTT.pdsc gen_pack.sh    # 精简
# 把上面 CMakeLists_rtt.txt 内容存到仓库根
git add .
git commit -m "refactor: 精简到 RTT 核心 + 加 CMakeLists_rtt.txt"
git push origin main                                       # 或对应分支
```

### ④(可选)打 tag 跟 SEGGER 上游对齐

```bash
git tag V8.58 4d8feab3150f86f37a9d323ddc88d6cdf5673072     # 跟 SEGGER 上游 V8.58 对齐
git push origin V8.58
```

让 FetchContent 用 tag 锁版本,而不是分支 HEAD(防止上游 merge 引入非预期破坏)。

## FetchContent 默认配置（根 CMakeLists.txt）

```cmake
FetchContent_Declare(
    segger_rtt
    GIT_REPOSITORY https://gitee.com/nzxhg/RTT.git        # 默认 Gitee 镜像(用户 fork)
    #GIT_REPOSITORY https://github.com/NingZiXi/RTT.git   # GitHub 直连备选
    #GIT_REPOSITORY https://github.com/SEGGERMicro/RTT.git # SEGGER 上游(没简化+没 CMakeLists,仅做备份)
    GIT_TAG        V8.58                                  # 用户 fork 打 V8.58 tag 时用,否则改成 SHA
    SOURCE_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/Lib/segger_rtt
)
FetchContent_MakeAvailable(segger_rtt)
include(${CMAKE_CURRENT_SOURCE_DIR}/Lib/segger_rtt/CMakeLists_rtt.txt)   # 包装 segger_rtt target
```

## main/CMakeLists.txt 关键写法

```cmake
target_link_libraries(${CMAKE_PROJECT_NAME} stm_log)        # 必
if(TARGET segger_rtt)                                       # RTT 分支才有
    target_link_libraries(${CMAKE_PROJECT_NAME} segger_rtt)
endif()
```

`if(TARGET segger_rtt)` 让 UART 分支不编译 SEGGER_RTT 任何源,完全干净。

## 路径约定(精简后)

```
Lib/segger_rtt/
├── CMakeLists_rtt.txt                ← 包装 target
├── RTT/                              ← 编译时实际 link
│   ├── SEGGER_RTT.c
│   ├── SEGGER_RTT.h
│   ├── SEGGER_RTT_ConfDefaults.h
│   └── SEGGER_RTT_ASM_ARMv7M.S       ← ARMv7-M 加速
├── Config/
│   └── SEGGER_RTT_Conf.h             ← 空文件,默认配置在 ConfDefaults.h
├── LICENSE.md
└── README.md
```

`.gitignore` 排除(`Lib/segger_rtt/` 下不应该出现的)目录已经在根 `.gitignore` 里:

```
Lib/segger_rtt/Examples/
Lib/segger_rtt/Syscalls/
Lib/segger_rtt/SEGGER.RTT.pdsc
Lib/segger_rtt/gen_pack.sh
```

## 验证

```bash
test -f <root>/Lib/segger_rtt/RTT/SEGGER_RTT.c && echo OK || echo MISSING
test -f <root>/Lib/segger_rtt/RTT/SEGGER_RTT.h && echo OK || echo MISSING
test -f <root>/Lib/segger_rtt/Config/SEGGER_RTT_Conf.h && echo OK || echo MISSING
test -f <root>/Lib/segger_rtt/CMakeLists_rtt.txt && echo OK || echo MISSING       ← 用户 fork 必须有
```

任一缺失会报:
- `undefined reference to SEGGER_RTT_Init` —— RTT 源没编译
- `fatal error: SEGGER_RTT.h: No such file` —— 头文件路径错
- `fatal error: SEGGER_RTT_Conf.h: No such file` —— 同上
- `Cannot find source file: CMakeLists_rtt.txt` —— **用户 fork 仓库没加这个文件**(按 §1.6 步骤加)

## 镜像选择

| 场景 | GIT_REPOSITORY |
|---|---|
| 默认（国内用户，Gitee 通常可达） | `https://gitee.com/nzxhg/RTT.git`（**用户 fork**,精简版） |
| GitHub 直连 / 境外 | `https://github.com/NingZiXi/RTT.git`（**用户 fork**,精简版） |
| SEGGER 官方仓库（权威,不一定国内可达） | `https://github.com/SEGGERMicro/RTT.git`（未简化,无 CMakeLists） |

`SEGGERMicro/RTT` 镜像**不能直接用** —— 没精简 + 没 CMakeLists_rtt.txt,build 会失败。

## 版本选择

SEGGER 官方 RTT tag:
```
V7.54
V8.58 / V8.58.0
```

最新 HEAD 对应 `V8.58`(SHA `4d8feab3150f86f37a9d323ddc88d6cdf5673072`)。

> **注意**:用户 fork 仓库必须 `git push origin V8.58` 把 tag 也同步过去(否则 `GIT_TAG V8.58` 会失败)。
>
> 解法:
> - 在自己 fork 上 `git push origin V8.58` 同步 tag
> - 或把 `GIT_TAG V8.58` 改成 SHA `4d8feab3150f86f37a9d323ddc88d6cdf5673072` 锁定

## 离线 / 代理环境

手动 `git clone https://gitee.com/nzxhg/RTT.git <root>/Lib/segger_rtt`(或 GitHub 同),把 fork 仓库克隆下来。FetchContent 检测到目录已存在则跳过拉取。

## 与 stm_log 库的关系

stm_log 库本身**不知道** RTT 存在,只提供 callback 注入接口。我们 RTT 分支做的事情就是:

1. 写一个 `rtt_output(buf, len)` 调 `SEGGER_RTT_Write(0, buf, len)`
2. 在 `app_main()` 里 `stm_log_init_output(rtt_output, STM_LOG_LVL_INFO)` 一步完成 init + 切换后端(v2.3.0+)

所以 stm_log 是 UART 还是 RTT 都一样,纯靠 callback 切换。RTT 升级不会影响 stm_log,反之亦然。

> **stm_log 版本要求**:RTT 后端模板用 `stm_log_init_output`,需要 stm_log **v2.3.0+**。如果工程的 FetchContent 还停在 v2.2.0,会编译失败 `undefined reference to stm_log_init_output` —— 把根 `CMakeLists.txt` 的 `GIT_TAG v2.2.0` 改成 `v2.3.1` 即可。

## Lib/segger_rtt 是否要 commit 进 git

**建议 commit**。FetchContent 自动拉到本地,clone 后**无需** `git submodule update` 即可直接 build。`.gitignore` 排除 `Examples/Syscalls/.pdsc/gen_pack.sh`,只 commit `RTT/ + Config/ + LICENSE/README + CMakeLists_rtt.txt`(精简版)进 git。