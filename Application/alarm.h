/*
 * alarm.h
 *
 *  Created on: Sep 24, 2026
 *      Author: 28380
 */

#ifndef ALARM_H
#define ALARM_H


#include "main.h"


typedef enum
{
    ALARM_NORMAL,
    ALARM_ACTIVE

} AlarmState;



void Alarm_Update(int32_t temperature_x10,
                  uint32_t humidity_x10);


void Alarm_SetThresholds(uint16_t temp_on_x10,
                         uint16_t temp_off_x10,
                         uint16_t humi_on_x10,
                         uint16_t humi_off_x10);


void Alarm_Buzzer_Update(void);


AlarmState Alarm_GetState(void);


void Alarm_Enable(uint8_t enable);


void Alarm_Enable_Toggle(void);


uint8_t Alarm_IsEnabled(void);


#endif
