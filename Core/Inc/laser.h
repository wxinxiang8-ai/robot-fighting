/*
 * @Author: Xiang xin wang wxinxiang8@gmail.com
 * @Date: 2026-01-19 16:39:33
 * @LastEditors: Xiang xin wang wxinxiang8@gmail.com
 * @LastEditTime: 2026-01-22 13:41:56
 * @FilePath: \MDK-ARMd:\robot fighting\robot\Core\Inc\laser.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef LASER_H
#define LASER_H

#include "main.h"

GPIO_PinState Laser_detect_cliff_left();
GPIO_PinState Laser_detect_cliff_right();
GPIO_PinState Laser_detect_object(GPIO_TypeDef* port, uint16_t Pin);

#endif // LASER_H