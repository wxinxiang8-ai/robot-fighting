/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    oled.c
  * @brief   This file provides code for the configuration
  *          of the OLED instances.
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
/* Includes ------------------------------------------------------------------*/
#include "oled.h"
#include "i2c.h"
#include "stdio.h"
#include "oled_font.h" // 添加字体头文件
#include <string.h>

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

/* USER CODE BEGIN 1 */

/**
  * @brief  OLED写命令
  * @param  Command 要写入的命令
  * @retval 无
  */
static void OLED_WriteCommand(uint8_t Command)
{
	uint8_t data[2] = {0x00, Command}; // 0x00表示命令
    HAL_I2C_Master_Transmit(&hi2c1, OLED_I2C_ADDRESS, data, 2, 100);
}

/**
  * @brief  OLED写数据
  * @param  Data 要写入的数据
  * @retval 无
  */
static void OLED_WriteData(uint8_t Data)
{
	uint8_t buffer[2] = {0x40, Data}; // 0x40表示数据
    HAL_I2C_Master_Transmit(&hi2c1, OLED_I2C_ADDRESS, buffer, 2, 100);
}

/**
  * @brief  OLED设置光标位置
  * @param  Y 以左上角为原点，向下方向的坐标，范围：0~7
  * @param  X 以左上角为原点，向右方向的坐标，范围：0~127
  * @retval 无
  */
void OLED_SetCursor(uint8_t Y, uint8_t X)
{
  if (Y >= (OLED_HEIGHT / 8) || X >= OLED_WIDTH) {
    return;
  }
	OLED_WriteCommand(0xB0 | Y);					//设置Y位置
	OLED_WriteCommand(0x10 | ((X & 0xF0) >> 4));	//设置X位置高4位
	OLED_WriteCommand(0x00 | (X & 0x0F));			//设置X位置低4位
}

/**
  * @brief  OLED清屏
  * @param  无
  * @retval 无
  */
void OLED_Clear(void)
{  
	uint8_t i, j;
  for (j = 0; j < (OLED_HEIGHT / 8); j++)
	{
		OLED_SetCursor(j, 0);
    for(i = 0; i < OLED_WIDTH; i++)
		{
			OLED_WriteData(0x00);
		}
	}
}

/**
  * @brief  开启OLED显示
  * @retval 无
  */
void OLED_DisplayOn(void)
{
    OLED_WriteCommand(0x8D);  // 使能电荷泵
    OLED_WriteCommand(0x14);
    OLED_WriteCommand(0xAF);  // 开启显示
}

/**
  * @brief  关闭OLED显示(低功耗)
  * @retval 无
  */
void OLED_DisplayOff(void)
{
    OLED_WriteCommand(0x8D);  // 关闭电荷泵
    OLED_WriteCommand(0x10);
    OLED_WriteCommand(0xAE);  // 关闭显示
}

/**
  * @brief  OLED显示一个字符
  * @param  Line 行位置，范围：1~4
  * @param  Column 列位置，范围：1~16
  * @param  Char 要显示的一个字符，范围：ASCII可见字符
  * @retval 无
  */
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char)
{      	
  if (Line == 0 || Column == 0) {
    return;
  }

  uint8_t start_page = (Line - 1) * 2;
  uint8_t start_column = (Column - 1) * 8;

  if ((start_page + 1) >= (OLED_HEIGHT / 8) || start_column >= OLED_WIDTH) {
    return;
  }

  uint8_t i;
  OLED_SetCursor(start_page, start_column);		//设置光标位置在上半部分
	for (i = 0; i < 8; i++)
	{
		OLED_WriteData(OLED_F8x16[Char - ' '][i]);			//显示上半部分内容
	}
  OLED_SetCursor(start_page + 1, start_column);	//设置光标位置在下半部分
	for (i = 0; i < 8; i++)
	{
		OLED_WriteData(OLED_F8x16[Char - ' '][i + 8]);		//显示下半部分内容
	}
}

/**
  * @brief  OLED显示字符串
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  String 要显示的字符串，范围：ASCII可见字符
  * @retval 无
  */
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String)
{
	uint8_t i;
	for (i = 0; String[i] != '\0'; i++)
	{
		OLED_ShowChar(Line, Column + i, String[i]);
	}
}

