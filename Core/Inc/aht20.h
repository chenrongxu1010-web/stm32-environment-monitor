/*
 * aht20.h
 *
 *  Created on: Sep 20, 2026
 *      Author: 28380
 */

#ifndef INC_AHT20_H_
#define INC_AHT20_H_

#include "main.h"

HAL_StatusTypeDef AHT20_Init(I2C_HandleTypeDef *hi2c);

HAL_StatusTypeDef AHT20_Read(
    float *temperature,
    float *humidity
);



#endif /* INC_AHT20_H_ */
