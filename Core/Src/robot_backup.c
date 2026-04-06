/*
 * @Author: Xiang xin wang wxinxiang8@gmail.com
 * @Date: 2026-03-15 15:20:29
 * @LastEditors: Xiang xin wang wxinxiang8@gmail.com
 * @LastEditTime: 2026-03-27 15:29:04
 * @FilePath: \MDK-ARMd:\robot fighting\robot\Core\Src\robot_backup.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "robot_backup.h"
#include "robot_roaming.h"
#include "motor.h"
#include "obstacle.h"
#include "shade.h"

extern float voltage[2];

typedef enum {
    BACKUP_SPIN = 0,
    BACKUP_RUSH_FORWARD,
    BACKUP_RUSH_BACK,
    BACKUP_ESCAPE_BACK,
    BACKUP_ESCAPE_TURN,
    BACKUP_RETRY_FORWARD,
    BACKUP_FINISH_TURN
} BackupStage_t;

static BackupStage_t Backup_Stage = BACKUP_SPIN;
static uint32_t Backup_StartTime = 0;
static bool Backup_Done = false;
static uint8_t Backup_OnStageCount = 0;

static void Backup_FinishRecovery(uint32_t current_time)
{
    Backup_OnStageCount = 0;
    Backup_Stage = BACKUP_FINISH_TURN;
    Backup_StartTime = current_time;
}

static void Backup_SwitchStage(BackupStage_t next_stage, uint32_t current_time)
{
    Backup_Stage = next_stage;
    Backup_StartTime = current_time;
}

static int Backup_IsOnStage(void)
{
    return (voltage[1] < 2.9f);
}

static int Backup_FrontAlignReady(void)
{
    return (Obs_Data.IR7 == SET &&
            Obs_Data.IR9 == SET &&
            Obs_Data.IR4 == RESET);
}

static int Backup_ShouldRushBack(void)
{
    return (Obs_Data.IR5 == RESET);
}

static int Backup_TryFinishIfOnStage(uint32_t current_time)
{
    site_detect_shade();
    if(Backup_IsOnStage())
    {
        if(Backup_OnStageCount < 2)
        {
            Backup_OnStageCount++;
        }
    }
    else
    {
        Backup_OnStageCount = 0;
    }

    if(Backup_OnStageCount >= 2)
    {
        Backup_FinishRecovery(current_time);
        return 1;
    }

    return 0;
}

void Backup_Init(void)
{
    Backup_Stage = BACKUP_SPIN;
    Backup_StartTime = HAL_GetTick();
    Backup_Done = false;
    Backup_OnStageCount = 0;
}

void Backup_Update(void)
{
    uint32_t current_time = HAL_GetTick();
    uint32_t elapsed_time = current_time - Backup_StartTime;

    if(Backup_Done)
    {
        return;
    }

    Obs_Sensor_ReadAll();

    switch (Backup_Stage)
    {
        case BACKUP_SPIN:
            drive_Left_L();
            if(Backup_FrontAlignReady())
            {
                Backup_SwitchStage(BACKUP_RUSH_FORWARD, current_time);
            }
            break;

        case BACKUP_RUSH_FORWARD:
            drive_For_L();
            if(Backup_TryFinishIfOnStage(current_time))
            {
                return;
            }
            if(elapsed_time >= BACKUP_FORWARD_TIME_MS)
            {
                if(Backup_ShouldRushBack())
                {
                    Backup_SwitchStage(BACKUP_RUSH_BACK, current_time);
                }
                else
                {
                    drive_Back_L();
                    Backup_SwitchStage(BACKUP_ESCAPE_BACK, current_time);
                }
            }
            break;

        case BACKUP_ESCAPE_BACK:
            drive_Back_L();
            if(Backup_TryFinishIfOnStage(current_time))
            {
                return;
            }
            if(elapsed_time >= BACKUP_ESCAPE_BACK_TIME_MS)
            {
                Backup_SwitchStage(BACKUP_ESCAPE_TURN, current_time);
            }
            break;

        case BACKUP_ESCAPE_TURN:
            drive_Left_S();
            if(elapsed_time >= BACKUP_TURN_TIME_MS)
            {
                Backup_SwitchStage(BACKUP_RETRY_FORWARD, current_time);
            }
            break;

        case BACKUP_RETRY_FORWARD:
            drive_For_L();
            if(Backup_TryFinishIfOnStage(current_time))
            {
                return;
            }
            if(elapsed_time >= BACKUP_FORWARD_TIME_MS)
            {
                Backup_SwitchStage(BACKUP_RUSH_BACK, current_time);
            }
            break;

        case BACKUP_RUSH_BACK:
            drive_Back_H();
            if(Backup_TryFinishIfOnStage(current_time))
            {
                return;
            }
            if(elapsed_time < BACKUP_BACK_TIME_MS)
            {
                break;
            }
            if(Backup_IsOnStage())
            {
                Backup_FinishRecovery(current_time);
            }
            else
            {
                Backup_OnStageCount = 0;
                Backup_SwitchStage(BACKUP_SPIN, current_time);
            }
            break;

        case BACKUP_FINISH_TURN:
            drive_Left_S();
            if(elapsed_time >= BACKUP_TURN_TIME_MS)
            {
                MOTOR_StopAll();
                Backup_Done = true;
                Backup_Stage = BACKUP_SPIN;
                Backup_StartTime = current_time;
                Roaming_Init();
            }
            break;

        default:
            Backup_Stage = BACKUP_SPIN;
            Backup_StartTime = current_time;
            break;
    }
}

bool Backup_IsDone(void)
{
    return Backup_Done;
}
