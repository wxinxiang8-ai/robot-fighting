#include "robot_fight.h"
#include "robot_roaming.h"
#include "motor.h"
#include "obstacle.h"
#include "shade.h"
#include "vision_parser.h"
#include "usart.h"

static FightState Fight_State = FIGHT_ENGAGE;
static uint32_t Fight_StartTime = 0;
static bool Fight_DoneFlag = false;
static uint32_t Fight_EngageLost = 0;     // ENGAGE中丢失目标的起始时间
static EnemyDir Fight_PrevRawDir = DIR_NONE;
static EnemyDir Fight_StableDir  = DIR_NONE;
static uint8_t Fight_EdgeCount = 0;
static bool Fight_DownFlag = false;

/**
 * @description: 视觉精准追踪白色物块
 * @param void
 * @return void
 */
static void Fight_VisionChase(void)
{
    int8_t d = vision_target.dir;  // [-100, +100]
    int16_t base = SPEED_MEDIUM;
    int16_t turn = (int16_t)(d * 3);
    int16_t left  = base + turn;
    int16_t right = base - turn;
    if (left > 1000)  left = 1000;
    if (left < -1000) left = -1000;
    if (right > 1000)  right = 1000;
    if (right < -1000) right = -1000;
    drive_user_defined(left, right);
}

/*======传感器读取======*/

/**
 * @description: 获取敌人方向
 * @param void
 * @return EnemyDir
 */
EnemyDir Fight_GetEnemyDir(void)
{
    /*读取八路光电传感器*/
    uint8_t nw    = (HAL_GPIO_ReadPin(FIGHT_IR_NW_PORT,    FIGHT_IR_NW_PIN)    == FIGHT_IR_TRIGGERED);
    uint8_t ne    = (HAL_GPIO_ReadPin(FIGHT_IR_NE_PORT,    FIGHT_IR_NE_PIN)    == FIGHT_IR_TRIGGERED);
    uint8_t l     = (HAL_GPIO_ReadPin(FIGHT_IR_L_PORT,     FIGHT_IR_L_PIN)     != FIGHT_IR_TRIGGERED);
    uint8_t r     = (HAL_GPIO_ReadPin(FIGHT_IR_R_PORT,     FIGHT_IR_R_PIN)     != FIGHT_IR_TRIGGERED);;
    uint8_t sw    = (HAL_GPIO_ReadPin(FIGHT_IR_SW_PORT,    FIGHT_IR_SW_PIN)    == FIGHT_IR_TRIGGERED);
    uint8_t se    = (HAL_GPIO_ReadPin(FIGHT_IR_SE_PORT,    FIGHT_IR_SE_PIN)    == FIGHT_IR_TRIGGERED);
    uint8_t front = (HAL_GPIO_ReadPin(FIGHT_IR_FRONT_PORT, FIGHT_IR_FRONT_PIN) == FIGHT_IR_TRIGGERED);
    uint8_t back  = (HAL_GPIO_ReadPin(FIGHT_IR_BACK_PORT,  FIGHT_IR_BACK_PIN)  == FIGHT_IR_TRIGGERED);

    /*判断敌人方向*/
    if(front)return DIR_FRONT;
    if(nw ||(nw && front)) return DIR_FRONT_LEFT;
    if(ne ||(ne && front)) return DIR_FRONT_RIGHT;
    if(l)  return DIR_LEFT;
    if(r)  return DIR_RIGHT;
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
    Edge_Sensor_Detect();
    return (Obs_Data.IR1 == SET || Obs_Data.IR2 == SET);
}

/**
 * @description: 检测是否掉台（连续检测到遮挡且电压高于阈值）
 * @param void
 * @return bool
 */
static bool detect_shade(void)
{
    site_detect_shade();
    return(voltage[0] > 2.7f && voltage[1] > 2.7f);
}

/*======状态机======*/

