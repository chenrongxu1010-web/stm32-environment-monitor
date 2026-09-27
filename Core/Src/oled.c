/*
 * oled.c
 *
 *  Created on: Sep 20, 2026
 *      Author: 28380
 */

#include "oled.h"

#define OLED_ADDR (0x3C << 1)

static  I2C_HandleTypeDef *oled_i2c;


static HAL_StatusTypeDef OLED_WriteCommand(uint8_t cmd)
{
    uint8_t data[2];

    data[0] = 0x00;
    data[1] = cmd;

    return HAL_I2C_Master_Transmit(
        oled_i2c,
        OLED_ADDR,
        data,
        2,
        100
    );
}

HAL_StatusTypeDef OLED_Init(I2C_HandleTypeDef *hi2c)
{
	oled_i2c = hi2c;

    if (HAL_I2C_IsDeviceReady(
            oled_i2c,
            OLED_ADDR,
            3,
            100) != HAL_OK)
    {
        return HAL_ERROR;
    }

	static const uint8_t init_cmds[] =
	    {
	        0xAE,
	        0xD5, 0x80,
	        0xA8, 0x3F,
	        0xD3, 0x00,
	        0x40,
	        0x8D, 0x14,
	        0x20, 0x00,
	        0xA1,
	        0xC8,
	        0xDA, 0x12,
	        0x81, 0x7F,
	        0xD9, 0xF1,
	        0xDB, 0x40,
	        0xA4,
	        0xA6,
	        0xAF
	    };

	    for (uint32_t i = 0;
	         i < sizeof(init_cmds) / sizeof(init_cmds[0]);
	         i++)
	    {
	        if (OLED_WriteCommand(init_cmds[i]) != HAL_OK)
	        {
	            return HAL_ERROR;
	        }
	    }

	    return HAL_OK;
}

static HAL_StatusTypeDef OLED_WriteData(uint8_t *data, uint16_t size)
{
    uint8_t buffer[129];

    if (size > 128)
    {
        return HAL_ERROR;
    }

    buffer[0] = 0x40;

    for (uint16_t i = 0; i < size; i++)
    {
        buffer[i + 1] = data[i];
    }

    return HAL_I2C_Master_Transmit(
        oled_i2c,
        OLED_ADDR,
        buffer,
        size + 1,
        100
    );
}

HAL_StatusTypeDef OLED_Clear(void)
{
    uint8_t empty_page[128] = {0};

    /* Column address：0 ~ 127 */
    if (OLED_WriteCommand(0x21) != HAL_OK)
        return HAL_ERROR;

    if (OLED_WriteCommand(0x00) != HAL_OK)
        return HAL_ERROR;

    if (OLED_WriteCommand(0x7F) != HAL_OK)
        return HAL_ERROR;


    /* Page address：0 ~ 7 */
    if (OLED_WriteCommand(0x22) != HAL_OK)
        return HAL_ERROR;

    if (OLED_WriteCommand(0x00) != HAL_OK)
        return HAL_ERROR;

    if (OLED_WriteCommand(0x07) != HAL_OK)
        return HAL_ERROR;


    for (uint8_t page = 0; page < 8; page++)
    {
        if (OLED_WriteData(empty_page, 128) != HAL_OK)
        {
            return HAL_ERROR;
        }
    }

    return HAL_OK;
}




static const uint8_t font_uppercase[26][5] =
{
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // A
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // C
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // E
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // F
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, // G
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // I
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // J
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // K
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // M
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // P
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // R
    {0x46, 0x49, 0x49, 0x49, 0x31}, // S
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // T
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
    {0x3F, 0x40, 0x38, 0x40, 0x3F}, // W
    {0x63, 0x14, 0x08, 0x14, 0x63}, // X
    {0x07, 0x08, 0x70, 0x08, 0x07}, // Y
    {0x61, 0x51, 0x49, 0x45, 0x43}  // Z
};

static const uint8_t font_digits[10][5] =
{
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
    {0x06, 0x49, 0x49, 0x29, 0x1E}  // 9
};

static const uint8_t font_dot[5] =
{
    0x00, 0x60, 0x60, 0x00, 0x00
};

static const uint8_t font_space[5] =
{
    0x00, 0x00, 0x00, 0x00, 0x00
};

static HAL_StatusTypeDef OLED_ShowChar(uint8_t col,
                                       uint8_t page,
                                       char ch)
{
    const uint8_t *bitmap;

    if (ch >= 'A' && ch <= 'Z')
    {
        bitmap = font_uppercase[ch - 'A'];
    }
    else if (ch >= '0' && ch <= '9')
    {
        bitmap = font_digits[ch - '0'];
    }
    else if (ch == '.')
    {
        bitmap = font_dot;
    }
    else if (ch == ' ')
    {
        bitmap = font_space;
    }
    else
    {
        return HAL_ERROR;
    }

    if (OLED_WriteCommand(0x21) != HAL_OK)
        return HAL_ERROR;

    if (OLED_WriteCommand(col) != HAL_OK)
        return HAL_ERROR;

    if (OLED_WriteCommand(col + 4) != HAL_OK)
        return HAL_ERROR;

    if (OLED_WriteCommand(0x22) != HAL_OK)
        return HAL_ERROR;

    if (OLED_WriteCommand(page) != HAL_OK)
        return HAL_ERROR;

    if (OLED_WriteCommand(page) != HAL_OK)
        return HAL_ERROR;

    return OLED_WriteData((uint8_t *)bitmap, 5);
}

HAL_StatusTypeDef OLED_ShowString(uint8_t col,
                                         uint8_t page,
                                         const char *str)
{
    while (*str != '\0')
    {
        if (OLED_ShowChar(col, page, *str) != HAL_OK)
        {
            return HAL_ERROR;
        }

        col += 6;
        str++;
    }

    return HAL_OK;
}

HAL_StatusTypeDef OLED_ClearPage(uint8_t page)
{
    uint8_t empty_page[128] = {0};

    if (page > 7)
    {
        return HAL_ERROR;
    }

    /* Column 0 ~ 127 */
    if (OLED_WriteCommand(0x21) != HAL_OK)
        return HAL_ERROR;

    if (OLED_WriteCommand(0x00) != HAL_OK)
        return HAL_ERROR;

    if (OLED_WriteCommand(0x7F) != HAL_OK)
        return HAL_ERROR;

    /* 只选择指定 Page */
    if (OLED_WriteCommand(0x22) != HAL_OK)
        return HAL_ERROR;

    if (OLED_WriteCommand(page) != HAL_OK)
        return HAL_ERROR;

    if (OLED_WriteCommand(page) != HAL_OK)
        return HAL_ERROR;

    return OLED_WriteData(empty_page, 128);
}

