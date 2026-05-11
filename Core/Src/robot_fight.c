#include "robot_fight.h"
#include "motor.h"
#include "obstacle.h"
#include "shade.h"
#include "vision_parser.h"

typedef enum {
    FIGHT_EDGE_SIDE_NONE = 0,
    FIGHT_EDGE_SIDE_LEFT,
    FIGHT_EDGE_SIDE_RIGHT,
    FIGHT_EDGE_SIDE_BOTH
} FightEdgeSide;

static FightState Fight_State = FIGHT_ENGAGE;        // 当前进攻状态机状态
static uint32_t Fight_StartTime = 0;                 // 当前状态起始时间戳
static bool Fight_DoneFlag = false;                  // 进攻阶段完成标志（通知总控回漫游）
static uint32_t Fight_EngageLost = 0;                // 交战态中丢失目标的起始时间
static EnemyDir Fight_PrevRawDir = DIR_NONE;         // 上一拍原始敌人方向（用于方向消抖）
static EnemyDir Fight_StableDir  = DIR_NONE;         // 消抖后的稳定敌人方向
static uint8_t Fight_RearEdgeCount = 0;
static char Fight_PrevVisionType = 'X';              // 上一拍视觉类型（用于视觉类型消抖）
static char Fight_StableVisionType = 'X';            // 消抖后的稳定视觉类型
static uint8_t Fight_VisionTypeCount = 0;            // 视觉类型连续命中计数
static uint8_t Fight_ShadeCount = 0;                 // 灰度掉台确认计数
static bool Fight_DownFlag = false;                  // 掉台标志（通知总控切BACKUP）
static EnemyDir Fight_TrackDir = DIR_NONE;           // 侧后追踪当前方向
static EnemyDir Fight_FrontArcHoldDir = DIR_NONE;
static uint32_t Fight_FrontArcHoldStart = 0;
static uint8_t Fight_FrontConfirmCount = 0;
static EnemyDir Fight_LastFrontArcDir = DIR_NONE;
static uint32_t Fight_LastFrontArcTime = 0;
static EnemyDir Fight_EdgeEscapeArcDir = DIR_NONE;
static FightEdgeSide Fight_EdgeSide = FIGHT_EDGE_SIDE_NONE;

static EnemyDir Fight_GetLockedTrackDir(void)
{
    if(Fight_TrackDir == DIR_LEFT || Fight_TrackDir == DIR_RIGHT)
    {
        return Fight_TrackDir;
    }
    if(Fight_StableDir == DIR_LEFT || Fight_StableDir == DIR_BACK_LEFT)
    {
        return DIR_LEFT;
    }
    if(Fight_StableDir == DIR_RIGHT || Fight_StableDir == DIR_BACK_RIGHT || Fight_StableDir == DIR_BACK)
    {
        return DIR_RIGHT;
    }
    if(Fight_PrevRawDir == DIR_LEFT || Fight_PrevRawDir == DIR_BACK_LEFT)
    {
        return DIR_LEFT;
    }
    if(Fight_PrevRawDir == DIR_RIGHT || Fight_PrevRawDir == DIR_BACK_RIGHT || Fight_PrevRawDir == DIR_BACK)
    {
        return DIR_RIGHT;
    }
    return DIR_LEFT;
}

static bool Fight_IsFrontArcDir(EnemyDir dir)
{
    return (dir == DIR_FRONT_LEFT || dir == DIR_FRONT_RIGHT);
}

static bool Fight_IsFrontArcHoldActive(uint32_t now)
{
    return (Fight_FrontArcHoldDir != DIR_NONE &&
            (now - Fight_FrontArcHoldStart) < FIGHT_FRONT_ARC_HOLD_MS);
}

static void Fight_UpdateLastFrontArcDir(uint32_t now, EnemyDir dir)
{
    if(Fight_IsFrontArcDir(dir))
    {
        Fight_LastFrontArcDir = dir;
        Fight_LastFrontArcTime = now;
    }
}

static EnemyDir Fight_GetEdgeEscapeArcDir(uint32_t now)
{
    if(Fight_LastFrontArcDir != DIR_NONE &&
       (now - Fight_LastFrontArcTime) <= FIGHT_EDGE_ARC_MEMORY_MS)
    {
        return Fight_LastFrontArcDir;
    }
    return DIR_NONE;
}

