/*
 * @Author: Xiang xin wang wxinxiang8@gmail.com
 * @Date: 2026-01-29 15:21:11
 * @LastEditors: Xiang xin wang wxinxiang8@gmail.com
 * @LastEditTime: 2026-01-29 16:34:37
 * @FilePath: \MDK-ARMd:\robot fighting\robot\Core\Src\robot_roaming.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "robot_roaming.h"
#include "shade.h"
#include "obstacle.h"
#include "dis_sensor.h"
#include "motor.h"
#include "oled.h"
#include "oled_font.h"

void detect_shade()
{
    Shade_Sensor_Init();//initialize shade sensor

    site_detect_shade();//read shade sensor data
    
}