#include "robot_control.h"
#include "robot_up.h"
#include "robot_roaming.h"
#include "robot_fight.h"
#include "robot_backup.h"
#include "motor.h"

static RobotState robot_state;

void Robot_Control_Init(void)
{
    GoUp_Init();
    robot_state = ROBOT_GO_UP;
}

void Robot_Control_Update(void)
{
    /* 掉台一票否决：任何状态下检测到掉台，立即切BACKUP */
    if (Backup_State == GOUP_FALL && robot_state != ROBOT_BACKUP)
    {
        MOTOR_StopAll();
        robot_state = ROBOT_BACKUP;
    }

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
            /* 漫游中检测到敌人 → 进攻（暂未接入，预留） */
            break;

        case ROBOT_ATTACK:
            /* 暂未接入 */
            break;

        case ROBOT_BACKUP:
            Backup_Update();
            /* backup完成后 Backup_State 会被设为 GOUP_ON，且内部已调 Roaming_Init() */
            if (Backup_State == GOUP_ON)
            {
                robot_state = ROBOT_ROAMING;
            }
            break;
    }
}
