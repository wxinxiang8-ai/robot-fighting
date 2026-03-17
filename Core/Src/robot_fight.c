#include "robot_fight.h"
#include "robot_roaming.h"
#include "motor.h"
#include "obstacle.h"
#include "shade.h"

static FightState Fight_State = FIGHT_ENGAGE;
static uint32_t Fight_StartTime = 0;
static bool Fight_DoneFlag = false;
static uint32_t Fight_EngageLost = 0;     // ENGAGE中丢失目标的起始时间

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
    uint8_t l     = (HAL_GPIO_ReadPin(FIGHT_IR_L_PORT,     FIGHT_IR_L_PIN)     == FIGHT_IR_TRIGGERED);
    uint8_t r     = (HAL_GPIO_ReadPin(FIGHT_IR_R_PORT,     FIGHT_IR_R_PIN)     == FIGHT_IR_TRIGGERED);
    uint8_t sw    = (HAL_GPIO_ReadPin(FIGHT_IR_SW_PORT,    FIGHT_IR_SW_PIN)    == FIGHT_IR_TRIGGERED);
    uint8_t se    = (HAL_GPIO_ReadPin(FIGHT_IR_SE_PORT,    FIGHT_IR_SE_PIN)    == FIGHT_IR_TRIGGERED);
    uint8_t front = (HAL_GPIO_ReadPin(FIGHT_IR_FRONT_PORT, FIGHT_IR_FRONT_PIN) == FIGHT_IR_TRIGGERED);
    uint8_t back  = (HAL_GPIO_ReadPin(FIGHT_IR_BACK_PORT,  FIGHT_IR_BACK_PIN)  == FIGHT_IR_TRIGGERED);

    /*判断敌人方向*/
    if( (ne && nw) || front)return DIR_FRONT;
    if(nw) return DIR_FRONT_LEFT;
    if(ne) return DIR_FRONT_RIGHT;
    if(l)  return DIR_LEFT;
    if(r)  return DIR_RIGHT;
    if(sw) return DIR_BACK_LEFT;
    if(se) return DIR_BACK_RIGHT;
    if(back) return DIR_BACK;
    return DIR_NONE;
}

/**
 * @description: 灰度边缘检测(一票否决)
 * @param void
 * @return bool
 */
static bool Fight_EdgeDetected(void)
{
    site_detect_shade();
    return (voltage[0] >3.0f || voltage[1] > 3.0f); // 任一传感器检测到边缘
}

/*======状态机======*/

void Fight_Init(void)
{
    Fight_State = FIGHT_ENGAGE;
    Fight_DoneFlag = false;
    Fight_StartTime = HAL_GetTick();
    Fight_EngageLost = 0;
}

void Fight_Update(void)
{
    uint32_t now = HAL_GetTick();
    uint32_t elapsed = now - Fight_StartTime;
    EnemyDir dir = Fight_GetEnemyDir();

    /*======边缘安全======*/
    if(Fight_EdgeDetected())
    {
        MOTOR_StopAll();
        Fight_State = FIGHT_RETREAT;
        Fight_StartTime = now;
        return;
    }

    switch(Fight_State)
    {
        /*======交战状态======*/
        case FIGHT_ENGAGE:
            if(dir == DIR_NONE)
            {
                /*有目标，清除交战丢失时间*/
                Fight_EngageLost = 0;

                switch(dir)
                {
                    case DIR_FRONT:
                        drive_For_H();
                        break;
                    case DIR_FRONT_LEFT:
                        drive_ArcLeft_M();
                        break;
                    case DIR_FRONT_RIGHT:
                        drive_ArcRight_M();
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

            /*交战超时，后退*/
            if(elapsed >= FIGHT_ENGAGE_TIMEOUT)
            {
                Fight_State = FIGHT_RETREAT;
                Fight_StartTime = now;
            }
            break;

        /*======撤退状态======*/
        case FIGHT_RETREAT:
            drive_Back_M();
            if(elapsed >= FIGHT_RETREAT_TIME)
            {
                if(dir != DIR_NONE)
                {
                    Fight_State = FIGHT_ENGAGE;
                    Fight_StartTime = now;
                    Fight_EngageLost = 0;
                }
                else
                {
                    MOTOR_StopAll();
                    Fight_State = FIGHT_DONE;
                    Fight_DoneFlag = true;
                }
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