/*
 * w25q64.c
 *
 *  Created on: Sep 22, 2026
 *      Author: 28380
 */


#include "w25q64.h"

#define W25Q64_CMD_JEDEC_ID  0x9F

#define W25Q64_CMD_READ_STATUS   0x05
#define W25Q64_CMD_WRITE_ENABLE  0x06

#define W25Q64_STATUS_BUSY       0x01
#define W25Q64_STATUS_WEL        0x02

#define W25Q64_CMD_READ_DATA  0x03

#define W25Q64_CMD_SECTOR_ERASE  0x20

#define W25Q64_CMD_PAGE_PROGRAM  0x02

static SPI_HandleTypeDef *w25q64_spi;

static void W25Q64_CS_Low(void)
{
    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port,
                      FLASH_CS_Pin,
                      GPIO_PIN_RESET);
}

static void W25Q64_CS_High(void)
{
    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port,
                      FLASH_CS_Pin,
                      GPIO_PIN_SET);
}

HAL_StatusTypeDef W25Q64_Init(SPI_HandleTypeDef *hspi)
{
    if (hspi == NULL)
    {
        return HAL_ERROR;
    }

    w25q64_spi = hspi;

    W25Q64_CS_High();

    return HAL_OK;
}

HAL_StatusTypeDef W25Q64_ReadJEDEC_ID(
    uint8_t *manufacturer,
    uint8_t *memory_type,
    uint8_t *capacity)
{

    uint8_t cmd = W25Q64_CMD_JEDEC_ID;
    uint8_t id[3];


    if((w25q64_spi == NULL) ||
       (manufacturer == NULL) ||
       (memory_type == NULL) ||
       (capacity == NULL))
    {
        return HAL_ERROR;
    }


    W25Q64_CS_Low();


    if(HAL_SPI_Transmit(
            w25q64_spi,
            &cmd,
            1,
            100)!=HAL_OK)
    {
        W25Q64_CS_High();
        return HAL_ERROR;
    }


    if(HAL_SPI_Receive(
            w25q64_spi,
            id,
            3,
            100)!=HAL_OK)
    {
        W25Q64_CS_High();
        return HAL_ERROR;
    }


    W25Q64_CS_High();


    *manufacturer=id[0];
    *memory_type=id[1];
    *capacity=id[2];


    return HAL_OK;
}

static HAL_StatusTypeDef W25Q64_ReadStatus(uint8_t *status)
{
    uint8_t cmd = W25Q64_CMD_READ_STATUS;

    if (status == NULL)
    {
        return HAL_ERROR;
    }

    W25Q64_CS_Low();

    if (HAL_SPI_Transmit(w25q64_spi, &cmd, 1, 100) != HAL_OK)
    {
        W25Q64_CS_High();
        return HAL_ERROR;
    }

    if (HAL_SPI_Receive(w25q64_spi, status, 1, 100) != HAL_OK)
    {
        W25Q64_CS_High();
        return HAL_ERROR;
    }

    W25Q64_CS_High();

    return HAL_OK;
}

static HAL_StatusTypeDef W25Q64_WriteEnable(void)
{
    uint8_t cmd = W25Q64_CMD_WRITE_ENABLE;

    W25Q64_CS_Low();

    HAL_StatusTypeDef status =
        HAL_SPI_Transmit(w25q64_spi, &cmd, 1, 100);

    W25Q64_CS_High();

    return status;
}

static HAL_StatusTypeDef W25Q64_WaitBusy(void)
{
    uint8_t status;

    do
    {
        if (W25Q64_ReadStatus(&status) != HAL_OK)
        {
            return HAL_ERROR;
        }

    } while ((status & W25Q64_STATUS_BUSY) != 0);

    return HAL_OK;
}

HAL_StatusTypeDef W25Q64_ReadData(uint32_t address,
                                 uint8_t *data,
                                 uint16_t size)
{
    uint8_t cmd[4];

    if ((w25q64_spi == NULL) ||
        (data == NULL) ||
        (size == 0))
    {
        return HAL_ERROR;
    }

    cmd[0] = W25Q64_CMD_READ_DATA;
    cmd[1] = (uint8_t)(address >> 16);
    cmd[2] = (uint8_t)(address >> 8);
    cmd[3] = (uint8_t)address;

    W25Q64_CS_Low();

    if (HAL_SPI_Transmit(w25q64_spi, cmd, 4, 100) != HAL_OK)
    {
        W25Q64_CS_High();
        return HAL_ERROR;
    }

    if (HAL_SPI_Receive(w25q64_spi, data, size, 100) != HAL_OK)
    {
        W25Q64_CS_High();
        return HAL_ERROR;
    }

    W25Q64_CS_High();

    return HAL_OK;
}

HAL_StatusTypeDef W25Q64_SectorErase(uint32_t address)
{
    uint8_t cmd[4];

    if (w25q64_spi == NULL)
    {
        return HAL_ERROR;
    }

    if (W25Q64_WriteEnable() != HAL_OK)
    {
        return HAL_ERROR;
    }

    cmd[0] = W25Q64_CMD_SECTOR_ERASE;
    cmd[1] = (uint8_t)(address >> 16);
    cmd[2] = (uint8_t)(address >> 8);
    cmd[3] = (uint8_t)address;

    W25Q64_CS_Low();

    if (HAL_SPI_Transmit(w25q64_spi, cmd, 4, 100) != HAL_OK)
    {
        W25Q64_CS_High();
        return HAL_ERROR;
    }

    W25Q64_CS_High();

    return W25Q64_WaitBusy();
}

HAL_StatusTypeDef W25Q64_PageProgram(uint32_t address,
                                    uint8_t *data,
                                    uint16_t size)
{
    uint8_t cmd[4];

    if ((w25q64_spi == NULL) ||
        (data == NULL) ||
        (size == 0) ||
        (size > 256))
    {
        return HAL_ERROR;
    }

    /* 不允许一次写操作跨越 256-byte Page 边界 */
    if (((address & 0xFF) + size) > 256)
    {
        return HAL_ERROR;
    }

    if (W25Q64_WriteEnable() != HAL_OK)
    {
        return HAL_ERROR;
    }

    cmd[0] = W25Q64_CMD_PAGE_PROGRAM;
    cmd[1] = (uint8_t)(address >> 16);
    cmd[2] = (uint8_t)(address >> 8);
    cmd[3] = (uint8_t)address;

    W25Q64_CS_Low();

    if (HAL_SPI_Transmit(w25q64_spi, cmd, 4, 100) != HAL_OK)
    {
        W25Q64_CS_High();
        return HAL_ERROR;
    }

    if (HAL_SPI_Transmit(w25q64_spi, data, size, 100) != HAL_OK)
    {
        W25Q64_CS_High();
        return HAL_ERROR;
    }

    W25Q64_CS_High();

    return W25Q64_WaitBusy();
}


