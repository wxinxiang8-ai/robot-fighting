#include "robot_control.h"
#include "robot_up.h"
#include "robot_roaming.h"
#include "robot_fight.h"
#include "robot_backup.h"
#include "motor.h"
#include "vision_parser.h"

static RobotState robot_state;

void Robot_Control_Init(void)
{
    GoUp_Init();
    robot_state = ROBOT_GO_UP;
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
                // /* 视觉确认不是己方能量块才进攻 */
                // uint8_t vision_ok = (!Vision_IsTimeout() && vision_target.valid);
                // if (vision_ok && (vision_target.type == 'F' || vision_target.type == 'B'))
                // {
                //     enemy_confirm_count = 0;
                //     break;  /* 己方能量块，不进攻 */
                // }
                // enemy_confirm_count++;
                // if (enemy_confirm_count >= 2)
                // {
                Fight_Init();
                robot_state = ROBOT_ATTACK;
                //enemy_confirm_count = 0;
            }
            // }
            // else
            // {
            //     enemy_confirm_count = 0;
            // }
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
