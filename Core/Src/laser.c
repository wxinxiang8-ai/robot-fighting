/*
 * @Author: Xiang xin wang wxinxiang8@gmail.com
 * @Date: 2026-01-19 16:39:25
 * @LastEditors: Xiang xin wang wxinxiang8@gmail.com
 * @LastEditTime: 2026-01-22 14:02:40
 * @FilePath: \MDK-ARMd:\robot fighting\robot\Core\Src\laser.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "laser.h"
#include "gpio.h"
#include "stm32f4xx_hal.h"


GPIO_PinState Laser_detect_cliff_left()
{
    GPIO_PinState state;
    state = HAL_GPIO_ReadPin(IR_1_GPIO_Port, IR_1_Pin);
    return state;
}
GPIO_PinState Laser_detect_cliff_right()
{
    GPIO_PinState state;
    state = HAL_GPIO_ReadPin(IR_2_GPIO_Port, IR_2_Pin);
    return state;
}
GPIO_PinState Laser_detect_object(GPIO_TypeDef* port, uint16_t Pin)
{
    GPIO_PinState state;
    state = HAL_GPIO_ReadPin(port, Pin);
    return state;
}

