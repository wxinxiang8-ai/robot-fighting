/*
 * @Author: Xiang xin wang wxinxiang8@gmail.com
 * @Date: 2026-03-15 15:20:29
 * @LastEditors: Xiang xin wang wxinxiang8@gmail.com
 * @LastEditTime: 2026-03-15 21:16:19
 * @FilePath: \MDK-ARMd:\robot fighting\robot\Core\Src\robot_backup.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "robot_backup.h"
#include "robot_roaming.h"
#include "motor.h"
#include "obstacle.h"
#include "shade.h"

BackUpstate_t Backup_State = GOUP_START;

extern float voltage[2];

typedef enum {
    BACKUP_SPIN = 0,
    BACKUP_RUSH_FORWARD,
    BACKUP_RUSH_BACK
} BackupStage_t;

static BackupStage_t Backup_Stage = BACKUP_SPIN;
static uint32_t Backup_StartTime = 0;

static void Backup_SwitchStage(BackupStage_t next_stage, uint32_t current_time)
{
    Backup_Stage = next_stage;
    Backup_StartTime = current_time;
}

static int Backup_IsOnStage(void)
{
    return (voltage[0] < 2.8f && voltage[1] < 2.8f);
}

static int Backup_FrontAlignReady(void)
{
    return (Obs_Data.IR7 == RESET && Obs_Data.IR8 == RESET &&
            Obs_Data.IR3 == SET   && Obs_Data.IR4 == SET);
}

static int Backup_ShouldRushBack(void)
{
    return (Obs_Data.IR9 == RESET || Obs_Data.IR10 == RESET);
}

void Backup_Init(void)
{
    Backup_Stage = BACKUP_SPIN;
    Backup_StartTime = HAL_GetTick();
    Backup_State = GOUP_START;
}

void Backup_Update(void)
{
    uint32_t current_time = HAL_GetTick();
    uint32_t elapsed_time = current_time - Backup_StartTime;

    if(Backup_State != GOUP_FALL)
    {
        Backup_Stage = BACKUP_SPIN;
        Backup_StartTime = current_time;
        return;
    }

    Obs_Sensor_ReadAll();
    site_detect_shade();

    switch (Backup_Stage)
    {
        case BACKUP_SPIN:
            drive_Left_M();
            if(Backup_FrontAlignReady())
            {
                Backup_SwitchStage(BACKUP_RUSH_FORWARD, current_time);
            }
            else if(elapsed_time >= BACKUP_SPIN_TIME_MS)
            {
                Backup_SwitchStage(BACKUP_RUSH_FORWARD, current_time);
            }
            break;

        case BACKUP_RUSH_FORWARD:
            drive_For_H();
            if(Backup_ShouldRushBack())
            {
                Backup_SwitchStage(BACKUP_RUSH_BACK, current_time);
            }
            else if(elapsed_time >= BACKUP_FORWARD_TIME_MS)
            {
                Backup_SwitchStage(BACKUP_RUSH_BACK, current_time);
            }
            break;

        case BACKUP_RUSH_BACK:
            drive_Back_H();
            if(Backup_IsOnStage())
            {
                MOTOR_StopAll();
                Backup_State = GOUP_ON;
                Backup_Stage = BACKUP_SPIN;
                Backup_StartTime = current_time;
                Roaming_Init();
            }
            else if(elapsed_time >= BACKUP_BACK_TIME_MS)
            {
                Backup_SwitchStage(BACKUP_SPIN, current_time);
            }
            break;

        default:
            Backup_Stage = BACKUP_SPIN;
            Backup_StartTime = current_time;
            break;
    }
}