/**
  * @brief  OLED次方函数
  * @retval 返回值等于X的Y次方
  */
uint32_t OLED_Pow(uint32_t X, uint32_t Y)
{
	uint32_t Result = 1;
	while (Y--)
	{
		Result *= X;
	}
	return Result;
}

/**
  * @brief  OLED显示数字（十进制，正数）
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  Number 要显示的数字，范围：0~4294967295
  * @param  Length 要显示数字的长度，范围：1~10
  * @retval 无
  */
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
	uint8_t i;
	for (i = 0; i < Length; i++)							
	{
		OLED_ShowChar(Line, Column + i, Number / OLED_Pow(10, Length - i - 1) % 10 + '0');
	}
}

/**
  * @brief  OLED显示数字（十进制，带符号数）
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  Number 要显示的数字，范围：-2147483648~2147483647
  * @param  Length 要显示数字的长度，范围：1~10
  * @retval 无
  */
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length)
{
	uint8_t i;
	uint32_t Number1;
	if (Number >= 0)
	{
		OLED_ShowChar(Line, Column, '+');
		Number1 = Number;
	}
	else
	{
		OLED_ShowChar(Line, Column, '-');
		Number1 = -Number;
	}
	for (i = 0; i < Length; i++)							
	{
		OLED_ShowChar(Line, Column + i + 1, Number1 / OLED_Pow(10, Length - i - 1) % 10 + '0');
	}
}

/**
  * @brief  OLED显示数字（十六进制，正数）
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  Number 要显示的数字，范围：0~0xFFFFFFFF
  * @param  Length 要显示数字的长度，范围：1~8
  * @retval 无
  */
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
	uint8_t i, SingleNumber;
	for (i = 0; i < Length; i++)							
	{
		SingleNumber = Number / OLED_Pow(16, Length - i - 1) % 16;
		if (SingleNumber < 10)
		{
			OLED_ShowChar(Line, Column + i, SingleNumber + '0');
		}
		else
		{
			OLED_ShowChar(Line, Column + i, SingleNumber - 10 + 'A');
		}
	}
}

/**
  * @brief  OLED显示数字（二进制，正数）
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  Number 要显示的数字，范围：0~1111 1111 1111 1111
  * @param  Length 要显示数字的长度，范围：1~16
  * @retval 无
  */
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
	uint8_t i;
	for (i = 0; i < Length; i++)							
	{
		OLED_ShowChar(Line, Column + i, Number / OLED_Pow(2, Length - i - 1) % 2 + '0');
	}
}

/**
  * @brief  OLED初始化
  * @param  无
  * @retval 无
  */
void OLED_Init(void)
{
	HAL_Delay(20);               // 上电稳定时间
	
	OLED_WriteCommand(0xAE);	//关闭显示
	
	OLED_WriteCommand(0xD5);	//设置显示时钟分频比/振荡器频率
	OLED_WriteCommand(0x80);
	
	OLED_WriteCommand(0xA8);	//设置多路复用率
	OLED_WriteCommand(0x3F);
	
	OLED_WriteCommand(0xD3);	//设置显示偏移
	OLED_WriteCommand(0x00);
	
	OLED_WriteCommand(0x40);	//设置显示开始行
	
	OLED_WriteCommand(0xA1);	//设置左右方向，0xA1正常 0xA0左右反置
	
	OLED_WriteCommand(0xC8);	//设置上下方向，0xC8正常 0xC0上下反置

	OLED_WriteCommand(0xDA);	//设置COM引脚硬件配置
	OLED_WriteCommand(0x12);
	
	OLED_WriteCommand(0x81);	//设置对比度控制
	OLED_WriteCommand(0xCF);

	OLED_WriteCommand(0xD9);	//设置预充电周期
	OLED_WriteCommand(0xF1);

	OLED_WriteCommand(0xDB);	//设置VCOMH取消选择级别
	OLED_WriteCommand(0x30);

	OLED_WriteCommand(0xA4);	//设置整个显示打开/关闭

	OLED_WriteCommand(0xA6);	//设置正常/倒转显示

	OLED_WriteCommand(0x8D);	//设置充电泵
	OLED_WriteCommand(0x14);

	OLED_WriteCommand(0xAF);	//开启显示
		
	OLED_Clear();				//OLED清屏
}

