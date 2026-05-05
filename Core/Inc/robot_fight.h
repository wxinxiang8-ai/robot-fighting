#ifndef ROBOT_FIGHT_H
#define ROBOT_FIGHT_H

#include "main.h"
#include <stdbool.h>

/*======时间参数(ms)======*/
#define FIGHT_ENGAGE_TIMEOUT 3000 //交战时间
#define FIGHT_ENGAGE_LOST 500 //交战丢失时间
#define FIGHT_EDGE_STOP_TIME 15 //边缘确认后停顿时间
#define FIGHT_RETREAT_TIME 400 //撤退时间
#define FIGHT_RETREAT_SPEED 700
#define FIGHT_TURN_TIME   550 //掉头时间(180°)
#define FIGHT_TURN_SPEED 500
#define FIGHT_FB_RETREAT_TIME 500 //F/B回避先后退时间
#define FIGHT_FB_RETREAT_SPEED 400
#define FIGHT_FB_FORWARD_TIME 900 //F/B回避后前进时间
#define FIGHT_FB_ADVANCE_SPEED 350
#define FIGHT_TRACK_FRONT_ARC_INNER_SPEED 40 //前侧方弧线内侧轮速度
#define FIGHT_TRACK_FRONT_ARC_OUTER_SPEED 550 //前侧方弧线外侧轮速度
#define FIGHT_TRACK_PUSH_SPEED 450 //正前推进速度
#define FIGHT_TRACK_SPIN_SPEED 500 //补角转向速度
#define FIGHT_VISION_CONFIRM_COUNT 2 //视觉类型消抖次数
#define FIGHT_SHADE_CONFIRM_COUNT 3 //V0/V1灰度掉台确认次数

/*======敌人（能量块）方向======*/
typedef enum{
    DIR_NONE = 0,
    DIR_FRONT, //正前方
    DIR_FRONT_LEFT, //左前方
    DIR_FRONT_RIGHT, //右前方
    DIR_LEFT, //正左
    DIR_RIGHT, //正右
    DIR_BACK_LEFT, //左后方
    DIR_BACK_RIGHT, //右后方
    DIR_BACK, //正后方
}EnemyDir;

/*======进攻状态======*/
typedef enum{
    FIGHT_ENGAGE, //交战
    FIGHT_EDGE_STOP, //边缘确认后短暂停顿
    FIGHT_RETREAT, //边缘后退脱离
    FIGHT_TURN,   //边缘恢复掉头180°
    FIGHT_FB_TURN, //F/B回避先后退
    FIGHT_FORWARD, //F/B回避后180°掉头
    FIGHT_FB_ADVANCE, //F/B回避后短前进
    FIGHT_TRACK_SPIN, //侧后向追踪原地补角
    FIGHT_DONE, //交还控制权回漫游
}FightState;

/* ============ 函数声明 ============ */
void     Fight_Init(void);
void     Fight_InitWithDir(EnemyDir dir);
void     Fight_Update(void);
bool     Fight_IsDone(void);
EnemyDir Fight_GetEnemyDir(void);       // 获取敌人方向
bool     Fight_IsDown(void);           // 检测是否掉台

#endif // ROBOT_FIGHT_H