void Fight_Init(void)
{
    Fight_State = FIGHT_ENGAGE;
    Fight_DoneFlag = false;
    Fight_StartTime = HAL_GetTick();
    Fight_EngageLost = 0;
    Fight_PrevRawDir = DIR_NONE;
    Fight_StableDir  = DIR_NONE;
    Fight_EdgeCount = 0;
    Fight_DownFlag = false;
}

void Fight_Update(void)
{
    uint32_t now = HAL_GetTick();
    uint32_t elapsed = now - Fight_StartTime;
    EnemyDir raw_dir = Fight_GetEnemyDir();
    if(raw_dir == Fight_PrevRawDir)
    {
        Fight_StableDir = raw_dir;
    }
    Fight_PrevRawDir = raw_dir;
    EnemyDir dir = Fight_StableDir;

    /*======掉台安全======*/
    if(detect_shade())
    {
        Fight_DownFlag = true;
        MOTOR_StopAll();
        return;
    }

    /*======边缘安全：连续确认后后退再回漫游======*/
    if(Fight_EdgeDetected())
    {
        Fight_EdgeCount++;
        if(Fight_EdgeCount >= 3 && Fight_State != FIGHT_RETREAT)
        {
            Fight_State = FIGHT_RETREAT;
            Fight_StartTime = now;
            return;
        }
    }
    else
    {
        Fight_EdgeCount = 0;
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
                if(dir == DIR_FRONT || dir == DIR_FRONT_LEFT || dir == DIR_FRONT_RIGHT)
                {
                    Fight_StartTime = now;
                }

                /*视觉判断*/
                uint8_t vision_ok = (!Vision_IsTimeout() && vision_target.valid);

                /*己方能量块->回避*/
                if (vision_ok && (vision_target.type == 'F' || vision_target.type == 'B')) {
                    MOTOR_StopAll();
                    Fight_State    = FIGHT_DONE;
                    Fight_DoneFlag = true;
                    break;
                }

                /*白色能量块*/
                if(vision_ok && vision_target.type == 'N' )
                {
                    Fight_VisionChase();
                    break;
                }

                switch(dir)
                {
                    case DIR_FRONT:
                        drive_For_M(); 
                        break;
                    case DIR_FRONT_LEFT:
                        drive_Left_M();
                        break;
                    case DIR_FRONT_RIGHT:
                        drive_Right_M();
                        break;
                    case DIR_LEFT:
                        drive_Left_M();
                        break;
                    case DIR_RIGHT:
                        drive_Right_M();
                        break;
                    case DIR_BACK_LEFT:
                        drive_Left_S();
                        break;
                    case DIR_BACK_RIGHT:
                        drive_Right_S();
                        break;
                    case DIR_BACK:
                        drive_Right_S();
                        break;
                }
            }
            else{
                /*丢失目标，开始/继续丢失倒计时*/
                if(Fight_EngageLost == 0)
                {
                    Fight_EngageLost = now;
                }
                /*目标丢失，回到漫游状态*/
                if(now - Fight_EngageLost >= FIGHT_ENGAGE_LOST)
                {
                    MOTOR_StopAll();
                    Fight_State = FIGHT_DONE;
                    Fight_DoneFlag = true;
                }
            }

            /*交战超时,结束*/
            if((now - Fight_StartTime) >= FIGHT_ENGAGE_TIMEOUT)
            {
                MOTOR_StopAll();
                Fight_State = FIGHT_DONE;
                Fight_DoneFlag = true;
            }
            break;

        /*======撤退状态======*/
        case FIGHT_RETREAT:
            drive_Back_M();
            if(elapsed >= FIGHT_RETREAT_TIME)
            {
                Fight_State = FIGHT_TURN;
                Fight_StartTime = now;
            }
            break;

        /*======掉头状态======*/
        case FIGHT_TURN:
            drive_Left_S();
            if(elapsed >= FIGHT_TURN_TIME)
            {
                MOTOR_StopAll();
                Fight_State = FIGHT_DONE;
                Fight_DoneFlag = true;
            }
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