/**
  * @brief  OLED显示24×24中文字符
  * @param  x 起始X坐标(像素), 范围: 0~104 (128-24=104)
  * @param  y 起始Y坐标(页), 范围: 0~5 (8页, 24像素占3页)
  * @param  gb_code GB2312编码(双字节), 例如0xCDAC表示"同"
  * @retval 无
  * @note   24×24字体高度约8mm, 满足比赛要求
  *         每个汉字占用24×24=576位=72字节
  *         显示位置: x以24像素递增, y占用3页(y, y+1, y+2)
  */
void OLED_ShowChinese24x24(uint8_t x, uint8_t y, uint16_t gb_code)
{
  uint8_t i, j, k;

  if (x > (OLED_WIDTH - 24) || y > ((OLED_HEIGHT / 8) - 3)) {
    return;
  }

  // 遍历字库查找匹配的汉字
  for (i = 0; i < OLED_Chinese24x24_Count; i++) {
    if (OLED_Chinese24x24[i].code == gb_code) {
      const uint8_t *glyph = OLED_Chinese24x24[i].data;

      // 字模按行存储(每行3字节=24列), 需转换为每列8点的页数据
      for (j = 0; j < 3; j++) {  // 3页(每页8行)
        OLED_SetCursor(y + j, x);

        for (k = 0; k < 24; k++) {  // 每列24像素
          uint8_t column_byte = 0;

          for (uint8_t bit = 0; bit < 8; bit++) {
            uint8_t row_index = j * 8 + bit;   // 当前页内的第bit行
            uint8_t row_byte = glyph[row_index * 3 + (k / 8)];
            uint8_t bit_mask = 0x80 >> (k % 8);

            if (row_byte & bit_mask) {
              column_byte |= (1u << bit);
            }
          }

          OLED_WriteData(column_byte);
        }
      }
      return;
    }
  }
}

/**
  * @brief  OLED连续显示24×24中文字符串
  * @param  x 起始X坐标(像素), 范围: 0~104 (128-24=104)
  * @param  y 起始Y坐标(页), 范围: 0~5 (8页, 24像素占3页)
  * @param  text 要显示的中文字符串, UTF-8编码, 当前字库仅包含“同”“异”“色”
  * @retval 无
  * @note   每个汉字使用3个字节的UTF-8编码, 显示后X坐标向右偏移24像素
  * @code
  *   OLED_ShowChinese24x24String(0, 0, "同色"); // 左上角显示“同色”
  *   OLED_ShowChinese24x24String(0, 3, "异色"); // Y=3表示向下偏移24像素
  * @endcode
  */
void OLED_ShowChinese24x24String(uint8_t x, uint8_t y, const char *text)
{
  uint8_t current_x = x;

  if (text == NULL) {
    return;
  }

  if (y > ((OLED_HEIGHT / 8) - 3)) {
    return;
  }

  while (*text != '\0') {
    uint8_t lead = (uint8_t)*text;

    // 仅处理UTF-8三字节汉字 (1110xxxx)
    if ((lead & 0xF0) == 0xE0) {
      if (current_x > (OLED_WIDTH - 24)) {
        break;  // 剩余宽度不足, 终止显示
      }

      // UTF-8转GB2312的简化映射(仅支持"同""异""色")
      uint16_t gb_code = 0;
      if (memcmp(text, "\xE5\x90\x8C", 3) == 0) {       // "同"
        gb_code = OLED_GB2312_TONG;
      } else if (memcmp(text, "\xE5\xBC\x82", 3) == 0) { // "异"
        gb_code = OLED_GB2312_YI;
      } else if (memcmp(text, "\xE8\x89\xB2", 3) == 0) { // "色"
        gb_code = OLED_GB2312_SE;
      }

      if (gb_code != 0) {
        OLED_ShowChinese24x24(current_x, y, gb_code);
      }

      text += 3;
      current_x += 24;
    } else if (lead < 0x80) {
      // ASCII字符不在24×24字库范围内, 停止处理
      break;
    } else {
      // 遇到不支持的编码, 跳过该字节避免死循环
      text++;
    }
  }
}

