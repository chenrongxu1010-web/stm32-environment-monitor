/*
 * config.c
 *
 *  Created on: Sep 22, 2026
 *      Author: 28380
 */


#include "config.h"
#include "w25q64.h"


#define FLASH_CONFIG_ADDR 0x002000


HAL_StatusTypeDef Config_Save(AlarmConfig *config)
{
    if(config == NULL)
    {
        return HAL_ERROR;
    }


    if(W25Q64_SectorErase(FLASH_CONFIG_ADDR)
            != HAL_OK)
    {
        return HAL_ERROR;
    }


    if(W25Q64_PageProgram(
            FLASH_CONFIG_ADDR,
            (uint8_t *)config,
            sizeof(AlarmConfig))
            != HAL_OK)
    {
        return HAL_ERROR;
    }


    return HAL_OK;
}


HAL_StatusTypeDef Config_Load(AlarmConfig *config)
{
    if(config == NULL)
    {
        return HAL_ERROR;
    }


    if(W25Q64_ReadData(
            FLASH_CONFIG_ADDR,
            (uint8_t *)config,
            sizeof(AlarmConfig))
            != HAL_OK)
    {
        return HAL_ERROR;
    }


    return HAL_OK;
}


uint8_t Config_IsValid(const AlarmConfig *config)
{
    if(config == NULL)
    {
        return 0;
    }

    if(config->temp_off_x10 >= config->temp_on_x10)
    {
        return 0;
    }

    if(config->humi_off_x10 >= config->humi_on_x10)
    {
        return 0;
    }

    if(config->temp_on_x10 > 1000)
    {
        return 0;
    }

    if(config->humi_on_x10 > 1000)
    {
        return 0;
    }

    return 1;
}


void Config_SetDefault(AlarmConfig *config)
{
    if(config == NULL)
    {
        return;
    }

    config->temp_on_x10  = 300;
    config->temp_off_x10 = 290;
    config->humi_on_x10  = 900;
    config->humi_off_x10 = 850;
}



