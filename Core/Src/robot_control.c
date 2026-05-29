#include "robot_control.h"
#include "robot_up.h"
#include "robot_roaming.h"
#include "robot_fight.h"
#include "robot_backup.h"
#include "motor.h"
#include "vision_parser.h"

#define ROBOT_ATTACK_DIR_CONFIRM_COUNT 2U
#define ROBOT_BACKUP_DONE_CMD_TIME_MS 500U
#define ROBOT_BACKUP_DONE_CMD_INTERVAL_MS 100U

static RobotState robot_state;
static EnemyDir robot_attack_candidate_dir = DIR_NONE;
static uint8_t robot_attack_candidate_count = 0;
static uint8_t robot_backup_done_cmd_active = 0;
static uint32_t robot_backup_done_cmd_start_time = 0;
static uint32_t robot_backup_done_cmd_last_time = 0;

static void Robot_Control_ResetAttackConfirm(void)
{
    robot_attack_candidate_dir = DIR_NONE;
    robot_attack_candidate_count = 0;
}

static void Robot_Control_EnterRoaming(void)
{
    Roaming_Init();
    Robot_Control_ResetAttackConfirm();
    robot_state = ROBOT_ROAMING;
}

static void Robot_Control_EnterBackup(void)
{
    robot_backup_done_cmd_active = 0;
    MOTOR_BrakeAll();
    Vision_SendCmd('D');
    Backup_Init();
    robot_state = ROBOT_BACKUP;
}

static void Robot_Control_StartBackupDoneNotify(uint32_t now)
{
    Vision_SendCmd('S');
    robot_backup_done_cmd_active = 1;
    robot_backup_done_cmd_start_time = now;
    robot_backup_done_cmd_last_time = now;
}

static void Robot_Control_ServiceBackupDoneNotify(uint32_t now)
{
    if(!robot_backup_done_cmd_active)
    {
        return;
    }

    if((now - robot_backup_done_cmd_start_time) >= ROBOT_BACKUP_DONE_CMD_TIME_MS)
    {
        robot_backup_done_cmd_active = 0;
        return;
    }

    if((now - robot_backup_done_cmd_last_time) >= ROBOT_BACKUP_DONE_CMD_INTERVAL_MS)
    {
        Vision_SendCmd('S');
        robot_backup_done_cmd_last_time = now;
    }
}

void Robot_Control_Init(void)
{
    GoUp_Init();
    Robot_Control_ResetAttackConfirm();
    robot_backup_done_cmd_active = 0;
    robot_state = ROBOT_GO_UP;
}

RobotState Robot_Control_GetState(void)
{
    return robot_state;
}

void Robot_Control_Update(void)
{
    EnemyDir enemy_dir;
    uint32_t current_time = HAL_GetTick();

    Robot_Control_ServiceBackupDoneNotify(current_time);

    switch (robot_state)
    {
        case ROBOT_GO_UP:
            GoUp_Update();
            if (GoUp_IsDone())
            {
                Robot_Control_EnterRoaming();
            }
            break;

        case ROBOT_ROAMING:
            Roaming_Update();
            if (Roaming_IsDone())
            {
                Robot_Control_EnterBackup();
                break;
            }
            enemy_dir = Fight_GetEnemyDir();
            if (enemy_dir == DIR_NONE)
            {
                Robot_Control_ResetAttackConfirm();
                break;
            }
            if (enemy_dir != robot_attack_candidate_dir)
            {
                robot_attack_candidate_dir = enemy_dir;
                robot_attack_candidate_count = 1;
                break;
            }
            if (robot_attack_candidate_count < ROBOT_ATTACK_DIR_CONFIRM_COUNT)
            {
                robot_attack_candidate_count++;
            }
            if (robot_attack_candidate_count >= ROBOT_ATTACK_DIR_CONFIRM_COUNT)
            {
                Fight_InitWithDir(enemy_dir);
                Robot_Control_ResetAttackConfirm();
                robot_state = ROBOT_ATTACK;
            }
            break;

        case ROBOT_ATTACK:
            Fight_Update();
            if(Fight_IsDown())
            {
                Robot_Control_EnterBackup();
                break;
            }
            if (Fight_IsDone())
            {
                Robot_Control_EnterRoaming();
                break;
            }
            break;

        case ROBOT_BACKUP:
            Backup_Update();
            if (Backup_IsDone())
            {
                Robot_Control_StartBackupDoneNotify(current_time);
                robot_state = ROBOT_ROAMING;
            }
            break;
    }
}
