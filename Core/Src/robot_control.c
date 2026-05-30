#include "robot_control.h"
#include "robot_up.h"
#include "robot_roaming.h"
#include "robot_fight.h"
#include "robot_backup.h"
#include "motor.h"
#include "vision_parser.h"

#define ROBOT_ATTACK_DIR_CONFIRM_COUNT 2U
#define ROBOT_HANDSHAKE_RESEND_PERIOD_MS 250U
#define ROBOT_HANDSHAKE_D_TIMEOUT_MS 2000U
#define ROBOT_HANDSHAKE_S_TIMEOUT_MS 2000U

typedef enum {
    ROBOT_HS_NONE = 0,
    ROBOT_HS_WAIT_D,
    ROBOT_HS_WAIT_S
} RobotHandshakeStage;

static RobotState robot_state;
static RobotHandshakeStage robot_hs_stage = ROBOT_HS_NONE;
static EnemyDir robot_attack_candidate_dir = DIR_NONE;
static uint8_t robot_attack_candidate_count = 0;
static uint32_t robot_hs_start_time = 0;
static uint32_t robot_hs_last_send_time = 0;

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

static void Robot_Control_StartHandshake(RobotHandshakeStage stage, char cmd, uint32_t now)
{
    Vision_ClearAck();
    Vision_SendCmd(cmd);
    robot_hs_stage = stage;
    robot_hs_start_time = now;
    robot_hs_last_send_time = now;
}

static void Robot_Control_EnterBackup(void)
{
    uint32_t now = HAL_GetTick();

    MOTOR_BrakeAll();
    Robot_Control_StartHandshake(ROBOT_HS_WAIT_D, 'D', now);
    Backup_Init();
    robot_state = ROBOT_BACKUP;
}

static void Robot_Control_ServiceHandshake(uint32_t now)
{
    char cmd;
    uint32_t timeout_ms;

    if(robot_hs_stage == ROBOT_HS_NONE)
    {
        return;
    }

    if(robot_hs_stage == ROBOT_HS_WAIT_D)
    {
        if(Vision_HasDAck())
        {
            robot_hs_stage = ROBOT_HS_NONE;
            return;
        }
        cmd = 'D';
        timeout_ms = ROBOT_HANDSHAKE_D_TIMEOUT_MS;
    }
    else
    {
        if(Vision_HasSAck())
        {
            robot_hs_stage = ROBOT_HS_NONE;
            robot_state = ROBOT_ROAMING;
            return;
        }
        cmd = 'S';
        timeout_ms = ROBOT_HANDSHAKE_S_TIMEOUT_MS;
    }

    if((now - robot_hs_start_time) >= timeout_ms)
    {
        if(robot_hs_stage == ROBOT_HS_WAIT_S)
        {
            robot_state = ROBOT_ROAMING;
        }
        robot_hs_stage = ROBOT_HS_NONE;
        return;
    }

    if((now - robot_hs_last_send_time) >= ROBOT_HANDSHAKE_RESEND_PERIOD_MS)
    {
        Vision_SendCmd(cmd);
        robot_hs_last_send_time = now;
    }
}

void Robot_Control_Init(void)
{
    GoUp_Init();
    Robot_Control_ResetAttackConfirm();
    robot_hs_stage = ROBOT_HS_NONE;
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

    Robot_Control_ServiceHandshake(current_time);

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
            if(robot_hs_stage == ROBOT_HS_WAIT_S)
            {
                break;
            }
            Backup_Update();
            if (Backup_IsDone())
            {
                Robot_Control_StartHandshake(ROBOT_HS_WAIT_S, 'S', current_time);
            }
            break;
    }
}
