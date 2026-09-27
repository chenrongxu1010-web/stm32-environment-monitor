/*
 * oled.h
 *
 *  Created on: Sep 20, 2026
 *      Author: 28380
 */

#ifndef INC_OLED_H_
#define INC_OLED_H_

#include "main.h"

HAL_StatusTypeDef OLED_Init(I2C_HandleTypeDef *hi2c);

HAL_StatusTypeDef OLED_Clear(void);

HAL_StatusTypeDef OLED_ClearPage(uint8_t page);

HAL_StatusTypeDef OLED_ShowString(
    uint8_t col,
    uint8_t page,
    const char *str
);



#endif /* INC_OLED_H_ */
