/*
 * config.h
 *
 *  Created on: Sep 22, 2026
 *      Author: 28380
 */

#ifndef INC_CONFIG_H_
#define INC_CONFIG_H_


#include "main.h"

typedef struct
{
    uint16_t temp_on_x10;
    uint16_t temp_off_x10;

    uint16_t humi_on_x10;
    uint16_t humi_off_x10;

} AlarmConfig;


HAL_StatusTypeDef Config_Save(AlarmConfig *config);

HAL_StatusTypeDef Config_Load(AlarmConfig *config);


uint8_t Config_IsValid(const AlarmConfig *config);

void Config_SetDefault(AlarmConfig *config);


#endif /* INC_CONFIG_H_ */
