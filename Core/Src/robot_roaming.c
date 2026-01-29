/*
 * @Author: Xiang xin wang wxinxiang8@gmail.com
 * @Date: 2026-01-29 15:21:11
 * @LastEditors: Xiang xin wang wxinxiang8@gmail.com
 * @LastEditTime: 2026-01-29 19:05:56
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

extern float voltage[2];

int detect_shade()
{
    Shade_Sensor_Init();//initialize shade sensor

    site_detect_shade();//read shade sensor data
    
    if(voltage[0]<3.0f&&voltage[1]<3.0f)
    {
        OLED_ShowString(4, 0, (char*)"safe");
        return 1;
    }
    else
    {
        OLED_ShowString(4, 0, (char*)"unsafe");
        return 0;
    }
}

void robot_roaming()
{
    int shade_status = 0;

    shade_status = detect_shade();

    while(shade_status)
    {
        // 读取前方地面检测传感器（IR1左侧，IR2右侧）
        // RESET(0) = 悬崖/无地面, SET(1) = 有地面
        Obs_Sensor_ReadAll();
        
        if(Obs_Data.IR1 == RESET && Obs_Data.IR2 == RESET)
        {
            // 两侧都检测到悬崖，后退并转向
            drive_Back_M();
            HAL_Delay(500);
            drive_Left_M();
            HAL_Delay(500);
        }
        else if(Obs_Data.IR1 == RESET && Obs_Data.IR2 == SET)
        {
            // 左侧检测到悬崖，后退并右转避开
            drive_Back_M();
            HAL_Delay(500);
            drive_Right_M();
            HAL_Delay(500);
        }
        else if(Obs_Data.IR1 == SET && Obs_Data.IR2 == RESET)
        {
            // 右侧检测到悬崖，后退并左转避开
            drive_Back_M();
            HAL_Delay(500);
            drive_Left_M();
            HAL_Delay(500);
        }
        else
        {
            // 两侧都检测到地面，安全，继续前进
            drive_For_M();
        }
        
        // 每次判断后检测是否还在擂台上
        shade_status = detect_shade();
    }
}

