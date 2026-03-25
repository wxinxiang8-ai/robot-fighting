/*
 * @Author: Xiang xin wang wxinxiang8@gmail.com
 * @Date: 2026-02-01 14:52:47
 * @LastEditors: Xiang xin wang wxinxiang8@gmail.com
 * @LastEditTime: 2026-03-24 22:05:28
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
#include "motor.h"

extern float voltage[2];

static RoamingState Roaming_Stage = ROAMING_FORWARD;
static uint32_t Roaming_StartTime = 0;
static bool Roaming_Done = false;
static RoamingBackReason Roaming_BackReason = BACK_REASON_NONE;
static uint32_t Roaming_TurnTime = ROAMING_TURN_TIME;
static RoamingBackReason Roaming_PendingBackReason = BACK_REASON_NONE;
static uint32_t Roaming_BackDebounceStart = 0;

/**
 * @description: 检测是否掉落擂台
 * @param void
 * @return int 1=掉落擂台, 0=在擂台上
 */
static int detect_shade(void)
{
    site_detect_shade();//read shade sensor data

    if(voltage[0] > 2.7f && voltage[1] > 2.7f)
    {
        return 1;
    }
    else
    {
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
    Shade_Sensor_Init();
    Roaming_Stage = ROAMING_FORWARD;
    Roaming_StartTime = HAL_GetTick();
    Roaming_Done = false;
    Roaming_BackReason = BACK_REASON_NONE;
    Roaming_PendingBackReason = BACK_REASON_NONE;
    Roaming_BackDebounceStart = 0;
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
    
    // 检测到掉台信号时，立即停机
    if(detect_shade())
    {
        Roaming_Stage = ROAMING_DONE;
        Roaming_Done = true;
        MOTOR_StopAll();
        return;
    }
    
    RoamingBackReason current_reason = BACK_REASON_NONE;

    switch(Roaming_Stage)
    {
        case ROAMING_FORWARD:
            // 前进状态
            drive_For_L();

            // 读取障碍物传感器
            Obs_Sensor_ReadAll();

             // 根据传感器状态决定是否后退

            current_reason = BACK_REASON_NONE;
            if(Obs_Data.IR1 == SET && Obs_Data.IR2 == SET)
            {
                // 两侧都检测到悬崖，后退
                current_reason = BACK_REASON_BOTH;
            }
            else if(Obs_Data.IR1 == SET && Obs_Data.IR2 == RESET)
            {
                // 左侧检测到悬崖，后退准备右转
                current_reason = BACK_REASON_RIGHT;
            }
            else if(Obs_Data.IR1 == RESET && Obs_Data.IR2 == SET)
            {
                // 右侧检测到悬崖，后退准备左转
                current_reason = BACK_REASON_LEFT;
            }

            if(current_reason == BACK_REASON_NONE)
            {
                Roaming_PendingBackReason = BACK_REASON_NONE;
                Roaming_BackDebounceStart = 0;
            }
            else if(current_reason != Roaming_PendingBackReason)
            {
                Roaming_PendingBackReason = current_reason;
                Roaming_BackDebounceStart = current_time;
            }
            else if((current_time - Roaming_BackDebounceStart) >= ROAMING_EDGE_DEBOUNCE_MS)
            {
                Roaming_BackReason = current_reason;
                Roaming_Stage = ROAMING_BACK;
                Roaming_StartTime = current_time;
                Roaming_PendingBackReason = BACK_REASON_NONE;
                Roaming_BackDebounceStart = 0;
            }
            // 否则继续前进
            break;
            
        case ROAMING_BACK:
            // 后退状态
            drive_Back_L();
            
            if(elapsed_time >= ROAMING_BACK_TIME)
            {
                // 后退完成，根据触发后退时锁存的原因决定转向
                if(Roaming_BackReason == BACK_REASON_BOTH)
                {
                    Roaming_Stage = ROAMING_TURN_RIGHT;
                    Roaming_TurnTime = ROAMING_BACKAND_TURN_TIME;
                }
                else if(Roaming_BackReason == BACK_REASON_LEFT)
                {
                    Roaming_Stage = ROAMING_TURN_LEFT;
                    Roaming_TurnTime = ROAMING_TURN_TIME;
                }
                else if(Roaming_BackReason == BACK_REASON_RIGHT)
                {
                    Roaming_Stage = ROAMING_TURN_RIGHT;
                    Roaming_TurnTime = ROAMING_TURN_TIME;
                }
                else
                {
                    Roaming_Stage = ROAMING_FORWARD;
                    Roaming_TurnTime = ROAMING_TURN_TIME;
                }
                Roaming_BackReason = BACK_REASON_NONE;
                
                Roaming_StartTime = current_time;
            }
            break;
            
        case ROAMING_TURN_LEFT:
            // 左转状态
            drive_Left_M();
            
            if(elapsed_time >= Roaming_TurnTime)
            {
                // 转向完成，继续前进
                Roaming_Stage = ROAMING_FORWARD;
                Roaming_StartTime = current_time;
            }
            break;
            
        case ROAMING_TURN_RIGHT:
            // 右转状态
            drive_Right_M();
            
            if(elapsed_time >= Roaming_TurnTime)
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

