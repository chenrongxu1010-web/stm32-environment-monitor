/*
 * alarm.c
 *
 *  Created on: Sep 24, 2026
 *      Author: 28380
 */

#include "alarm.h"



static uint16_t temp_on_x10  = 300;
static uint16_t temp_off_x10 = 290;
static uint16_t humi_on_x10  = 900;
static uint16_t humi_off_x10 = 850;


void Alarm_SetThresholds(uint16_t temp_on,
                         uint16_t temp_off,
                         uint16_t humi_on,
                         uint16_t humi_off)
{
    temp_on_x10  = temp_on;
    temp_off_x10 = temp_off;
    humi_on_x10  = humi_on;
    humi_off_x10 = humi_off;
}


static AlarmState alarm_state = ALARM_NORMAL;


static uint8_t alarm_enabled = 1;



void Alarm_Update(int32_t temperature_x10,
                  uint32_t humidity_x10)
{

    if(alarm_state == ALARM_NORMAL)
    {
        if(temperature_x10 >= temp_on_x10 ||
        		humidity_x10 >= humi_on_x10)
        {
            alarm_state = ALARM_ACTIVE;
        }
    }
    else
    {
        if(temperature_x10 >= temp_on_x10 &&
        		humidity_x10 >= humi_on_x10)
        {
            alarm_state = ALARM_NORMAL;
        }
    }

}



void Alarm_Buzzer_Update(void)
{
    if(alarm_state == ALARM_ACTIVE && alarm_enabled == 1)
    {
        /* 低电平触发：报警时蜂鸣器响 */
        HAL_GPIO_WritePin(
            BUZZER_GPIO_Port,
            BUZZER_Pin,
            GPIO_PIN_RESET
        );
    }
    else
    {
        /* 高电平：蜂鸣器关闭 */
        HAL_GPIO_WritePin(
            BUZZER_GPIO_Port,
            BUZZER_Pin,
            GPIO_PIN_SET
        );
    }
}


AlarmState Alarm_GetState(void)
{

    return alarm_state;

}



void Alarm_Enable(uint8_t enable)
{
    alarm_enabled = (enable != 0);
    Alarm_Buzzer_Update();
}


uint8_t Alarm_IsEnabled(void)
{
    return alarm_enabled;
}


void Alarm_Enable_Toggle(void)
{
    alarm_enabled = !alarm_enabled;
    Alarm_Buzzer_Update();
}