static FightEdgeSide Fight_GetEdgeSide(void)
{
    if(Obs_Data.IR1 == OBS_EDGE_TRIGGERED_STATE && Obs_Data.IR2 == OBS_EDGE_TRIGGERED_STATE)
    {
        return FIGHT_EDGE_SIDE_BOTH;
    }
    if(Obs_Data.IR1 == OBS_EDGE_TRIGGERED_STATE)
    {
        return FIGHT_EDGE_SIDE_RIGHT;
    }
    if(Obs_Data.IR2 == OBS_EDGE_TRIGGERED_STATE)
    {
        return FIGHT_EDGE_SIDE_LEFT;
    }
    return FIGHT_EDGE_SIDE_NONE;
}

static void Fight_DriveEdgeRetreat(void)
{
    if(Fight_EdgeSide == FIGHT_EDGE_SIDE_LEFT ||
       (Fight_EdgeSide == FIGHT_EDGE_SIDE_BOTH && Fight_EdgeEscapeArcDir == DIR_FRONT_LEFT))
    {
        drive_user_defined(-FIGHT_EDGE_ARC_BACK_WEAK_SPEED, -FIGHT_EDGE_ARC_BACK_STRONG_SPEED);
    }
    else if(Fight_EdgeSide == FIGHT_EDGE_SIDE_RIGHT ||
            (Fight_EdgeSide == FIGHT_EDGE_SIDE_BOTH && Fight_EdgeEscapeArcDir == DIR_FRONT_RIGHT))
    {
        drive_user_defined(-FIGHT_EDGE_ARC_BACK_STRONG_SPEED, -FIGHT_EDGE_ARC_BACK_WEAK_SPEED);
    }
    else
    {
        drive_user_defined(-FIGHT_RETREAT_SPEED, -FIGHT_RETREAT_SPEED);
    }
}

static bool Fight_ShouldTurnRight(void)
{
    return (Fight_EdgeSide == FIGHT_EDGE_SIDE_LEFT ||
            (Fight_EdgeSide == FIGHT_EDGE_SIDE_BOTH && Fight_EdgeEscapeArcDir == DIR_FRONT_LEFT) ||
            (Fight_EdgeSide == FIGHT_EDGE_SIDE_NONE && Fight_EdgeEscapeArcDir == DIR_FRONT_LEFT));
}

static EnemyDir Fight_ApplyFrontArcHold(uint32_t now, EnemyDir dir)
{
    if(Fight_IsFrontArcDir(dir))
    {
        Fight_FrontArcHoldDir = dir;
        Fight_FrontArcHoldStart = now;
        Fight_FrontConfirmCount = 0;
        return dir;
    }

    if(Fight_IsFrontArcHoldActive(now))
    {
        if(dir == DIR_FRONT)
        {
            if(Fight_FrontConfirmCount < FIGHT_FRONT_CONFIRM_COUNT)
            {
                Fight_FrontConfirmCount++;
            }
            if(Fight_FrontConfirmCount < FIGHT_FRONT_CONFIRM_COUNT)
            {
                return Fight_FrontArcHoldDir;
            }
            Fight_FrontArcHoldDir = DIR_NONE;
            Fight_FrontConfirmCount = 0;
            return dir;
        }
        else if(dir == DIR_NONE)
        {
            Fight_FrontConfirmCount = 0;
            return Fight_FrontArcHoldDir;
        }
    }
    else
    {
        Fight_FrontArcHoldDir = DIR_NONE;
    }

    if(dir != DIR_FRONT && dir != DIR_FRONT_SLIGHT_LEFT && dir != DIR_FRONT_SLIGHT_RIGHT)
    {
        Fight_FrontArcHoldDir = DIR_NONE;
    }
    Fight_FrontConfirmCount = 0;
    return dir;
}

/*======传感器读取======*/

/**
 * @description: 获取敌人方向
 * @param void
 * @return EnemyDir
 */
