# FreeRTOS 设计与实现边界

依据 `Core/Src/main.c`、`FreeRTOSConfig.h` 和随仓库提供的 `cmsis_os2.c`。内核 V10.3.1，抢占调度、1000 Hz tick、heap_4，动态堆 15360 字节。HAL 的毫秒时基改用 TIM5，内核使用 SysTick。

## 五任务

| 任务 | 优先级 | 栈配置 | 行为 |
| --- | --- | ---: | --- |
| SensorTask | Normal | 1024 bytes | 持锁读取 AHT20；成功后向三个队列分别投递；`osDelay(1000)` |
| DisplayTask | Normal | 1024 bytes | 等待显示队列，持锁刷新 OLED |
| AlarmTask | AboveNormal | 1024 bytes | 等待报警队列，更新滞回状态与蜂鸣器 |
| UartTask | Normal | 1024 bytes | 等待串口队列，监测五个任务栈空间并输出温湿度 |
| ButtonTask | Normal | 1024 bytes | 等待二值信号量，切换报警使能 |

`osThreadNew` 在启动内核前创建任务。提高 AlarmTask 优先级是为了数据到达后优先处理报警，不代表已经测得响应时间上界。采集周期是工作耗时加 1000 tick，相对延时不等于严格 1 Hz。

## 三路队列

`osMessageQueueNew(5, 8, ...)`：每路 5 个元素、每个 8 字节，对应 `SensorData_t`。`osMessageQueuePut(..., 0, 0)` 非阻塞；满时丢弃本次新消息，已有消息仍保留。三路投递互不等待，任一路失败都不阻止其他投递。

`displayQueueDropCount`、`alarmQueueDropCount`、`uartQueueDropCount` 对所有非 osOK 返回计数，并非只统计满队列。压力模式另行区分 `osErrorResource` 与其他错误。消费者使用 `osMessageQueueGet(..., osWaitForever)`，无数据时阻塞，不轮询空队列。

## I2C 资源保护

SensorTask 在 `osMutexAcquire` 后调用 AHT20，完成后 `osMutexRelease`；DisplayTask 对整次 OLED 刷新加同一把锁。AHT20 的 80 ms `HAL_Delay` 仍在锁内，当前属于忙等待，增加锁占用和 CPU 负担。驱动迁移为任务阻塞等待或异步状态机属于后续优化，当前没有自动总线恢复。

I2C Mutex 属性只有名称，未显式请求 `osMutexPrioInherit`。压力模式的 UART Mutex 显式使用该属性。不要把 I2C 锁描述成已经验证优先级继承效果。

## 按键与中断

PC13 下降沿 EXTI，NVIC 优先级为 5；内核允许调用中断安全 API 的阈值配置也是 5。回调用 HAL tick 做 50 ms 间隔过滤，然后 `osSemaphoreRelease`。`osSemaphoreNew(1,0,...)` 初始无事件；ButtonTask 用 `osSemaphoreAcquire(...,osWaitForever)` 等待。连续事件可能合并，当前没有按键丢失计数。

## 栈与初始化检查

UartTask 每收到一帧数据，调用 `osThreadGetStackSpace` 更新五个 `*StackFree` 变量。仓库适配层将 `uxTaskGetStackHighWaterMark` 乘 `sizeof(StackType_t)`，返回字节数，表示历史最小未使用栈空间，不是瞬时剩余值。正常串口输出未包含这些变量，需调试器观察或后续增加遥测。传感器不产出数据时监测也不更新。

启动前检查任务、队列、Mutex、Semaphore 和事件对象句柄；失败输出 `APP_RTOS_INIT_FAILED` 并进入错误处理。尚未启用专用 malloc-failed / stack-overflow hook，也未实现持续 heap 遥测。

## Event Flags

代码创建并检查 `systemEventFlagsHandle`，但没有业务 `Set/Wait` 调用。独立学习中的 Event Flags 不计为主项目已实现同步功能。

## 压力模式

`QUEUE_STRESS_TEST=1` 保留真实采集和消费者，依次 Warmup 3 s、Pressure 18 s、Drain 6 s、Recovery 4 s。压力阶段消费者完成实际工作且释放 Mutex 后延时 3 s，生产者继续采集。记录容量、峰值、投递、满队列、错误和丢弃增量，输出 `Q_LOAD`、`Q_RECOVERY` 与最终结果。

当前 Drain 阶段恢复消费者速度，但生产者仍投递；队列深度快照等于 0 并不构成“停止投递后排空”的严格证明。真实测试尚待执行，须区分调度时序造成的非零快照与消费者无法恢复。正常业务默认关闭此模式，Flash 也保持隔离。
