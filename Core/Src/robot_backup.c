/*
 * @file robot_backup.c
 * @brief 掉台后回台恢复状态机
 *
 * 整体思路：
 * 1. SPIN 原地左转，找到允许前冲修姿的朝向；
 * 2. 若 SPIN 连续超过 2s 仍未对正，则切入 SPIN_FORWARD 短前进脱困，随后回到 SPIN 继续搜索；
 * 3. RUSH_FORWARD 前冲修正姿态；
 * 4. 前冲结束后，若视觉识别到 'G'，则直接进入 RUSH_BACK 后退冲台；
 * 5. 若视觉未满足，则进入 ESCAPE_BACK / ESCAPE_TURN 退让并换角度后回到 RUSH_FORWARD 重试；
 * 6. RUSH_BACK 必须完整执行完后，再进入 POST_RUSH_CHECK 用双灰度连续确认是否已回到台上；
 * 7. FINISH_TURN 上台后再做一次收尾左转，随后交回漫游模块。
 */
#include "robot_backup.h"
#include "robot_roaming.h"
#include "robot_up.h"
#include "motor.h"
#include "obstacle.h"
#include "shade.h"
#include "vision_parser.h"


typedef enum {
    BACKUP_SPIN = 0,
    BACKUP_SPIN_FORWARD,
    BACKUP_RUSH_FORWARD,
    BACKUP_RUSH_BACK,
    BACKUP_POST_RUSH_CHECK,
    BACKUP_ESCAPE_BACK,
    BACKUP_ESCAPE_TURN,
    BACKUP_FINISH_TURN
} BackupStage_t;

static BackupStage_t Backup_Stage = BACKUP_SPIN;  // 当前回台状态机阶段
static uint32_t Backup_StartTime = 0;             // 当前阶段开始时刻，用于阶段超时判断
static bool Backup_Done = false;                  // 本轮回台是否完成
static uint8_t Backup_OnStageCount = 0;           // 已上台连续确认计数（灰度去抖）
static uint8_t Backup_FrontAlignCount = 0;        // 前向对正连续确认计数（IR去抖）
static char Backup_StableVisionType = 'X';        // backup 当前视觉类型（调试回读）

#define BACKUP_FRONT_ALIGN_CONFIRM_COUNT 3   // IR 前向对正确认次数
#define BACKUP_FORWARD_LEFT_SPEED 200        // 前冲修姿时左轮速度
#define BACKUP_FORWARD_RIGHT_SPEED 400       // 前冲修姿时右轮速度

/**
 * @brief 进入回台收尾阶段
 * @param current_time 当前时刻
 */
static void Backup_FinishRecovery(uint32_t current_time)
{
    Backup_OnStageCount = 0;
    Backup_Stage = BACKUP_FINISH_TURN;
    Backup_StartTime = current_time;
}

/**
 * @brief 切换回台阶段并刷新阶段计时
 * @param next_stage   下一阶段
 * @param current_time 当前时刻
 */
static void Backup_SwitchStage(BackupStage_t next_stage, uint32_t current_time)
{
    Backup_Stage = next_stage;
    Backup_StartTime = current_time;
    Backup_FrontAlignCount = 0;
}

/**
 * @brief 判断是否已回到台上
 * @return 1=V0/V1 同时低于阈值，认为已在台上；0=仍在台下
 */
static int Backup_IsOnStage(void)
{
    return (voltage_v0 < 2.8f && voltage_v1 < 2.8f);
}

/**
 * @brief 判断车头是否已经对准可前冲修姿的方向
 * @return 1=连续多次确认通过；0=仍需继续原地左转搜索
 *
 * 当前使用 IR7/IR9/IR4 组合做前向对正：
 * - IR7/IR9 为 SET：表示两侧对齐参考满足
 * - IR4 为 RESET：表示正前检测到目标/围栏方向
 */
static int Backup_FrontAlignReady(void)
{
    if(Obs_Data.IR7 == SET &&
       Obs_Data.IR9 == SET &&
       Obs_Data.IR4 == RESET)
    {
        if(Backup_FrontAlignCount < BACKUP_FRONT_ALIGN_CONFIRM_COUNT)
        {
            Backup_FrontAlignCount++;
        }
    }
    else
    {
        Backup_FrontAlignCount = 0;
    }

    return (Backup_FrontAlignCount >= BACKUP_FRONT_ALIGN_CONFIRM_COUNT);
}

/**
 * @brief 获取 backup 当前视觉类型
 * @return 当前有效视觉类型；超时/无效时返回 'X'
 */
static char Backup_GetVisionType(void)
{
    if (Vision_IsTimeout() || !vision_target.valid)
    {
        Backup_StableVisionType = 'X';
        return 'X';
    }

    Backup_StableVisionType = vision_target.type;
    return Backup_StableVisionType;
}

/**
 * @brief 判断是否满足直接后退冲台条件
 * @return 1=视觉识别到 'G'；0=仍需继续换角度重试
 */
