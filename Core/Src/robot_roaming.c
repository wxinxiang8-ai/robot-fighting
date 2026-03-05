/*
 * @Author: Xiang xin wang wxinxiang8@gmail.com
 * @Date: 2026-02-01 14:52:47
 * @LastEditors: Xiang xin wang wxinxiang8@gmail.com
 * @LastEditTime: 2026-03-04 15:00:11
 * @FilePath: \MDK-ARMd:\robot fighting\robot\Core\Src\robot_roaming.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
/*
 * @Author: Xiang xin wang wxinxiang8@gmail.com
 * @Date: 2026-02-01 14:52:47
 * @LastEditors: Xiang xin wang wxinxiang8@gmail.com
 * @LastEditTime: 2026-03-04 14:58:50
 * @FilePath: \MDK-ARMd:\robot fighting\robot\Core\Src\robot_roaming.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
/*
 * @Author: Xiang xin wang wxinxiang8@gmail.com
 * @Date: 2026-02-01 14:52:47
 * @LastEditors: Xiang xin wang wxinxiang8@gmail.com
 * @LastEditTime: 2026-02-03 14:23:36
 * @FilePath: \MDK-ARMd:\robot fighting\robot\Core\Src\robot_roaming.c
 * @Description: 机器人漫游控制，使用状态机实现非阻塞式控制
 */
#include "robot_roaming.h"
#include "shade.h"
#include "obstacle.h"
#include "dis_sensor.h"
#include "motor.h"
#include "oled.h"
#include "oled_font.h"

extern float voltage[2];

static RoamingState Roaming_Stage = ROAMING_FORWARD;
static uint32_t Roaming_StartTime = 0;
static bool Roaming_Done = false;

/**
 * @description: 检测是否在擂台上
 * @param void
 * @return int 1=在擂台上, 0=掉落擂台
 */
static int detect_shade(void)
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

/**
 * @description: 初始化漫游状态机
 * @param void
 * @return void
 */
void Roaming_Init(void)
{
    Roaming_Stage = ROAMING_FORWARD;
    Roaming_StartTime = HAL_GetTick();
    Roaming_Done = false;
}

/**
 * @description: 更新漫游状态机，根据传感器和时间执行相应动作
 * @param void
 * @return void
 */
void Roaming_Update(void)
{
    uint32_t current_time = HAL_GetTick();
    uint32_t elapsed_time = current_time - Roaming_StartTime;
    
    // 首先检测是否还在擂台上
    if(!detect_shade())
    {
        Roaming_Stage = ROAMING_DONE;
        MOTOR_StopAll();
        return;
    }
    
    switch(Roaming_Stage)
    {
        case ROAMING_FORWARD:
            // 前进状态
            drive_For_L();
            
            // 读取障碍物传感器
            Obs_Sensor_ReadAll();
            
            if(Obs_Data.IR1 == SET && Obs_Data.IR2 == SET)
            {
                // 两侧都检测到悬崖，后退
                Roaming_Stage = ROAMING_BACK;
                Roaming_StartTime = current_time;
            }
            else if(Obs_Data.IR1 == SET && Obs_Data.IR2 == RESET)
            {
                // 左侧检测到悬崖，后退准备右转
                Roaming_Stage = ROAMING_BACK;
                Roaming_StartTime = current_time;
            }
            else if(Obs_Data.IR1 == RESET && Obs_Data.IR2 == SET)
            {
                // 右侧检测到悬崖，后退准备左转
                Roaming_Stage = ROAMING_BACK;
                Roaming_StartTime = current_time;
            }
            // 否则继续前进
            break;
            
        case ROAMING_BACK:
            // 后退状态
            drive_Back_L();
            
            if(elapsed_time >= ROAMING_BACK_TIME)
            {
                // 后退完成，判断应该向哪个方向转
                Obs_Sensor_ReadAll();
                
                if(Obs_Data.IR1 == SET && Obs_Data.IR2 == SET)
                {
                    // 两侧都有悬崖，默认左转
                    Roaming_Stage = ROAMING_TURN_LEFT;
                }
                else if(Obs_Data.IR1 == SET && Obs_Data.IR2 == RESET)
                {
                    // 左侧有悬崖，右转避开
                    Roaming_Stage = ROAMING_TURN_RIGHT;
                }
                else if(Obs_Data.IR1 == RESET && Obs_Data.IR2 == SET)
                {
                    // 右侧有悬崖，左转避开
                    Roaming_Stage = ROAMING_TURN_LEFT;
                }
                else
                {
                    // 后退后前方安全，继续前进
                    Roaming_Stage = ROAMING_FORWARD;
                }
                
                Roaming_StartTime = current_time;
            }
            break;
            
        case ROAMING_TURN_LEFT:
            // 左转状态
            drive_Left_M();
            
            if(elapsed_time >= ROAMING_TURN_TIME)
            {
                // 转向完成，继续前进
                Roaming_Stage = ROAMING_FORWARD;
                Roaming_StartTime = current_time;
            }
            break;
            
        case ROAMING_TURN_RIGHT:
            // 右转状态
            drive_Right_M();
            
            if(elapsed_time >= ROAMING_TURN_TIME)
            {
                // 转向完成，继续前进
                Roaming_Stage = ROAMING_FORWARD;
                Roaming_StartTime = current_time;
            }
            break;
            
        case ROAMING_DONE:
            // 完成状态（掉落擂台）
            MOTOR_StopAll();
            Roaming_Done = true;
            break;
    }
}

/**
 * @description: 返回漫游任务是否完成（掉落擂台）
 * @param void
 * @return bool
 */
bool Roaming_IsDone(void)
{
    return Roaming_Done;
}

