/*
 * @Author: Xiang xin wang wxinxiang8@gmail.com
 * @Date: 2026-03-19 12:37:14
 * @LastEditors: Xiang xin wang wxinxiang8@gmail.com
 * @LastEditTime: 2026-03-24 22:05:43
 * @FilePath: \MDK-ARMd:\robot fighting\robot\Core\Inc\robot_up.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef __ROBOT_UP_H
#define __ROBOT_UP_H

#include <stdbool.h>
#include "main.h"

/*======非接触式启动======*/
/*引脚定义*/
#define STARTUP_LEFT_PIN IR_5_Pin
#define STARTUP_LEFT_PORT IR_5_GPIO_Port
#define STARTUP_RIGHT_PIN IR_7_Pin
#define STARTUP_RIGHT_PORT IR_7_GPIO_Port

/* 遮挡时传感器输出电平（当前逻辑：遮挡=1，未遮挡=0）*/
#define STARTUP_BLOCKED_STATE GPIO_PIN_SET
#define STARTUP_UNBLOCKED_STATE GPIO_PIN_RESET

/*消抖时间*/
#define STARTUP_DEBOUNCE_MS 80

/*队伍颜色定义*/
typedef enum{
    TEAM_NONE = 0,
    TEAM_YELLOW, //黄色方 左侧遮挡
    TEAM_BLUE //蓝色方 右侧遮挡
}TeamColor;

extern TeamColor Current_Team;

/*======上台状态======*/
typedef enum{
    GOUP_RUSH,//全速冲台
    GOUP_TURN,//差速掉头
    GOUP_DONE
}GoUpState;

/*======时间参数======*/
//#define GOUP_SPEED_TIME 500
#define GOUP_RUSH_TIME 1000
#define GOUP_TURN_TIME 500

/*======函数声明======*/
void GoUp_Init(void);
void GoUp_Update(void);
bool GoUp_IsDone(void);
TeamColor Startup_WaitForTrigger(void);
void Startup_Notify(TeamColor team);

#endif  