EnemyDir Fight_GetEnemyDir(void)
{
    uint8_t nw;
    uint8_t ne;
    uint8_t l;
    uint8_t r;
    uint8_t sw;
    uint8_t se;
    uint8_t front_left;
    uint8_t front_right;
    uint8_t back;

    Enmy_Sensor_Detect();

    nw          = (Obs_Data.IR3  == OBS_BLOCKED_STATE);
    ne          = (Obs_Data.IR5  == OBS_BLOCKED_STATE);
    l           = (Obs_Data.IR10 == OBS_BLOCKED_STATE);
    r           = (Obs_Data.IR6  == OBS_BLOCKED_STATE);
    sw          = (Obs_Data.IR9  == OBS_BLOCKED_STATE);
    se          = (Obs_Data.IR7  == OBS_BLOCKED_STATE);
    front_left  = (Obs_Data.IR4  == OBS_BLOCKED_STATE);
    front_right = (Obs_Data.IR11 == OBS_BLOCKED_STATE);
    back        = (Obs_Data.IR8  == OBS_BLOCKED_STATE);

    /*判断敌人方向*/
    if(front_left && front_right) return DIR_FRONT;
    if(front_left) return DIR_FRONT_SLIGHT_LEFT;
    if(front_right) return DIR_FRONT_SLIGHT_RIGHT;
    if(nw && ne) return DIR_FRONT;
    if(nw) return DIR_FRONT_LEFT;
    if(ne) return DIR_FRONT_RIGHT;
    if(l && r) return Fight_GetLockedTrackDir();
    if(l)  return DIR_LEFT;
    if(r)  return DIR_RIGHT;
    if(sw && se) return DIR_BACK;
    if(sw) return DIR_BACK_LEFT;
    if(se) return DIR_BACK_RIGHT;
    if(back) return DIR_BACK;
    return DIR_NONE;
}

/**
 * @description: 光电边缘检测(一票否决)
 * @param void
 * @return bool
 */
static bool Fight_EdgeDetected(void)
{
    return (Obs_Data.IR1 == OBS_EDGE_TRIGGERED_STATE || Obs_Data.IR2 == OBS_EDGE_TRIGGERED_STATE);
}

static bool Fight_RearEdgeDetected(void)
{
    return (Obs_Data.IR12 != OBS_BLOCKED_STATE && Obs_Data.IR13 != OBS_BLOCKED_STATE);
}

static bool detect_shade(void)
{
    site_detect_shade();

    if((voltage_v0 > 2.8f && voltage_v0 < 3.0f) &&
       (voltage_v1 > 2.8f && voltage_v1 < 3.0f))
    {
        if(Fight_ShadeCount < FIGHT_SHADE_CONFIRM_COUNT)
        {
            Fight_ShadeCount++;
        }
    }
    else
    {
        Fight_ShadeCount = 0;
    }

    return (Fight_ShadeCount >= FIGHT_SHADE_CONFIRM_COUNT);
}

static char Fight_GetStableVisionType(void)
{
    if (Vision_IsTimeout() || !vision_target.valid)
    {
        Fight_PrevVisionType = 'X';
        Fight_StableVisionType = 'X';
        Fight_VisionTypeCount = 0;
        return 'X';
    }

    if (vision_target.type != Fight_PrevVisionType)
    {
        Fight_PrevVisionType = vision_target.type;
        Fight_VisionTypeCount = 1;
    }
    else if (Fight_VisionTypeCount < FIGHT_VISION_CONFIRM_COUNT)
    {
        Fight_VisionTypeCount++;
    }

    if (Fight_VisionTypeCount >= FIGHT_VISION_CONFIRM_COUNT)
    {
        Fight_StableVisionType = Fight_PrevVisionType;
    }

    return Fight_StableVisionType;
}

/*======状态机======*/

void Fight_Init(void)
{
    Motor_Ramp_SyncFromCurrent();
    Fight_State = FIGHT_ENGAGE;
    Fight_DoneFlag = false;
    Fight_StartTime = HAL_GetTick();
    Fight_EngageLost = 0;
    Fight_PrevRawDir = DIR_NONE;
    Fight_StableDir = DIR_NONE;
    Fight_RearEdgeCount = 0;
    Fight_PrevVisionType = 'X';
    Fight_StableVisionType = 'X';
    Fight_VisionTypeCount = 0;
    Fight_ShadeCount = 0;
    Fight_DownFlag = false;
    Fight_TrackDir = DIR_NONE;
    Fight_FrontArcHoldDir = DIR_NONE;
    Fight_FrontArcHoldStart = 0;
    Fight_FrontConfirmCount = 0;
    Fight_LastFrontArcDir = DIR_NONE;
    Fight_LastFrontArcTime = 0;
    Fight_EdgeEscapeArcDir = DIR_NONE;
    Fight_EdgeSide = FIGHT_EDGE_SIDE_NONE;
}

