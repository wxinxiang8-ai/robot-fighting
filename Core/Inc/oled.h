/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    oled.h
  * @brief   This file contains all the function prototypes for
  *          the oled.c file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __OLED_H__
#define __OLED_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"

/* USER CODE BEGIN Includes */
#include "oled_font.h"
/* USER CODE END Includes */

/* USER CODE BEGIN Private defines */
#define OLED_I2C_ADDRESS    0x78
#define OLED_WIDTH          128
#define OLED_HEIGHT         64

/* USER CODE END Private defines */

/* USER CODE BEGIN Prototypes */
void OLED_Init(void);
void OLED_Clear(void);
void OLED_DisplayOn(void);
void OLED_DisplayOff(void);
void OLED_SetCursor(uint8_t x, uint8_t y);
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char);
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String);
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length);
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowChinese24x24(uint8_t x, uint8_t y, uint16_t gb_code);
void OLED_ShowChinese24x24String(uint8_t x, uint8_t y, const char *text);
void OLED_ShowChinese16x16(uint8_t x, uint8_t y, uint16_t gb_code);
void OLED_ShowChinese16x16String(uint8_t x, uint8_t y, const char *text);
/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __OLED_H__ */
