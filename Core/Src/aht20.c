/*
 * aht20.c
 *
 *  Created on: Sep 20, 2026
 *      Author: 28380
 */


#include "aht20.h"

#define AHT20_ADDR (0x38 << 1)

static I2C_HandleTypeDef *aht20_i2c;

static uint8_t measure_cmd[3] =
{
    0xAC, 0x33, 0x00
};

HAL_StatusTypeDef AHT20_Init(I2C_HandleTypeDef *hi2c)
{
    aht20_i2c = hi2c;

    if (HAL_I2C_IsDeviceReady(
          aht20_i2c,
          AHT20_ADDR,
          3,
          100
      ) != HAL_OK)
    {
    		return HAL_ERROR;
    }

    return HAL_OK;
}

HAL_StatusTypeDef AHT20_Read(
    float *temperature,
    float *humidity
)
{
    uint8_t data[6] = {0};

    uint32_t raw_humidity = 0;
    uint32_t raw_temperature = 0;


    /* 1. 发送测量命令 */
    if (HAL_I2C_Master_Transmit(
            aht20_i2c,
            AHT20_ADDR,
            measure_cmd,
            3,
            100) != HAL_OK)
    {
        return HAL_ERROR;
    }


    /* 2. 等待传感器完成测量 */
    HAL_Delay(80);


    /* 3. 读取 6 Byte 数据 */
    if (HAL_I2C_Master_Receive(
            aht20_i2c,
            AHT20_ADDR,
            data,
            6,
            100) != HAL_OK)
    {
        return HAL_ERROR;
    }


    /* 4. bit7 = 1 表示仍然 Busy（忙） */
    if ((data[0] & 0x80) != 0)
    {
        return HAL_ERROR;
    }


    /* 5. 拼接 20-bit 湿度原始数据 */
    raw_humidity =
        ((uint32_t)data[1] << 12) |
        ((uint32_t)data[2] << 4) |
        ((uint32_t)data[3] >> 4);


    /* 6. 拼接 20-bit 温度原始数据 */
    raw_temperature =
        (((uint32_t)data[3] & 0x0F) << 16) |
        ((uint32_t)data[4] << 8) |
        ((uint32_t)data[5]);


    /* 7. 转换成真实物理量 */
    *humidity =
        raw_humidity * 100.0f / 1048576.0f;

    *temperature =
        raw_temperature * 200.0f / 1048576.0f - 50.0f;


    return HAL_OK;
}
