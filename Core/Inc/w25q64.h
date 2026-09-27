/*
 * w25q64.h
 *
 *  Created on: Sep 22, 2026
 *      Author: 28380
 */

#ifndef INC_W25Q64_H_
#define INC_W25Q64_H_

#include "main.h"

#define FLASH_CONFIG_ADDR 0x002000

HAL_StatusTypeDef W25Q64_Init(SPI_HandleTypeDef *hspi);

HAL_StatusTypeDef W25Q64_ReadJEDEC_ID(uint8_t *manufacturer,
                                     uint8_t *memory_type,
                                     uint8_t *capacity);

HAL_StatusTypeDef W25Q64_ReadData(uint32_t address,
                                 uint8_t *data,
                                 uint16_t size);

HAL_StatusTypeDef W25Q64_SectorErase(uint32_t address);

HAL_StatusTypeDef W25Q64_PageProgram(uint32_t address,
                                    uint8_t *data,
                                    uint16_t size);

#endif /* INC_W25Q64_H_ */
