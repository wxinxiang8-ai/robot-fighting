#include "robot_control.h"
#include "robot_up.h"
#include "robot_roaming.h"
#include "robot_fight.h"
#include "robot_backup.h"
#include "motor.h"

static RobotState robot_state;
static uint8_t enemy_confirm_count = 0;

void Robot_Control_Init(void)
{
    GoUp_Init();
    robot_state = ROBOT_GO_UP;
    enemy_confirm_count = 0;
}

void Robot_Control_Update(void)
{
    switch (robot_state)
    {
        case ROBOT_GO_UP:
            GoUp_Update();
            if (GoUp_IsDone())
            {
                Roaming_Init();
                robot_state = ROBOT_ROAMING;
            }
            break;

        case ROBOT_ROAMING:
            Roaming_Update();
            if (Roaming_IsDone())
            {
                MOTOR_StopAll();
                Backup_Init();
                robot_state = ROBOT_BACKUP;
                break;
            }
            /* 漫游中发现敌人 → 连续确认后进攻 */
            if (Fight_GetEnemyDir() != DIR_NONE)
            {
                enemy_confirm_count++;
                if (enemy_confirm_count >= 2)
                {
                    Fight_Init();
                    robot_state = ROBOT_ATTACK;
                    enemy_confirm_count = 0;
                }
            }
            else
            {
                enemy_confirm_count = 0;
            }
            break;

        case ROBOT_ATTACK:
            Fight_Update();
            if(Fight_IsDown())
            {
                MOTOR_StopAll();
                Backup_Init();
                robot_state = ROBOT_BACKUP;
                break;
            }
            if (Fight_IsDone())
            {
                Roaming_Init();
                robot_state = ROBOT_ROAMING;
                break;
            }
            break;

        case ROBOT_BACKUP:
            Backup_Update();
            if (Backup_IsDone())
            {
                robot_state = ROBOT_ROAMING;
            }
            break;
    }
}
