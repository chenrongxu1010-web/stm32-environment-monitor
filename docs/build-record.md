# 构建与来源核对

2026-11-22，STM32CubeIDE 2.2.0 headless build，在独立 Eclipse 工作空间从发布副本执行 `-cleanBuild gpio_led01/Debug`。未连接、烧录开发板。每次均实际编译并链接。

| 模式 | 额外编译宏 | 结果 | text / data / bss (bytes) |
| --- | --- | --- | --- |
| [normal](build-normal.txt) | 无 | 0 errors / 0 warnings | 41172 / 112 / 21272 |
| [queue-stress](build-queue-stress.txt) | QUEUE_STRESS_TEST=1 | 0 errors / 0 warnings | 43744 / 112 / 21432 |
| [flash-capture](build-flash-capture.txt) | FLASH_READ_DIAG_MODE=1; FLASH_SPI_CAPTURE_MODE=1 | 0 errors / 0 warnings | 22264 / 96 / 3360 |

text/data/bss 是链接器尺寸统计，不是运行时栈或堆峰值。只读诊断模式在调度前运行，不可拿其较小尺寸代表正常五任务内存用量。

## 复现

在已安装 CubeIDE 的命令环境中调用 `headless-build.bat -data <独立工作空间> -import <工程file URI> -cleanBuild gpio_led01/Debug`；压力模式追加 `-D QUEUE_STRESS_TEST=1`，捕获模式追加两个对应 `-D`。Windows 导入参数使用 `file:/C:/.../project/` URI，避免盘符被解析为文件系统 scheme。构建不会自动烧录。

## 同步边界

原本地开发目录未配置 Git remote，且含未提交修改。本次读取并复制源码、Cube 配置和中间件到已核实远程的发布副本，不更改原工作区或其 Git 状态。目标为 `chenrongxu1010-web/stm32-environment-monitor`，基线为 `0fec1c4`。

同步变化包括：main.c 五任务、三路队列、监测与诊断；alarm.c 恢复逻辑修正；w25q64.c/.h 状态寄存器、WEL 与 BUSY 超时；FreeRTOSConfig.h、freertos.c、内核中间件；HAL 配置、TIM5 时基、MSP、中断和 .ioc / .cproject。AHT20、OLED、Config 原模块结构保留。

`source-snapshot.json` 保存本次原始源文件 SHA-256 清单。清单为复制时的字节级来源快照；Git 可能规范化 CRLF，不应把换行变化误判为语义修改。除调试启动文件的日志路径改为工作空间相对引用外，本次未自行改写业务算法。

构建产物留在忽略的 Debug 目录，未提交；保留必要的 CubeIDE 工程文件、HAL/CMSIS/FreeRTOS 版权声明。发布前检查文本编码、Markdown 相对链接、图片路径及敏感值；不存在凭空补写的真机结果。

差异空白检查：新增文档已清理行尾空格；原始 Cube 生成文件和 FreeRTOS 第三方文件保留上游空白格式，未为格式检查重写供应商源码。Mermaid 图按源码数据流核对，GitHub Markdown 转换通过；未进行浏览器像素级检查。