void Fight_InitWithDir(EnemyDir dir)
{
    Fight_Init();
    Fight_PrevRawDir = dir;
    Fight_StableDir = dir;
    if(Fight_IsFrontArcDir(dir))
    {
        Fight_LastFrontArcDir = dir;
        Fight_LastFrontArcTime = HAL_GetTick();
    }
}

void Fight_Update(void)
{
    uint32_t now = HAL_GetTick();
    uint32_t elapsed = now - Fight_StartTime;
    EnemyDir raw_dir;
    EnemyDir dir;
    char vision_type;

    Edge_Sensor_Detect();
    if(Fight_State != FIGHT_EDGE_STOP &&
       Fight_State != FIGHT_RETREAT &&
       Fight_State != FIGHT_TURN &&
       Fight_State != FIGHT_DONE &&
       Fight_EdgeDetected())
    {
        MOTOR_BrakeAll();
        Fight_ShadeCount = 0;
        Fight_RearEdgeCount = 0;
        Fight_EdgeEscapeArcDir = Fight_GetEdgeEscapeArcDir(now);
        Fight_EdgeSide = Fight_GetEdgeSide();
        Fight_State = FIGHT_EDGE_STOP;
        Fight_StartTime = now;
        return;
    }

    raw_dir = Fight_GetEnemyDir();
    if(raw_dir == Fight_PrevRawDir)
    {
        Fight_StableDir = raw_dir;
    }
    Fight_PrevRawDir = raw_dir;
    dir = Fight_ApplyFrontArcHold(now, Fight_StableDir);
    vision_type = Fight_GetStableVisionType();
    if(Fight_State == FIGHT_ENGAGE)
    {
        Fight_UpdateLastFrontArcDir(now, dir);
    }

    /*======掉台安全======*/
    if(detect_shade())
    {
        Fight_DownFlag = true;
        MOTOR_BrakeAll();
        return;
    }

    if(Fight_State != FIGHT_EDGE_STOP &&
       Fight_State != FIGHT_REAR_EDGE_STOP &&
       Fight_State != FIGHT_REAR_ESCAPE &&
       Fight_State != FIGHT_DONE)
    {
        if(!Fight_EdgeDetected() && Fight_RearEdgeDetected())
        {
            if(Fight_RearEdgeCount < FIGHT_REAR_EDGE_CONFIRM_COUNT)
            {
                Fight_RearEdgeCount++;
            }
            if(Fight_RearEdgeCount >= FIGHT_REAR_EDGE_CONFIRM_COUNT)
            {
                MOTOR_BrakeAll();
                Fight_ShadeCount = 0;
                Fight_RearEdgeCount = 0;
                Fight_State = FIGHT_REAR_EDGE_STOP;
                Fight_StartTime = now;
                return;
            }
        }
        else
        {
            Fight_RearEdgeCount = 0;
        }
    }

    switch(Fight_State)
    {
        /*======交战状态======*/
        case FIGHT_ENGAGE:
            if(dir != DIR_NONE)
            {
                /*有目标，清除交战丢失时间*/
                Fight_EngageLost = 0;

                /*正面接触后重置交战时间*/
                if(dir == DIR_FRONT || dir == DIR_FRONT_SLIGHT_LEFT || dir == DIR_FRONT_SLIGHT_RIGHT ||
                   dir == DIR_FRONT_LEFT || dir == DIR_FRONT_RIGHT)
                {
                    Fight_StartTime = now;
                }

                /*视觉类型消抖后再判断*/
                if (vision_type == 'F' || vision_type == 'B') {
                    MOTOR_BrakeAll();
                    Fight_State = FIGHT_FB_TURN;
                    Fight_StartTime = now;
                    break;
                }

                /*IR方向追踪：前扇区走Ramp，侧后方直接进原地旋转*/
                switch(dir)
                {
                    case DIR_FRONT:
                        Motor_Ramp_SetTarget(FIGHT_TRACK_PUSH_SPEED, FIGHT_TRACK_PUSH_SPEED);
                        Motor_Ramp_Update();
                        break;
                    case DIR_FRONT_SLIGHT_LEFT:
                        Motor_Ramp_SetTarget(FIGHT_TRACK_FRONT_SMALL_ARC_INNER_SPEED,
                                             FIGHT_TRACK_FRONT_SMALL_ARC_OUTER_SPEED);
                        Motor_Ramp_Update();
                        break;
                    case DIR_FRONT_SLIGHT_RIGHT:
                        Motor_Ramp_SetTarget(FIGHT_TRACK_FRONT_SMALL_ARC_OUTER_SPEED,
                                             FIGHT_TRACK_FRONT_SMALL_ARC_INNER_SPEED);
                        Motor_Ramp_Update();
                        break;
                    case DIR_FRONT_LEFT:
                        Motor_Ramp_SetTarget(FIGHT_TRACK_FRONT_ARC_INNER_SPEED,
                                             FIGHT_TRACK_FRONT_ARC_OUTER_SPEED);
                        Motor_Ramp_Update();
                        break;
                    case DIR_FRONT_RIGHT:
                        Motor_Ramp_SetTarget(FIGHT_TRACK_FRONT_ARC_OUTER_SPEED,
                                             FIGHT_TRACK_FRONT_ARC_INNER_SPEED);
                        Motor_Ramp_Update();
                        break;
                    case DIR_LEFT:
                    case DIR_BACK_LEFT:
                        Motor_Ramp_SyncFromCurrent();
                        Fight_State = FIGHT_TRACK_SPIN;
                        Fight_TrackDir = DIR_LEFT;
                        Fight_EngageLost = 0;
                        Fight_StartTime = now;
                        break;
                    case DIR_RIGHT:
                    case DIR_BACK_RIGHT:
                    case DIR_BACK:
                        Motor_Ramp_SyncFromCurrent();
                        Fight_State = FIGHT_TRACK_SPIN;
                        Fight_TrackDir = DIR_RIGHT;
                        Fight_EngageLost = 0;
                        Fight_StartTime = now;
                        break;
                    default:
                        break;
                }
            }
            else{
                /*丢失目标：保持当前速度继续跑，不立刻减速*/
                Motor_Ramp_Update();
                if(Fight_EngageLost == 0)
                {
                    Fight_EngageLost = now;
                }
                /*丢失超时，直接回漫游*/
                if(now - Fight_EngageLost >= FIGHT_ENGAGE_LOST)
                {
                    Fight_State = FIGHT_DONE;
                }
            }

            /*交战超时,结束：直接交给 DONE 统一停机收尾*/
            if((now - Fight_StartTime) >= FIGHT_ENGAGE_TIMEOUT)
            {
                Fight_State = FIGHT_DONE;
            }
            break;

        /*======边缘确认后短暂停顿======*/
        case FIGHT_EDGE_STOP:
            if(!MOTOR_IsBraking() && elapsed >= FIGHT_EDGE_STOP_TIME)
            {
                Fight_State = FIGHT_RETREAT;
                Fight_StartTime = now;
            }
            break;

        case FIGHT_REAR_EDGE_STOP:
            if(!MOTOR_IsBraking() && elapsed >= FIGHT_EDGE_STOP_TIME)
            {
                Fight_State = FIGHT_REAR_ESCAPE;
                Fight_StartTime = now;
            }
            break;

        case FIGHT_REAR_ESCAPE:
            drive_user_defined(FIGHT_REAR_ESCAPE_SPEED, FIGHT_REAR_ESCAPE_SPEED);
            if(elapsed >= FIGHT_REAR_ESCAPE_TIME)
            {
                Fight_State = FIGHT_DONE;
            }
            break;

        /*======撤退状态======*/
        case FIGHT_RETREAT:
            Fight_DriveEdgeRetreat();
            if(elapsed >= ((Fight_EdgeSide == FIGHT_EDGE_SIDE_NONE) ? FIGHT_RETREAT_TIME : FIGHT_EDGE_ARC_BACK_TIME))
            {
                Fight_State = FIGHT_TURN;
                Fight_StartTime = now;
            }
            break;

        /*======边缘恢复掉头状态======*/
        case FIGHT_TURN:
            if(Fight_ShouldTurnRight())
            {
                drive_user_defined(FIGHT_TURN_SPEED, -FIGHT_TURN_SPEED);
            }
            else
            {
                drive_user_defined(-FIGHT_TURN_SPEED, FIGHT_TURN_SPEED);
            }
            if(elapsed >= ((Fight_EdgeSide == FIGHT_EDGE_SIDE_NONE && Fight_EdgeEscapeArcDir == DIR_NONE) ? FIGHT_TURN_TIME : FIGHT_EDGE_ARC_TURN_TIME))
            {
                Fight_EdgeEscapeArcDir = DIR_NONE;
                Fight_EdgeSide = FIGHT_EDGE_SIDE_NONE;
                Fight_State = FIGHT_DONE;
            }
            break;

        /*======F/B回避先后退======*/
        case FIGHT_FB_TURN:
            drive_user_defined(-FIGHT_FB_RETREAT_SPEED, -FIGHT_FB_RETREAT_SPEED);
            if(elapsed >= FIGHT_FB_RETREAT_TIME)
            {
                Fight_State = FIGHT_FORWARD;
                Fight_StartTime = now;
            }
            break;

        /*======F/B回避后180°掉头======*/
        case FIGHT_FORWARD:
            drive_user_defined(-FIGHT_TURN_SPEED, FIGHT_TURN_SPEED);
            if(elapsed >= FIGHT_TURN_TIME)
            {
                Fight_State = FIGHT_FB_ADVANCE;
                Fight_StartTime = now;
            }
            break;

        /*======F/B回避后短前进======*/
        case FIGHT_FB_ADVANCE:
            drive_user_defined(FIGHT_FB_ADVANCE_SPEED, FIGHT_FB_ADVANCE_SPEED);
            if(elapsed >= FIGHT_FB_FORWARD_TIME)
            {
                Fight_State = FIGHT_DONE;
            }
            break;

        /*======侧后向追踪：直接原地补角，直到前扇区命中======*/
        case FIGHT_TRACK_SPIN:
            if(dir == DIR_FRONT || dir == DIR_FRONT_SLIGHT_LEFT || dir == DIR_FRONT_SLIGHT_RIGHT ||
                   dir == DIR_FRONT_LEFT || dir == DIR_FRONT_RIGHT)
            {
                Fight_TrackDir = DIR_NONE;
                Fight_EngageLost = 0;
                Motor_Ramp_SyncFromCurrent();
                Fight_State = FIGHT_ENGAGE;
                Fight_StartTime = now;
                break;
            }

            if(dir == DIR_LEFT || dir == DIR_BACK_LEFT)
            {
                Fight_TrackDir = DIR_LEFT;
                Fight_EngageLost = 0;
            }
            else if(dir == DIR_RIGHT || dir == DIR_BACK_RIGHT || dir == DIR_BACK)
            {
                Fight_TrackDir = DIR_RIGHT;
                Fight_EngageLost = 0;
            }
            else if(dir == DIR_NONE)
            {
                if(Fight_EngageLost == 0)
                {
                    Fight_EngageLost = now;
                }
                else if((now - Fight_EngageLost) >= FIGHT_ENGAGE_LOST)
                {
                    Fight_State = FIGHT_DONE;
                    break;
                }
            }

            if(Fight_TrackDir == DIR_LEFT)
            {
                Motor_Ramp_SetTarget(-FIGHT_TRACK_SPIN_SPEED, FIGHT_TRACK_SPIN_SPEED);
            }
            else
            {
                Motor_Ramp_SetTarget(FIGHT_TRACK_SPIN_SPEED, -FIGHT_TRACK_SPIN_SPEED);
            }
            Motor_Ramp_Update();
            break;

        /*======完成状态======*/
        case FIGHT_DONE:
            MOTOR_StopAll();
            Fight_DoneFlag = true;
            break;
    }

}

bool Fight_IsDone(void)
{
    return Fight_DoneFlag;
}

bool Fight_IsDown(void)
{
    return Fight_DownFlag;
}
