/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include<stdio.h>
#include <string.h>
#include <stdarg.h>

#include "oled.h"
#include "aht20.h"
#include "w25q64.h"
#include "config.h"
#include "alarm.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

typedef struct
{
    int32_t temperature_x10;
    uint32_t humidity_x10;

} SensorData_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* Keep external configuration isolated until reliability is verified. */
#ifndef APP_USE_FLASH_CONFIG
#define APP_USE_FLASH_CONFIG 0U
#endif

#ifndef QUEUE_STRESS_TEST
#define QUEUE_STRESS_TEST 0U
#endif

#define FLASH_CONFIG_WRITE_TEST 0U
#define FLASH_RAW_TEST 0U
#ifndef FLASH_READ_DIAG_MODE
#define FLASH_READ_DIAG_MODE 0U
#endif

#ifndef FLASH_SPI_CAPTURE_MODE
#define FLASH_SPI_CAPTURE_MODE 0U
#endif

#if !APP_USE_FLASH_CONFIG && (FLASH_RAW_TEST || FLASH_CONFIG_WRITE_TEST)
#error "Flash write tests require an explicitly enabled application Flash path"
#endif

#if QUEUE_STRESS_TEST && (APP_USE_FLASH_CONFIG || FLASH_READ_DIAG_MODE || FLASH_RAW_TEST || FLASH_CONFIG_WRITE_TEST)
#error "Queue stress must run the normal tasks with external Flash isolated"
#endif

#if FLASH_SPI_CAPTURE_MODE && !FLASH_READ_DIAG_MODE
#error "SPI capture mode requires the read-only Flash diagnostic"
#endif

#if FLASH_READ_DIAG_MODE && (FLASH_RAW_TEST || FLASH_CONFIG_WRITE_TEST)
#error "Flash read diagnostic must be built with both write tests disabled"
#endif

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart2;