/**
  * @brief  OLED显示16×16中文字符
  * @param  x 起始X坐标(像素), 范围: 0~112 (128-16=112)
  * @param  y 起始Y坐标(页), 范围: 0~6 (8页, 16像素占2页)
  * @param  gb_code GB2312编码的汉字
  * @retval 无
  * @note   16×16字体高度约5.3mm, 适用于常规文字显示
  *         每个汉字占用16×16=256位=32字节
  *         显示位置: x以16像素递增, y占用2页(y, y+1)
  */
void OLED_ShowChinese16x16(uint8_t x, uint8_t y, uint16_t gb_code)
{
  uint8_t i, j, k;

  if (x > (OLED_WIDTH - 16) || y > ((OLED_HEIGHT / 8) - 2)) {
    return;
  }

  // 遍历字库查找匹配的汉字
  for (i = 0; i < OLED_Chinese16x16_Count; i++) {
    if (OLED_Chinese16x16[i].code == gb_code) {
      const uint8_t *glyph = OLED_Chinese16x16[i].data;

      // 字模按行存储(每行2字节=16列), 需转换为每列8点的页数据
      for (j = 0; j < 2; j++) {  // 2页(每页8行)
        OLED_SetCursor(y + j, x);

        for (k = 0; k < 16; k++) {  // 每列16像素
          uint8_t column_byte = 0;

          for (uint8_t bit = 0; bit < 8; bit++) {
            uint8_t row_index = j * 8 + bit;   // 当前页内的第bit行
            uint8_t row_byte = glyph[row_index * 2 + (k / 8)];
            uint8_t bit_mask = 0x80 >> (k % 8);

            if (row_byte & bit_mask) {
              column_byte |= (1u << bit);
            }
          }

          OLED_WriteData(column_byte);
        }
      }
      return;
    }
  }
}

/**
  * @brief  OLED连续显示16×16中文字符串
  * @param  x 起始X坐标(像素), 范围: 0~112 (128-16=112)
  * @param  y 起始Y坐标(页), 范围: 0~6 (8页, 16像素占2页)
  * @param  text 要显示的中文字符串, UTF-8编码, 当前字库包含"同""异""色"
  * @retval 无
  * @note   每个汉字使用3个字节的UTF-8编码, 显示后X坐标向右偏移16像素
  * @code
  *   OLED_ShowChinese16x16String(0, 0, "同色"); // 左上角显示"同色"(占32像素宽)
  *   OLED_ShowChinese16x16String(0, 2, "异色"); // Y=2表示向下偏移16像素
  * @endcode
  */
void OLED_ShowChinese16x16String(uint8_t x, uint8_t y, const char *text)
{
  uint8_t current_x = x;

  if (text == NULL) {
    return;
  }

  if (y > ((OLED_HEIGHT / 8) - 2)) {
    return;
  }

  while (*text != '\0') {
    uint8_t lead = (uint8_t)*text;

    // 仅处理UTF-8三字节汉字 (1110xxxx)
    if ((lead & 0xF0) == 0xE0) {
      if (current_x > (OLED_WIDTH - 16)) {
        break;  // 剩余宽度不足, 终止显示
      }

      // UTF-8转GB2312的简化映射(仅支持"同""异""色")
      uint16_t gb_code = 0;
      if (memcmp(text, "\xE5\x90\x8C", 3) == 0) {       // "同"
        gb_code = OLED_GB2312_TONG;
      } else if (memcmp(text, "\xE5\xBC\x82", 3) == 0) { // "异"
        gb_code = OLED_GB2312_YI;
      } else if (memcmp(text, "\xE8\x89\xB2", 3) == 0) { // "色"
        gb_code = OLED_GB2312_SE;
      }

      if (gb_code != 0) {
        OLED_ShowChinese16x16(current_x, y, gb_code);
      }

      text += 3;
      current_x += 16;
    } else if (lead < 0x80) {
      // ASCII字符不在16×16字库范围内, 停止处理
      break;
    } else {
      // 遇到不支持的编码, 跳过该字节避免死循环
      text++;
    }
  }
}

/* USER CODE END 1 */