static bool Backup_RushBackReady(void)
{
    return (Backup_GetVisionType() == 'G');
}

/**
 * @brief 处理前冲超时后的分支
 * @param current_time 当前时刻
 *
 * 规则：
 * - 若视觉识别到 'G'，说明当前姿态允许直接后退冲台，进入 RUSH_BACK；
 * - 否则先短后退脱离，再转向重试。
 */
static void Backup_HandleForwardTimeout(uint32_t current_time)
{
    if(Backup_RushBackReady())
    {
        Backup_SwitchStage(BACKUP_RUSH_BACK, current_time);
    }
    else
    {
        drive_Back_L();
        Backup_SwitchStage(BACKUP_ESCAPE_BACK, current_time);
    }
}

/**
 * @brief 在后冲结束后的允许阶段尝试判定是否已成功上台
 * @param current_time 当前时刻
 * @return 1=已判定上台并切入 FINISH_TURN；0=尚未上台
 */
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
    Backup_FrontAlignCount = 0;
    Backup_StableVisionType = 'X';
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
            /* 原地左转搜索可前冲修姿的朝向；若连续转满 2s 仍未对正，则短前进一次脱困 */
            drive_Left_L();
            if(Backup_FrontAlignReady())
            {
                Backup_SwitchStage(BACKUP_RUSH_FORWARD, current_time);
                break;
            }
            if(elapsed_time >= BACKUP_SPIN_TIMEOUT_MS)
            {
                Backup_SwitchStage(BACKUP_SPIN_FORWARD, current_time);
            }
            break;

        case BACKUP_SPIN_FORWARD:
            /* SPIN 超时后短前进脱困，结束后回到 SPIN，必要时每满 2s 再触发一次 */
            drive_For_M();
            if(Backup_TryFinishIfOnStage(current_time))
            {
                return;
            }
            if(elapsed_time >= BACKUP_SPIN_FORWARD_TIME_MS)
            {
                Backup_SwitchStage(BACKUP_SPIN, current_time);
            }
            break;

        case BACKUP_RUSH_FORWARD:
            /* 以前冲修正姿态，等待前冲结束后再判断是否满足后冲条件 */
            drive_user_defined(BACKUP_FORWARD_LEFT_SPEED, BACKUP_FORWARD_RIGHT_SPEED);
            if(Backup_TryFinishIfOnStage(current_time))
            {
                return;
            }
            if(elapsed_time >= BACKUP_FORWARD_TIME_MS)
            {
                Backup_HandleForwardTimeout(current_time);
            }
            break;

        case BACKUP_ESCAPE_BACK:
            /* 当前姿态不允许直接后冲，先短后退拉开距离 */
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
            /* 短后退后快速左转，换一个入台角度后重新进入前冲修姿 */
            drive_Left_S();
            if(elapsed_time >= BACKUP_TURN_TIME_MS)
            {
                Backup_SwitchStage(BACKUP_RUSH_FORWARD, current_time);
            }
            break;

        case BACKUP_RUSH_BACK:
            /* 后冲阶段必须完整执行，避免后半身刚上台就被提前判成已回台 */
            if(elapsed_time < GOUP_RUSH_STAGE1_TIME)
            {
                drive_user_defined(-500, -500);
            }
            else if(elapsed_time < GOUP_RUSH_STAGE2_TIME)
            {
                drive_user_defined(-600, -600);
            }
            else if(elapsed_time < BACKUP_BACK_TIME_MS)
            {
                drive_user_defined(-700, -700);
            }
            else
            {
                MOTOR_StopAll();
                Backup_OnStageCount = 0;
                Backup_SwitchStage(BACKUP_POST_RUSH_CHECK, current_time);
            }
            break;

        case BACKUP_POST_RUSH_CHECK:
            /* 后冲结束后再静止采样灰度，确认是否整车已真正回到台上 */
            MOTOR_StopAll();
            if(Backup_TryFinishIfOnStage(current_time))
            {
                return;
            }
            if(elapsed_time >= BACKUP_POST_RUSH_CHECK_TIME_MS)
            {
                Backup_OnStageCount = 0;
                Backup_SwitchStage(BACKUP_SPIN, current_time);
            }
            break;

        case BACKUP_FINISH_TURN:
            /* 已确认回台后，再左转收尾，随后交回漫游模块 */
            drive_Left_S();
            if(elapsed_time >= BACKUP_TURN_TIME_MS)
            {
                MOTOR_StopAll();
                Backup_Done = true;
                Roaming_Init();
            }
            break;

        default:
            /* 异常分支统一回到 SPIN，避免状态机卡死 */
            Backup_SwitchStage(BACKUP_SPIN, current_time);
            break;
    }
}

bool Backup_IsDone(void)
{
    return Backup_Done;
}

uint8_t Backup_DebugGetStage(void)
{
    return (uint8_t)Backup_Stage;
}

char Backup_DebugGetStableVisionType(void)
{
    return Backup_StableVisionType;
}