/* Definitions for SensorTask */
osThreadId_t SensorTaskHandle;
const osThreadAttr_t SensorTask_attributes = {
  .name = "SensorTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for DisplayTask */
osThreadId_t DisplayTaskHandle;
const osThreadAttr_t DisplayTask_attributes = {
  .name = "DisplayTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for AlarmTask */
osThreadId_t AlarmTaskHandle;
const osThreadAttr_t AlarmTask_attributes = {
  .name = "AlarmTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};
/* Definitions for UartTask */
osThreadId_t UartTaskHandle;
const osThreadAttr_t UartTask_attributes = {
  .name = "UartTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for ButtonTask */
osThreadId_t ButtonTaskHandle;
const osThreadAttr_t ButtonTask_attributes = {
  .name = "ButtonTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for sensorDisplayQueue */
osMessageQueueId_t sensorDisplayQueueHandle;
const osMessageQueueAttr_t sensorDisplayQueue_attributes = {
  .name = "sensorDisplayQueue"
};
/* Definitions for sensorAlarmQueue */
osMessageQueueId_t sensorAlarmQueueHandle;
const osMessageQueueAttr_t sensorAlarmQueue_attributes = {
  .name = "sensorAlarmQueue"
};
/* Definitions for sensorUartQueue */
osMessageQueueId_t sensorUartQueueHandle;
const osMessageQueueAttr_t sensorUartQueue_attributes = {
  .name = "sensorUartQueue"
};
/* Definitions for i2cMutex */
osMutexId_t i2cMutexHandle;
const osMutexAttr_t i2cMutex_attributes = {
  .name = "i2cMutex"
};
/* Definitions for buttonSem */
osSemaphoreId_t buttonSemHandle;
const osSemaphoreAttr_t buttonSem_attributes = {
  .name = "buttonSem"
};
/* Definitions for systemEventFlags */
osEventFlagsId_t systemEventFlagsHandle;
const osEventFlagsAttr_t systemEventFlags_attributes = {
  .name = "systemEventFlags"
};
/* USER CODE BEGIN PV */

static volatile uint8_t button_pressed = 0;

static char tx_buffer[64];

static char oled_temp[16];
static char oled_humi[16];
static char oled_state[16];

static float humidity = 0.0f;
static float temperature = 0.0f;

static int32_t temperature_x10 = 0;
static uint32_t humidity_x10 = 0;

static uint8_t oled_ready = 0U;

static volatile uint32_t displayQueueDropCount = 0;
static volatile uint32_t alarmQueueDropCount = 0;
static volatile uint32_t uartQueueDropCount = 0;

/* FreeRTOS Task Stack Monitoring */
volatile uint32_t sensorStackFree = 0;
volatile uint32_t displayStackFree = 0;
volatile uint32_t alarmStackFree = 0;
volatile uint32_t uartStackFree = 0;
volatile uint32_t buttonStackFree = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM2_Init(void);
static void MX_ADC1_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI1_Init(void);
void StartSensorTask(void *argument);
void StartDisplayTask(void *argument);
void StartAlarmTask(void *argument);
void StartUartTask(void *argument);
void StartButtonTask(void *argument);

/* USER CODE BEGIN PFP */


/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
#if QUEUE_STRESS_TEST
typedef enum {
    QueueWarmup, QueuePressure, QueueDrain, QueueRecovery, QueueDone
} QueueStressPhase;
typedef enum {
    QueueDisplay, QueueAlarm, QueueUart, QueueCount
} QueueStressIndex;

typedef struct {
    uint32_t capacity;
    uint32_t peak;
    uint32_t attempts;
    uint32_t accepted;
    uint32_t full;
    uint32_t errors;
} QueueStressStats;

static osMutexId_t queueStressUartMutex;
static volatile QueueStressPhase queueStressPhase = QueueWarmup;
static volatile uint32_t queueStressProcessed[QueueCount];
static QueueStressStats queueStressStats[QueueCount];
static uint32_t queueStressStartDrops[QueueCount];
static uint32_t queueStressStartProcessed[QueueCount];
static uint32_t queueStressRecoveryErrors[QueueCount];
static uint32_t queueStressRecoveryAccepted[QueueCount];
static uint32_t queueStressDrainDepth[QueueCount];
static uint32_t queueStressMark;
static uint32_t queueStressTicksPerSec;
static uint8_t queueStressPressureOk;

static void QueueStress_Uart(const char *data, uint16_t size)
{
    if (osMutexAcquire(queueStressUartMutex, osWaitForever) == osOK)
    {
        HAL_UART_Transmit(&huart2, (const uint8_t *)data, size, 100U);
        osMutexRelease(queueStressUartMutex);
    }
}

static void QueueStress_Log(const char *format, ...) __attribute__((format(printf, 1, 2)));
static void QueueStress_Log(const char *format, ...)
{
    char line[144];
    va_list args;
    va_start(args, format);
    int length = vsnprintf(line, sizeof(line), format, args);
    va_end(args);
    if (length > 0 && length < (int)sizeof(line))
    {
        QueueStress_Uart(line, (uint16_t)length);
    }
}

static void QueueStress_RecordPut(QueueStressIndex index, osMessageQueueId_t queue,
                                  osStatus_t status)
{
    if (queueStressPhase == QueuePressure)
    {
        QueueStressStats *stat = &queueStressStats[index];
        ++stat->attempts;
        if (status == osOK)
        {
            ++stat->accepted;
        }
        else if (status == osErrorResource)
        {
            ++stat->full;
        }
        else
        {
            ++stat->errors;
        }
        /* A non-blocking put rejected as full is evidence of reaching capacity. */
        uint32_t depth = (status == osErrorResource) ? stat->capacity : osMessageQueueGetCount(queue);
        if (depth > stat->peak)
        {
            stat->peak = depth;
        }
    }
    if (queueStressPhase == QueueRecovery)
    {
        if (status == osOK)
        {
            ++queueStressRecoveryAccepted[index];
        }
        else
        {
            ++queueStressRecoveryErrors[index];
        }
    }
}

static void QueueStress_Consumed(QueueStressIndex index)
{
    ++queueStressProcessed[index];
    if (queueStressPhase == QueuePressure)
    {
        /* Delay only after the real consumer work and after releasing its mutex. */
        osDelay(3U * queueStressTicksPerSec);
    }
}

/* SensorTask owns test transitions, statistics, and the queue producer. */
static void QueueStress_Step(void)
{
    if (queueStressPhase == QueueDone)
    {
        return;
    }

    const osMessageQueueId_t queues[QueueCount] = {
        sensorDisplayQueueHandle, sensorAlarmQueueHandle, sensorUartQueueHandle
    };
    const char *const names[QueueCount] = {"display", "alarm", "uart"};
    const uint32_t drops[QueueCount] = {
        displayQueueDropCount, alarmQueueDropCount, uartQueueDropCount
    };
    uint32_t now = osKernelGetTickCount();
    if (queueStressTicksPerSec == 0U)
    {
        queueStressTicksPerSec = osKernelGetTickFreq();
        queueStressMark = now;
        QueueStress_Log("QUEUE_STRESS_BEGIN v=1 warm_s=3 pressure_s=18 slow_s=3 drain_s=6 recovery_s=4\r\n");
    }
    uint32_t elapsed = now - queueStressMark;

    if (queueStressPhase == QueueWarmup && elapsed >= 3U * queueStressTicksPerSec)
    {
        QueueStress_Log("Q_BEFORE drops=%lu,%lu,%lu\r\n",
                        (unsigned long)drops[QueueDisplay], (unsigned long)drops[QueueAlarm],
                        (unsigned long)drops[QueueUart]);
        for (unsigned i = 0; i < QueueCount; ++i)
        {
            queueStressStats[i].capacity = osMessageQueueGetCapacity(queues[i]);
            queueStressStartDrops[i] = drops[i];
            queueStressStartProcessed[i] = queueStressProcessed[i];
        }
        queueStressMark = osKernelGetTickCount();
        queueStressPhase = QueuePressure;
    }
    else if (queueStressPhase == QueuePressure && elapsed >= 18U * queueStressTicksPerSec)
    {
        uint32_t pressureProcessed[QueueCount];
        for (unsigned i = 0; i < QueueCount; ++i)
        {
            pressureProcessed[i] = queueStressProcessed[i];
        }
        queueStressPhase = QueueDrain;
        queueStressMark = now;
        queueStressPressureOk = 1U;
        for (unsigned i = 0; i < QueueCount; ++i)
        {
            const QueueStressStats *stat = &queueStressStats[i];
            uint32_t lost = drops[i] - queueStressStartDrops[i];
            uint32_t handled = pressureProcessed[i] - queueStressStartProcessed[i];
            unsigned pass = stat->capacity == 5U && stat->peak == stat->capacity &&
                            stat->full > 0U && lost == stat->full && stat->errors == 0U &&
                            stat->attempts == stat->accepted + stat->full && handled > 0U;
            queueStressPressureOk &= pass;
            QueueStress_Log("Q_LOAD %s cap=%lu peak=%lu attempts=%lu ok=%lu full=%lu err=%lu drops=%lu done=%lu pass=%u\r\n",
                            names[i], (unsigned long)stat->capacity, (unsigned long)stat->peak,
                            (unsigned long)stat->attempts, (unsigned long)stat->accepted,
                            (unsigned long)stat->full, (unsigned long)stat->errors,
                            (unsigned long)lost, (unsigned long)handled, pass);
        }
    }
    else if (queueStressPhase == QueueDrain && elapsed >= 6U * queueStressTicksPerSec)
    {
        for (unsigned i = 0; i < QueueCount; ++i)
        {
            queueStressDrainDepth[i] = osMessageQueueGetCount(queues[i]);
            queueStressStartDrops[i] = drops[i];
            queueStressStartProcessed[i] = queueStressProcessed[i];
        }
        queueStressMark = now;
        queueStressPhase = QueueRecovery;
    }
    else if (queueStressPhase == QueueRecovery && elapsed >= 4U * queueStressTicksPerSec)
    {
        queueStressPhase = QueueDone;
        unsigned recoveryOk = 1U;
        for (unsigned i = 0; i < QueueCount; ++i)
        {
            uint32_t lost = drops[i] - queueStressStartDrops[i];
            uint32_t handled = queueStressProcessed[i] - queueStressStartProcessed[i];
            unsigned pass = queueStressDrainDepth[i] == 0U && lost == 0U &&
                            queueStressRecoveryErrors[i] == 0U &&
                            queueStressRecoveryAccepted[i] >= 2U && handled >= 2U;
            recoveryOk &= pass;
            QueueStress_Log("Q_RECOVERY %s depth=%lu ok=%lu err=%lu drops=%lu done=%lu pass=%u\r\n",
                            names[i], (unsigned long)queueStressDrainDepth[i],
                            (unsigned long)queueStressRecoveryAccepted[i],
                            (unsigned long)queueStressRecoveryErrors[i],
                            (unsigned long)lost, (unsigned long)handled, pass);
        }
        QueueStress_Log("QUEUE_STRESS_END result=%s drops=%lu,%lu,%lu\r\n",
                        queueStressPressureOk && recoveryOk ? "PASS" : "FAIL",
                        (unsigned long)drops[QueueDisplay], (unsigned long)drops[QueueAlarm],
                        (unsigned long)drops[QueueUart]);
    }
}
#endif
#if FLASH_READ_DIAG_MODE
static void FlashDiag_Print(const char *format, ...)
{
    char line[160];
    va_list args;
    va_start(args, format);
    int length = vsnprintf(line, sizeof(line), format, args);
    va_end(args);
    if (length > 0 && length < (int)sizeof(line))
    {
        HAL_UART_Transmit(&huart2, (uint8_t *)line, (uint16_t)length, 100U);
    }
}

static __attribute__((noreturn)) void FlashDiag_Stop(void)
{
    HAL_SuspendTick();
    for (;;)
    {
        __WFI();
    }
}

#if FLASH_SPI_CAPTURE_MODE
static __attribute__((noreturn)) void FlashDiag_Capture(void)
{
    enum { ReadCount = 100, ReadLength = 8, GapMs = 4 };
    struct {
        HAL_StatusTypeDef status;
        uint32_t offset_ms;
        uint8_t data[ReadLength];
    } samples[ReadCount];
    unsigned batch = 0;

    for (;;)
    {
        uint8_t request = 0;
        FlashDiag_Print("SPI_CAPTURE_READY cmd=R addr=0x%06lX count=%u gap_ms=%u\r\n",
                        (unsigned long)FLASH_CONFIG_ADDR, (unsigned)ReadCount, (unsigned)GapMs);
        do
        {
            request = 0;
        } while (HAL_UART_Receive(&huart2, &request, 1, 100U) != HAL_OK || request != 'R');

        /* The host starts the analyzer before sending R. */
        HAL_Delay(50U);
        uint32_t start = HAL_GetTick();
        for (unsigned i = 0; i < ReadCount; ++i)
        {
            memset(samples[i].data, 0, sizeof(samples[i].data));
            samples[i].offset_ms = HAL_GetTick() - start;
            samples[i].status = W25Q64_ReadData(FLASH_CONFIG_ADDR, samples[i].data, ReadLength);
            if (i + 1U < ReadCount)
            {
                HAL_Delay(GapMs);
            }
        }

        unsigned failures = 0;
        unsigned changed = 0;
        unsigned reference_ok = samples[0].status == HAL_OK;
        FlashDiag_Print("SPI_CAPTURE_BEGIN batch=%u count=%u elapsed_ms=%lu\r\n",
                        ++batch, (unsigned)ReadCount, (unsigned long)(HAL_GetTick() - start));
        for (unsigned i = 0; i < ReadCount; ++i)
        {
            const uint8_t *data = samples[i].data;
            FlashDiag_Print("READ %03u: %02X %02X %02X %02X %02X %02X %02X %02X HAL=%u t_ms=%lu\r\n",
                            i + 1U, (unsigned)data[0], (unsigned)data[1],
                            (unsigned)data[2], (unsigned)data[3], (unsigned)data[4],
                            (unsigned)data[5], (unsigned)data[6], (unsigned)data[7],
                            (unsigned)samples[i].status, (unsigned long)samples[i].offset_ms);
            if (samples[i].status != HAL_OK)
            {
                ++failures;
            }
            else if (reference_ok && memcmp(samples[0].data, data, ReadLength) != 0)
            {
                ++changed;
            }
        }
        FlashDiag_Print("SPI_CAPTURE_END batch=%u hal_fail=%u mismatch=%u reference_ok=%u\r\n",
                        batch, failures, changed, reference_ok);
    }
}
#endif

static __attribute__((noreturn)) void FlashDiag_Run(void)
{
    __HAL_RCC_GPIOB_CLK_ENABLE();
    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);
    GPIO_InitTypeDef cs = {
        .Pin = FLASH_CS_Pin,
        .Mode = GPIO_MODE_OUTPUT_PP,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_LOW
    };
    HAL_GPIO_Init(FLASH_CS_GPIO_Port, &cs);
    MX_USART2_UART_Init();
    MX_SPI1_Init();

    /* Bound the time-base check even if TIM5 interrupts never arrive. */
    uint32_t tick = HAL_GetTick();
    uint32_t spins = SystemCoreClock / 4U;
    while (((uint32_t)(HAL_GetTick() - tick) < 2U) && (spins > 0U))
    {
        --spins;
    }
    unsigned tick_ok = (uint32_t)(HAL_GetTick() - tick) >= 2U;
    if (tick_ok)
    {
        HAL_Delay(100U);
    }

    FlashDiag_Print("FLASH_DIAG_BEGIN version=1 build=%s %s mode=READ_ONLY\r\n",
                    __DATE__, __TIME__);
    FlashDiag_Print("HAL_TICK_OK=%u\r\n", tick_ok);
    HAL_StatusTypeDef init = W25Q64_Init(&hspi1);
    if (!tick_ok || init != HAL_OK)
    {
        FlashDiag_Print("FLASH_DIAG_END result=INIT_FAILED init=%u\r\n", (unsigned)init);
        FlashDiag_Stop();
    }

#if FLASH_SPI_CAPTURE_MODE
    FlashDiag_Capture();
#else
    unsigned id_ok = 0, id_fail = 0, id_mismatch = 0;
    for (unsigned i = 1; i <= 100U; ++i)
    {
        uint8_t mfr = 0, type = 0, capacity = 0;
        HAL_StatusTypeDef status = W25Q64_ReadJEDEC_ID(&mfr, &type, &capacity);
        if (status != HAL_OK)
        {
            ++id_fail;
        }
        else
        {
            ++id_ok;
            if (mfr != 0xEFU || type != 0x40U || capacity != 0x17U)
            {
                ++id_mismatch;
            }
        }
        FlashDiag_Print("ID %03u HAL=%u: %02X %02X %02X\r\n",
                        i, (unsigned)status, (unsigned)mfr, (unsigned)type, (unsigned)capacity);
    }
    FlashDiag_Print("ID_SUMMARY ok=%u hal_fail=%u mismatch=%u\r\n", id_ok, id_fail, id_mismatch);

    HAL_StatusTypeDef (*const read_sr[])(uint8_t *) = {
        W25Q64_ReadStatus1, W25Q64_ReadStatus2, W25Q64_ReadStatus3
    };
    unsigned sr_fail[3] = {0}, sr_changes[3] = {0}, sr_seen[3] = {0};
    uint8_t sr_ref[3] = {0};
    for (unsigned i = 1; i <= 10U; ++i)
    {
        for (unsigned reg = 0; reg < 3U; ++reg)
        {
            uint8_t value = 0;
            HAL_StatusTypeDef status = read_sr[reg](&value);
            if (status != HAL_OK)
            {
                ++sr_fail[reg];
            }
            else if (!sr_seen[reg])
            {
                sr_ref[reg] = value;
                sr_seen[reg] = 1U;
            }
            else if (value != sr_ref[reg])
            {
                ++sr_changes[reg];
            }
            FlashDiag_Print("SR%u %02u HAL=%u: %02X\r\n",
                            reg + 1U, i, (unsigned)status, (unsigned)value);
        }
    }
    for (unsigned reg = 0; reg < 3U; ++reg)
    {
        FlashDiag_Print("SR%u_SUMMARY ok=%u hal_fail=%u changed=%u ref=%02X\r\n",
                        reg + 1U, 10U - sr_fail[reg], sr_fail[reg],
                        sr_changes[reg], (unsigned)sr_ref[reg]);
    }

    unsigned read_fail = 0, read_mismatch = 0, reference_ok = 0;
    uint8_t reference[8] = {0};
    FlashDiag_Print("READ_TARGET addr=0x%06lX length=%u\r\n",
                    (unsigned long)FLASH_CONFIG_ADDR, (unsigned)sizeof(reference));
    for (unsigned i = 1; i <= 10U; ++i)
    {
        uint8_t data[sizeof(reference)] = {0};
        HAL_StatusTypeDef status = W25Q64_ReadData(FLASH_CONFIG_ADDR, data, sizeof(data));
        FlashDiag_Print("READ %02u: %02X %02X %02X %02X %02X %02X %02X %02X HAL=%u\r\n",
                        i, (unsigned)data[0], (unsigned)data[1], (unsigned)data[2],
                        (unsigned)data[3], (unsigned)data[4], (unsigned)data[5],
                        (unsigned)data[6], (unsigned)data[7], (unsigned)status);
        if (status != HAL_OK)
        {
            ++read_fail;
            continue;
        }
        if (i == 1U)
        {
            memcpy(reference, data, sizeof(reference));
            reference_ok = 1U;
        }
        else if (reference_ok)
        {
            for (unsigned offset = 0; offset < sizeof(reference); ++offset)
            {
                if (data[offset] != reference[offset])
                {
                    ++read_mismatch;
                    FlashDiag_Print("READ_DIFF sample=%02u offset=%u expected=%02X actual=%02X\r\n",
                                    i, offset, (unsigned)reference[offset], (unsigned)data[offset]);
                    break;
                }
            }
        }
    }

    unsigned spi_errors = id_fail + sr_fail[0] + sr_fail[1] + sr_fail[2] + read_fail;
    unsigned observed_issue = spi_errors || id_mismatch || read_mismatch ||
                              sr_changes[0] || sr_changes[1] || sr_changes[2];
    FlashDiag_Print("READ_SUMMARY ok=%u hal_fail=%u mismatch=%u reference_ok=%u\r\n",
                    10U - read_fail, read_fail, read_mismatch, reference_ok);
    FlashDiag_Print("FLASH_DIAG_END spi_hal_errors=%u result=%s\r\n",
                    spi_errors, observed_issue ? "CHECK_LOG" : "READS_REPEATABLE");
    FlashDiag_Stop();
#endif
}
#endif


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */


  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
#if FLASH_READ_DIAG_MODE
  FlashDiag_Run();
#endif

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_TIM2_Init();
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */

  HAL_Delay(100);

  HAL_StatusTypeDef aht20_init_status;

  aht20_init_status = AHT20_Init(&hi2c1);


  int len = snprintf(
      tx_buffer,
      sizeof(tx_buffer),
      "AHT20 Init: %s\r\n",
      (aht20_init_status == HAL_OK) ? "OK" : "ERROR"
  );

  HAL_StatusTypeDef oled_init_status;

  oled_init_status = OLED_Init(&hi2c1);

  oled_ready = (oled_init_status == HAL_OK) ? 1U : 0U;

  if (oled_init_status == HAL_OK)
  {
	    OLED_Clear();
  }

  HAL_UART_Transmit(
      &huart2,
      (uint8_t *)tx_buffer,
      len,
      100
  );


  int oled_len = snprintf(
      tx_buffer,
      sizeof(tx_buffer),
      "OLED Init: %s\r\n",
      (oled_init_status == HAL_OK) ? "OK" : "ERROR"
  );

  HAL_UART_Transmit(
      &huart2,
      (uint8_t *)tx_buffer,
      oled_len,
      100
  );


  #if APP_USE_FLASH_CONFIG
  uint8_t flash_manufacturer;
  uint8_t flash_memory_type;
  uint8_t flash_capacity;

  HAL_StatusTypeDef flash_status;

  flash_status = W25Q64_Init(&hspi1);

  if (flash_status == HAL_OK)
  {
      flash_status = W25Q64_ReadJEDEC_ID(&flash_manufacturer,
                                        &flash_memory_type,
                                        &flash_capacity);
  }

  if (flash_status == HAL_OK)
  {
      int flash_len = snprintf(
          tx_buffer,
          sizeof(tx_buffer),
          "W25Q64 JEDEC ID: %02X %02X %02X\r\n",
          flash_manufacturer,
          flash_memory_type,
          flash_capacity
      );

      HAL_UART_Transmit(
          &huart2,
          (uint8_t *)tx_buffer,
          flash_len,
          100
      );
  }
  else
  {
      int flash_len = snprintf(
          tx_buffer,
          sizeof(tx_buffer),
          "W25Q64 ERROR\r\n"
      );

      HAL_UART_Transmit(
          &huart2,
          (uint8_t *)tx_buffer,
          flash_len,
          100
      );
  }


  /* USER CODE BEGIN FLASH_TEST */

  /* W25Q64 底层读写测试 */

  #if FLASH_RAW_TEST

  {
      /* 擦除后的数据 */
      uint8_t erase_data[8] = {0};

      /* 准备写入的测试数据 */
      uint8_t test_data[8] = {
          0x2C, 0x01,
          0x22, 0x01,
          0x84, 0x03,
          0x52, 0x03
      };

      /* 读取回来的数据 */
      uint8_t read_data[8] = {0};

      HAL_StatusTypeDef erase_status = HAL_ERROR;
      HAL_StatusTypeDef erase_read_status = HAL_ERROR;
      HAL_StatusTypeDef program_status = HAL_ERROR;
      HAL_StatusTypeDef read_status = HAL_ERROR;

      uint8_t erase_verified = 0U;
      uint8_t program_attempted = 0U;
      uint8_t data_match = 0U;

      /* 检查 Flash 是否识别正常 */
      uint8_t flash_id_valid =
          (flash_status == HAL_OK &&
           flash_manufacturer == 0xEF &&
           flash_memory_type == 0x40 &&
           flash_capacity == 0x17);

      if (flash_id_valid)
      {
          /* 第一步：擦除扇区 */
          erase_status = W25Q64_SectorErase(0x002000);

          /* 第二步：读取擦除后的数据 */
          if (erase_status == HAL_OK)
          {
              erase_read_status = W25Q64_ReadData(
                  0x002000,
                  erase_data,
                  sizeof(erase_data)
              );

              if (erase_read_status == HAL_OK)
              {
                  erase_verified = 1U;

                  for (uint32_t i = 0; i < 8U; i++)
                  {
                      if (erase_data[i] != 0xFFU)
                      {
                          erase_verified = 0U;
                          break;
                      }
                  }
              }
          }

          /* 第三步：确认擦除成功后再写入 */
          if (erase_verified)
          {
              program_attempted = 1U;

              program_status = W25Q64_PageProgram(
                  0x002000,
                  test_data,
                  sizeof(test_data)
              );

              /* 第四步：重新读取数据 */
              if (program_status == HAL_OK)
              {
                  read_status = W25Q64_ReadData(
                      0x002000,
                      read_data,
                      sizeof(read_data)
                  );

                  if (read_status == HAL_OK)
                  {
                      data_match = 1U;

                      for (uint32_t i = 0; i < 8U; i++)
                      {
                          if (read_data[i] != test_data[i])
                          {
                              data_match = 0U;
                              break;
                          }
                      }
                  }
              }
          }
      }

      /* 输出擦除结果 */
      if (flash_id_valid)
      {
          int msg_len = snprintf(
              tx_buffer,
              sizeof(tx_buffer),
              "ERASE=%d EREAD=%d EVER=%u\r\n",
              (int)erase_status,
              (int)erase_read_status,
              (unsigned int)erase_verified
          );

          HAL_UART_Transmit(
              &huart2,
              (uint8_t *)tx_buffer,
              (uint16_t)msg_len,
              100
          );

          /* 输出写入结果 */
          msg_len = snprintf(
              tx_buffer,
              sizeof(tx_buffer),
              "PROG_TRY=%u PROG=%d READ=%d MATCH=%u\r\n",
              (unsigned int)program_attempted,
              (int)program_status,
              (int)read_status,
              (unsigned int)data_match
          );

          HAL_UART_Transmit(
              &huart2,
              (uint8_t *)tx_buffer,
              (uint16_t)msg_len,
              100
          );

          /* 输出擦除后的实际数据 */
          msg_len = snprintf(
              tx_buffer,
              sizeof(tx_buffer),
              "ERASED: %02X %02X %02X %02X %02X %02X %02X %02X\r\n",
              (unsigned int)erase_data[0],
              (unsigned int)erase_data[1],
              (unsigned int)erase_data[2],
              (unsigned int)erase_data[3],
              (unsigned int)erase_data[4],
              (unsigned int)erase_data[5],
              (unsigned int)erase_data[6],
              (unsigned int)erase_data[7]
          );

          HAL_UART_Transmit(
              &huart2,
              (uint8_t *)tx_buffer,
              (uint16_t)msg_len,
              100
          );

          /* 输出写入后读取的实际数据 */
          msg_len = snprintf(
              tx_buffer,
              sizeof(tx_buffer),
              "READBACK: %02X %02X %02X %02X %02X %02X %02X %02X\r\n",
              (unsigned int)read_data[0],
              (unsigned int)read_data[1],
              (unsigned int)read_data[2],
              (unsigned int)read_data[3],
              (unsigned int)read_data[4],
              (unsigned int)read_data[5],
              (unsigned int)read_data[6],
              (unsigned int)read_data[7]
          );

          HAL_UART_Transmit(
              &huart2,
              (uint8_t *)tx_buffer,
              (uint16_t)msg_len,
              100
          );
      }
      else
      {
          int msg_len = snprintf(
              tx_buffer,
              sizeof(tx_buffer),
              "RAW TEST SKIPPED: FLASH ID ERROR\r\n"
          );

          HAL_UART_Transmit(
              &huart2,
              (uint8_t *)tx_buffer,
              (uint16_t)msg_len,
              100
          );
      }
  }

  #endif

  /* W25Q64 底层测试结束 */

  #if FLASH_CONFIG_WRITE_TEST

  {
      /* 准备测试数据 */
      AlarmConfig testConfig;

      /* 准备读回数据 */
      AlarmConfig verifyConfig = {0};

      /* 初始化测试状态 */
      HAL_StatusTypeDef saveStatus = HAL_ERROR;
      HAL_StatusTypeDef readBackStatus = HAL_ERROR;

      uint8_t dataMatch = 0U;

      /* 检查 JEDEC ID 是否正确 */
      uint8_t flashIdValid =
          (flash_status == HAL_OK &&
           flash_manufacturer == 0xEF &&
           flash_memory_type == 0x40 &&
           flash_capacity == 0x17);

      if (flashIdValid)
      {
          /* 设置默认报警阈值 */
          Config_SetDefault(&testConfig);

          /* 第一步：擦除并写入 Flash */
          saveStatus = Config_Save(&testConfig);

          /* 第二步：立即读回 */
          if (saveStatus == HAL_OK)
          {
              readBackStatus = Config_Load(&verifyConfig);

              /* 第三步：逐字段比较 */
              if (readBackStatus == HAL_OK &&
                  Config_IsValid(&verifyConfig) &&
                  verifyConfig.temp_on_x10 ==
                      testConfig.temp_on_x10 &&
                  verifyConfig.temp_off_x10 ==
                      testConfig.temp_off_x10 &&
                  verifyConfig.humi_on_x10 ==
                      testConfig.humi_on_x10 &&
                  verifyConfig.humi_off_x10 ==
                      testConfig.humi_off_x10)
              {
                  dataMatch = 1U;
              }
          }
      }

      /* 输出测试结果 */
      int testLen = snprintf(
          tx_buffer,
          sizeof(tx_buffer),
          "FLASH TEST: ID=%u SAVE=%d READ=%d MATCH=%u\r\n",
          (unsigned int)flashIdValid,
          (int)saveStatus,
          (int)readBackStatus,
          (unsigned int)dataMatch
      );

      if (testLen > 0 &&
          testLen < (int)sizeof(tx_buffer))
      {
          HAL_UART_Transmit(
              &huart2,
              (uint8_t *)tx_buffer,
              (uint16_t)testLen,
              100
          );
      }
  }

  #endif

  /* USER CODE END FLASH_TEST */


  AlarmConfig read_config = {0};

  HAL_StatusTypeDef cfgLoadStatus = Config_Load(&read_config);

  uint8_t cfgValid = 0;

  if (cfgLoadStatus == HAL_OK)
  {
      cfgValid = Config_IsValid(&read_config);
  }

  /* 只读取和打印，不写 Flash */
  int cfgDebugLen = snprintf(
      tx_buffer,
      sizeof(tx_buffer),
      "CFG load=%d valid=%u raw=%lu,%lu,%lu,%lu\r\n",
      (int)cfgLoadStatus,
      (unsigned int)cfgValid,
      (unsigned long)read_config.temp_on_x10,
      (unsigned long)read_config.temp_off_x10,
      (unsigned long)read_config.humi_on_x10,
      (unsigned long)read_config.humi_off_x10
  );

  if (cfgDebugLen > 0 &&
      cfgDebugLen < (int)sizeof(tx_buffer))
  {
      HAL_UART_Transmit(
          &huart2,
          (uint8_t *)tx_buffer,
          (uint16_t)cfgDebugLen,
          100
      );
  }


  if (cfgLoadStatus == HAL_OK && cfgValid)
  {
      int config_len = snprintf(
          tx_buffer,
          sizeof(tx_buffer),
          "Load Config: T_ON=%d T_OFF=%d H_ON=%d H_OFF=%d\r\n",
          read_config.temp_on_x10,
          read_config.temp_off_x10,
          read_config.humi_on_x10,
          read_config.humi_off_x10
      );


      HAL_UART_Transmit(
          &huart2,
          (uint8_t *)tx_buffer,
          config_len,
          100
      );

      snprintf(
          tx_buffer,
          sizeof(tx_buffer),
          "Config source: FLASH\r\n"
      );

      HAL_UART_Transmit(
          &huart2,
          (uint8_t *)tx_buffer,
          strlen(tx_buffer),
          100
      );


  }
  else
  {
      /* Flash读取失败或配置非法 */
      Config_SetDefault(&read_config);

      //Config_Save(&read_config);


      snprintf(
          tx_buffer,
          sizeof(tx_buffer),
          "Config invalid, defaults restored\r\n"
      );

      HAL_UART_Transmit(
          &huart2,
          (uint8_t *)tx_buffer,
          strlen(tx_buffer),
          100
      );
  }


  #else
  AlarmConfig read_config;
  Config_SetDefault(&read_config);
  static const char config_source[] = "Config source: RAM defaults (Flash isolated)\r\n";
  HAL_UART_Transmit(&huart2, (const uint8_t *)config_source, sizeof(config_source) - 1U, 100U);
  #endif

  Alarm_SetThresholds(
      read_config.temp_on_x10,
      read_config.temp_off_x10,
      read_config.humi_on_x10,
      read_config.humi_off_x10
  );



  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();
  /* Create the mutex(es) */
  /* creation of i2cMutex */
  i2cMutexHandle = osMutexNew(&i2cMutex_attributes);

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
#if QUEUE_STRESS_TEST
  const osMutexAttr_t stressUartMutexAttrs = {.name = "stressUart", .attr_bits = osMutexPrioInherit};
  queueStressUartMutex = osMutexNew(&stressUartMutexAttrs);
  if (queueStressUartMutex == NULL)
  {
    Error_Handler();
  }
#endif
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* creation of buttonSem */
  buttonSemHandle = osSemaphoreNew(1, 0, &buttonSem_attributes);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of sensorDisplayQueue */
  sensorDisplayQueueHandle = osMessageQueueNew (5, 8, &sensorDisplayQueue_attributes);

  /* creation of sensorAlarmQueue */
  sensorAlarmQueueHandle = osMessageQueueNew (5, 8, &sensorAlarmQueue_attributes);

  /* creation of sensorUartQueue */
  sensorUartQueueHandle = osMessageQueueNew (5, 8, &sensorUartQueue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of SensorTask */
  SensorTaskHandle = osThreadNew(StartSensorTask, NULL, &SensorTask_attributes);

  /* creation of DisplayTask */
  DisplayTaskHandle = osThreadNew(StartDisplayTask, NULL, &DisplayTask_attributes);

  /* creation of AlarmTask */
  AlarmTaskHandle = osThreadNew(StartAlarmTask, NULL, &AlarmTask_attributes);

  /* creation of UartTask */
  UartTaskHandle = osThreadNew(StartUartTask, NULL, &UartTask_attributes);

  /* creation of ButtonTask */
  ButtonTaskHandle = osThreadNew(StartButtonTask, NULL, &ButtonTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* creation of systemEventFlags */
  systemEventFlagsHandle = osEventFlagsNew(&systemEventFlags_attributes);

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  if (SensorTaskHandle == NULL || DisplayTaskHandle == NULL ||
      AlarmTaskHandle == NULL || UartTaskHandle == NULL || ButtonTaskHandle == NULL ||
      sensorDisplayQueueHandle == NULL || sensorAlarmQueueHandle == NULL ||
      sensorUartQueueHandle == NULL || i2cMutexHandle == NULL || buttonSemHandle == NULL ||
      systemEventFlagsHandle == NULL)
  {
    static const char init_error[] = "APP_RTOS_INIT_FAILED\r\n";
    HAL_UART_Transmit(&huart2, (const uint8_t *)init_error, sizeof(init_error) - 1U, 100U);
    Error_Handler();
  }
  static const char tasks_ready[] = "APP_RTOS_READY tasks=5 queues=3\r\n";
  HAL_UART_Transmit(&huart2, (const uint8_t *)tasks_ready, sizeof(tasks_ready) - 1U, 100U);
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

	  if (button_pressed == 1)
	  {
	      button_pressed = 0;

	      Alarm_Enable_Toggle();
	  }


	  if (AHT20_Read(&temperature, &humidity) == HAL_OK)
	  {

		  temperature_x10 = (int32_t)(temperature * 10.0f);
		  humidity_x10 = (uint32_t)(humidity * 10.0f);

		  Alarm_Update(
		      temperature_x10,
		      humidity_x10
		  );


		  Alarm_Buzzer_Update();
		  /* ---------- UART 输出 ---------- */

		  int aht_len = snprintf(
		      tx_buffer,
		      sizeof(tx_buffer),
		      "Temperature: %ld.%ld C, Humidity: %lu.%lu %%, State: %s\r\n",
		      (long)(temperature_x10 / 10),
		      (long)(temperature_x10 % 10),
		      (unsigned long)(humidity_x10 / 10),
		      (unsigned long)(humidity_x10 % 10),
		      (Alarm_GetState() == ALARM_ACTIVE) ? "ALARM" : "NORMAL"
		  );

		  HAL_UART_Transmit(
			  &huart2,
			  (uint8_t *)tx_buffer,
			  aht_len,
			  100
		  );


		  /* ---------- OLED 实时温度 ---------- */

		  snprintf(
			  oled_temp,
			  sizeof(oled_temp),
			  "%ld.%ld",
			  (long)(temperature_x10 / 10),
			  (long)(temperature_x10 % 10)
		  );

		  snprintf(
			  oled_humi,
			  sizeof(oled_humi),
			  "%lu.%lu",
			  (unsigned long)(humidity_x10 / 10),
			  (unsigned long)(humidity_x10 % 10)
		  );

		  if (Alarm_GetState() == ALARM_ACTIVE)
		  {
		      snprintf(
		          oled_state,
		          sizeof(oled_state),
		          "ALARM"
		      );
		  }
		  else
		  {
		      snprintf(
		          oled_state,
		          sizeof(oled_state),
		          "NORMAL"
		      );
		  }

		  if (oled_init_status == HAL_OK)
		  {
			  /* 第一行：温度 */
			  OLED_ClearPage(0);
			  OLED_ShowString(0, 0, oled_temp);

			  /* 第二行：湿度 */
			  OLED_ClearPage(1);
			  OLED_ShowString(0, 1, oled_humi);

			  OLED_ClearPage(2);
			  OLED_ShowString(0, 2, oled_state);

			  OLED_ClearPage(3);

			  if (Alarm_IsEnabled())
			  {
			      OLED_ShowString(0, 3, "ALARM ON");
			  }
			  else
			  {
			      OLED_ShowString(0, 3, "ALARM OFF");
			  }
		  }



	  }

	  HAL_Delay(1000);
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */
#if !APP_USE_FLASH_CONFIG && !FLASH_READ_DIAG_MODE
  return;
#endif

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 4294967295;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, BUZZER_Pin|FLASH_CS_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : BUZZER_Pin FLASH_CS_Pin */
  GPIO_InitStruct.Pin = BUZZER_Pin|FLASH_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    static uint32_t last_button_tick = 0;

    if (GPIO_Pin == GPIO_PIN_13)
    {
        uint32_t now = HAL_GetTick();

        // 50ms 消抖
        if (now - last_button_tick >= 50U)
        {
            last_button_tick = now;

            // 释放二值信号量，通知 ButtonTask
            if (buttonSemHandle != NULL)
            {
                osSemaphoreRelease(buttonSemHandle);
            }
        }
    }
}


/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartSensorTask */
/**
  * @brief  Function implementing the SensorTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartSensorTask */
void StartSensorTask(void *argument)
{
    (void)argument;

    float temperature = 0.0f;
    float humidity = 0.0f;

    SensorData_t sensorData;

    HAL_StatusTypeDef status;

    for (;;)
    {
#if QUEUE_STRESS_TEST
        QueueStress_Step();
#endif
        /* 获取 I2C 互斥锁 */
        if (osMutexAcquire(i2cMutexHandle, osWaitForever) == osOK)
        {
            /* 读取 AHT20 */
            status = AHT20_Read(&temperature, &humidity);

            /* 释放互斥锁 */
            osMutexRelease(i2cMutexHandle);

            if (status == HAL_OK)
            {
                /* 数据转换 */
                sensorData.temperature_x10 =
                    (int32_t)(temperature * 10.0f);

                sensorData.humidity_x10 =
                    (uint32_t)(humidity * 10.0f);

                /* 发送 OLED 消息 */
                osStatus_t displayPut = osMessageQueuePut(
                        sensorDisplayQueueHandle, &sensorData, 0, 0);
                if (displayPut != osOK)
                {
                    displayQueueDropCount++;
                }
#if QUEUE_STRESS_TEST
                QueueStress_RecordPut(QueueDisplay, sensorDisplayQueueHandle, displayPut);
#endif

                /* 发送报警消息 */
                osStatus_t alarmPut = osMessageQueuePut(
                        sensorAlarmQueueHandle, &sensorData, 0, 0);
                if (alarmPut != osOK)
                {
                    alarmQueueDropCount++;
                }
#if QUEUE_STRESS_TEST
                QueueStress_RecordPut(QueueAlarm, sensorAlarmQueueHandle, alarmPut);
#endif

                /* 发送 UART 消息 */
                osStatus_t uartPut = osMessageQueuePut(
                        sensorUartQueueHandle, &sensorData, 0, 0);
                if (uartPut != osOK)
                {
                    uartQueueDropCount++;
                }
#if QUEUE_STRESS_TEST
                QueueStress_RecordPut(QueueUart, sensorUartQueueHandle, uartPut);
#endif
            }
        }

        /* 每秒采集一次 */
        osDelay(1000);
    }
}

/* USER CODE BEGIN Header_StartDisplayTask */
/**
* @brief Function implementing the DisplayTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartDisplayTask */
void StartDisplayTask(void *argument)
{
    (void)argument;

    SensorData_t receivedData;

    for (;;)
    {
        /* 等待传感器数据 */
        if (osMessageQueueGet(
                sensorDisplayQueueHandle,
                &receivedData,
                NULL,
                osWaitForever) != osOK)
        {
            continue;
        }

        /* 温度格式化 */
        snprintf(
            oled_temp,
            sizeof(oled_temp),
            "%ld.%ld",
            (long)(receivedData.temperature_x10 / 10),
            (long)(receivedData.temperature_x10 % 10)
        );

        /* 湿度格式化 */
        snprintf(
            oled_humi,
            sizeof(oled_humi),
            "%lu.%lu",
            (unsigned long)(receivedData.humidity_x10 / 10),
            (unsigned long)(receivedData.humidity_x10 % 10)
        );

        /* 获取报警状态 */
        if (Alarm_GetState() == ALARM_ACTIVE)
        {
            snprintf(oled_state, sizeof(oled_state), "ALARM");
        }
        else
        {
            snprintf(oled_state, sizeof(oled_state), "NORMAL");
        }

        /* OLED 初始化成功才执行显示 */
        if (oled_ready == 1U)
        {
            /* 获取 I2C 互斥锁 */
            if (osMutexAcquire(i2cMutexHandle, osWaitForever) == osOK)
            {
                /* 第一行：温度 */
                OLED_ClearPage(0);
                OLED_ShowString(0, 0, oled_temp);

                /* 第二行：湿度 */
                OLED_ClearPage(1);
                OLED_ShowString(0, 1, oled_humi);

                /* 第三行：报警状态 */
                OLED_ClearPage(2);
                OLED_ShowString(0, 2, oled_state);

                /* 第四行：报警使能状态 */
                OLED_ClearPage(3);

                if (Alarm_IsEnabled())
                {
                    OLED_ShowString(0, 3, "ALARM ON");
                }
                else
                {
                    OLED_ShowString(0, 3, "ALARM OFF");
                }

                /* 释放 I2C 互斥锁 */
                osMutexRelease(i2cMutexHandle);
            }
        }
#if QUEUE_STRESS_TEST
        QueueStress_Consumed(QueueDisplay);
#endif
    }
}

/* USER CODE BEGIN Header_StartAlarmTask */
/**
* @brief Function implementing the AlarmTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartAlarmTask */
void StartAlarmTask(void *argument)
{
    (void)argument;

    SensorData_t receivedData;

    for (;;)
    {
        /* 等待传感器数据 */
        if (osMessageQueueGet(
                sensorAlarmQueueHandle,
                &receivedData,
                NULL,
                osWaitForever) != osOK)
        {
            continue;
        }

        /* 更新报警状态 */
        Alarm_Update(
            receivedData.temperature_x10,
            receivedData.humidity_x10
        );

        /* 更新蜂鸣器状态 */
        Alarm_Buzzer_Update();
#if QUEUE_STRESS_TEST
        QueueStress_Consumed(QueueAlarm);
#endif
    }
}

/* USER CODE BEGIN Header_StartUartTask */
/**
* @brief Function implementing the UartTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartUartTask */
void StartUartTask(void *argument)
{
    (void)argument;

    SensorData_t receivedData;
    char uart_buffer[80];

    for (;;)
    {
        /* 等待传感器数据 */
        osStatus_t status = osMessageQueueGet(
            sensorUartQueueHandle,
            &receivedData,
            NULL,
            osWaitForever
        );

        if (status != osOK)
        {
            continue;
        }

        /* ================= 栈空间监测 ================= */

        sensorStackFree = osThreadGetStackSpace(SensorTaskHandle);

        displayStackFree = osThreadGetStackSpace(DisplayTaskHandle);

        alarmStackFree = osThreadGetStackSpace(AlarmTaskHandle);

        uartStackFree = osThreadGetStackSpace(UartTaskHandle);

        buttonStackFree = osThreadGetStackSpace(ButtonTaskHandle);

        /* ================================================= */

        /* 处理温度正负号 */
        int32_t temp_x10 = receivedData.temperature_x10;

        uint32_t temp_abs_x10 = (temp_x10 < 0)
            ? (uint32_t)(-(int64_t)temp_x10)
            : (uint32_t)temp_x10;

        /* 格式化串口输出 */
        int len = snprintf(
            uart_buffer,
            sizeof(uart_buffer),
            "Temperature: %s%lu.%lu C, Humidity: %lu.%lu %%, State: %s\r\n",

            (temp_x10 < 0) ? "-" : "",

            (unsigned long)(temp_abs_x10 / 10),
            (unsigned long)(temp_abs_x10 % 10),

            (unsigned long)(receivedData.humidity_x10 / 10),
            (unsigned long)(receivedData.humidity_x10 % 10),

            (Alarm_GetState() == ALARM_ACTIVE) ? "ALARM" : "NORMAL"
        );

        /* 发送串口数据 */
        if (len > 0)
        {
            if (len >= (int)sizeof(uart_buffer))
            {
                len = sizeof(uart_buffer) - 1;
            }

#if QUEUE_STRESS_TEST
            QueueStress_Uart(uart_buffer, (uint16_t)len);
#else
            HAL_UART_Transmit(
                &huart2,
                (uint8_t *)uart_buffer,
                (uint16_t)len,
                100
            );
#endif
        }
#if QUEUE_STRESS_TEST
        QueueStress_Consumed(QueueUart);
#endif
    }
}
/* USER CODE BEGIN Header_StartButtonTask */
/**
* @brief Function implementing the ButtonTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartButtonTask */
void StartButtonTask(void *argument)
{
    (void)argument;

    for (;;)
    {
        // 等待按键事件
        osStatus_t status = osSemaphoreAcquire(
            buttonSemHandle,
            osWaitForever
        );

        if (status == osOK)
        {
            // 切换报警使能状态
            Alarm_Enable_Toggle();
        }
    }
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM5 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM5)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
