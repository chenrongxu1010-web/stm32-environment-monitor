# STM32F401RE Environment Monitor

**STM32F401RE Bare-Metal Environment Monitoring and Alarm Terminal**

基于 STM32F401RE 的裸机环境监测与报警终端。使用 AHT20 采集温湿度，通过 SSD1306 OLED 显示和 UART 输出调试信息，使用 W25Q64 SPI Flash 保存报警阈值，配合报警状态机、有源蜂鸣器与 EXTI 按键构成完整应用。

**关键词：** STM32F401RE · Cortex-M4 · C · HAL · STM32CubeIDE · I2C · SPI · UART · EXTI · AHT20 · SSD1306 OLED · W25Q64 SPI Flash

## 硬件平台

| 硬件 | 接口与用途 |
| --- | --- |
| NUCLEO-F401RE / STM32F401RE | ARM Cortex-M4 主控 |
| AHT20 | I2C1：PB8 SCL / PB9 SDA，7-bit 地址 0x38 |
| SSD1306 OLED | 共用 I2C1，7-bit 地址 0x3C |
| W25Q64 SPI Flash | SPI1：PA5 SCK / PA6 MISO / PA7 MOSI，PB6 CS |
| 低电平触发有源蜂鸣器 | PB5 |
| 板载 B1 按键 | PC13，下降沿 EXTI |
| ST-LINK Virtual COM Port | USART2，115200 / 8N1，无流控 |

外设按原工程使用 3.3 V 供电并共地。SPI 配置为 Master、8-bit、MSB First、CPOL Low、CPHA 1 Edge、Software NSS、64 分频。

## 核心功能

- **I2C 驱动 AHT20：** 初始化、测量命令、数据读取与温湿度解析。
- **OLED 显示：** 温度、湿度、NORMAL / ALARM 与报警使能状态。
- **UART 调试输出：** 外设初始化结果、JEDEC ID、配置来源和实时数据。
- **SPI W25Q64 驱动：** JEDEC ID 读取、Write Enable、状态寄存器读取、BUSY 等待、Sector Erase、Page Program、Data Read。
- **参数掉电保存：** 在 Flash 地址 `0x002000` 保存报警阈值，上电读取并校验，读取失败或配置非法时恢复默认参数并尝试保存。当前保存的是配置，尚未实现连续环境数据日志。
- **滞回报警状态机：** 已拆分状态与触发/恢复阈值接口；当前恢复条件存在已知问题，见下文。
- **蜂鸣器控制：** 根据报警状态和使能标志控制低电平触发蜂鸣器。
- **EXTI 按键控制：** 50 ms 软件消抖，中断设置事件标志，主循环切换报警使能。

## 软件结构

采用模块化设计，在职责上分离驱动层、配置层、业务层。保留现有 CubeIDE 工程结构，不为展示移动代码到 Modules 目录。

```text
stm32-environment-monitor/
├── README.md
├── Core/
│   ├── Inc/                  # 驱动和配置头文件
│   ├── Src/                  # 主程序、外设驱动、配置模块
│   └── Startup/              # Cortex-M4 启动文件
├── Application/              # alarm.c / alarm.h
├── Drivers/                  # CMSIS / STM32 HAL 及原许可证
├── Images/                   # 图片目录说明
├── .project / .cproject      # CubeIDE 工程配置
├── .settings/
├── gpio_led01.ioc            # 保留原工程名
├── STM32F401RETX_FLASH.ld
├── STM32F401RETX_RAM.ld
└── .gitignore
```

| 模块 | 文件位置 | 职责 |
| --- | --- | --- |
| aht20.c / .h | Core/Src、Core/Inc | 传感器初始化、I2C 通信、温湿度换算 |
| oled.c / .h | Core/Src、Core/Inc | SSD1306 初始化、页清除、字符显示 |
| w25q64.c / .h | Core/Src、Core/Inc | Flash 命令、擦除、编程、读取 |
| config.c / .h | Core/Src、Core/Inc | 默认阈值、合法性检查、加载与保存 |
| alarm.c / .h | Application | 状态、阈值、使能管理与蜂鸣器控制 |
| main.c | Core/Src | 初始化、主循环调度、串口输出、EXTI 回调 |

## 编译与运行

1. 克隆仓库，在 STM32CubeIDE 中选择 **File → Import → General → Existing Projects into Workspace**，选择仓库根目录。
2. 导入后的工程名仍为 `gpio_led01`。如已有同名工程，请使用新的工作空间。
3. 选择 **Debug** 配置，执行 **Build Project**，通过 ST-LINK 下载运行。
4. 按接口表连接外设，串口终端选择 ST-LINK 对应端口并设置 `115200 / 8N1`。

本次整理使用 STM32CubeIDE 2.2.0 的 headless build，在独立工作空间完成干净构建：**0 errors / 0 warnings**。核心代码、工程配置、启动文件及链接脚本保留原样，构建产物不提交。

串口输出格式示例（非本次实机日志）：

```text
AHT20 Init: OK
OLED Init: OK
W25Q64 JEDEC ID: EF 40 17
Load Config: T_ON=300 T_OFF=290 H_ON=900 H_OFF=850
Config source: FLASH
Temperature: 26.3 C, Humidity: 71.0 %, State: NORMAL
```

## 调试经历：W25Q64 JEDEC ID 异常

同一套代码在不同复位过程中曾出现正常和异常结果：

```text
EF 40 17    正常
FF FF FF   异常
00 00 00   异常
```

排查过程：

1. **SPI 配置：** 核查 Master、CPOL / CPHA、位宽、位序和软件 NSS。
2. **CS 时序：** 核查片选拉低、发送 JEDEC ID 命令、读取数据及片选释放流程。
3. **GPIO：** 核查 PA5 / PA6 / PA7 复用配置与 PB6 片选输出。
4. **供电：** 检查模块电源和 GND 共地。
5. **接线：** 检查 SPI 连接，替换可疑杜邦线。

**最终定位为杜邦线接触问题。** 更换 SPI 使用的杜邦线后，多次复位均稳定读取到 `EF 40 17`。通信异常需要结合软件、协议、GPIO、时序、供电和物理连接逐层定位。

本段保留项目原有真实调试记录；本次仓库整理未重新执行实机测试。

## 当前状态与已知限制

- 默认温度触发/恢复阈值为 `30.0 / 29.0 °C`，湿度为 `90.0 / 85.0 %RH`，内部用 ×10 整数表示。
- **报警恢复条件待修正：** 当前 `Alarm_Update()` 在报警态使用“温度和湿度均达到触发阈值”回到正常态，未使用恢复阈值。预期滞回应在两项均降至各自恢复阈值时恢复，因此当前不能宣称完整滞回行为已验证。此次整理保留原核心逻辑。
- 主循环使用 `HAL_Delay(1000)`，按键业务响应受主循环和外设操作耗时影响。
- 配置尚未加入 Magic Number、版本号、CRC 和断电写入保护。
- 编译通过不代替硬件验证；报警边界和掉电恢复仍需在开发板上复测。

## 后续计划

- 修正并验证报警恢复条件及阈值边界。
- 迁移 FreeRTOS 版本，学习多任务设计，拆分采集、显示、通信和存储任务。
- 学习 Queue / Semaphore 通信与同步，以及共享总线资源的互斥访问。
- 完善配置校验，补充实物照片、串口日志和逻辑分析仪截图。
