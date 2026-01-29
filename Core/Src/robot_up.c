#include "motor.h"
#include "robot_up.h"

static GoUpState GoUp_Stage = GOUP_RUSH;
static uint32_t GoUp_StartTime = 0;
static bool GoUp_Done = false;

/**
 * @description: 设置初始状态；获取初始事件；设置完成标志位为false
 * @param void
 * @return void
 */
void GoUp_Init(void)
{
    GoUp_Stage = GOUP_RUSH;
    GoUp_StartTime = HAL_GetTick();
    GoUp_Done = false;
}

/**
 * @description: 更新上台状态机，根据时间和状态执行相应动作
 * @param void
 * @return void
 */
void GoUp_Update()
{
    uint32_t current_time = HAL_GetTick();
    uint32_t elapsed_time = current_time - GoUp_StartTime;

    switch(GoUp_Stage)
    {
        case GOUP_RUSH:
            /*全速倒退冲台*/
            drive_Back_H();
            if(elapsed_time >= GOUP_RUSH_TIME)
            {
                GoUp_Stage = GOUP_TURN;
                GoUp_StartTime = current_time;
            }
            break;
        case GOUP_TURN:
            /*差速掉头*/
            drive_Retreat_L();
            if(elapsed_time >= GOUP_TURN_TIME)
            {
                GoUp_Stage = GOUP_DONE;
                GoUp_StartTime = current_time;
            }
            break;

        case GOUP_DONE:
            GoUp_Done = true;
            break;

    }

}

/**
 * @description: 返回上台任务是否完成
 * @param void
 * @return bool
 */
bool GoUp_IsDone(void)
{
    return GoUp_Done;
}