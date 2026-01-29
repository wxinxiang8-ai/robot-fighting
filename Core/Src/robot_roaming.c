/*
 * @Author: Xiang xin wang wxinxiang8@gmail.com
 * @Date: 2026-01-29 15:21:11
 * @LastEditors: Xiang xin wang wxinxiang8@gmail.com
 * @LastEditTime: 2026-01-29 18:36:03
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
        drive_For_M();//move forward at medium speed

        Obs_Sensor_ReadAll();//read obstacle sensor data
        if(Obs_Data.IR1 == RESET && Obs_Data.IR2 == RESET)
        {
            drive_Back_M();//both front sensors detect obstacle, move backward
            HAL_Delay(500);
            drive_Left_M();//turn left at medium speed
            HAL_Delay(500);
        }
        else if(Obs_Data.IR1 == RESET && Obs_Data.IR2 == SET)
        {
            drive_Back_M();
            HAL_Delay(500);
            drive_Right_M();
            HAL_Delay(500);
        }
        else if(Obs_Data.IR2 == RESET && Obs_Data.IR1 == SET)
        {
            drive_Back_M();
            HAL_Delay(500);
            drive_Left_M();
            HAL_Delay(500);
        }
        else if(Obs_Data.IR1 == SET && Obs_Data.IR2 == SET)
        {
            drive_Back_M();
            HAL_Delay(500);
        }
        shade_status = detect_shade();
    }
}

