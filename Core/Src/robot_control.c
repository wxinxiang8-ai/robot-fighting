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
            }
            break;

        case ROBOT_ATTACK:
            /* 暂未接入 */
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
