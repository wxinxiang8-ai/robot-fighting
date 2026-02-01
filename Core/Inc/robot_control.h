#ifndef __ROBOT_CONTROL_H
#define __ROBOT_CONTROL_H

typedef enum {
    ROBOT_IDLE,      // 等待启动
    ROBOT_GO_UP,     // 上台中
    ROBOT_ROAMING,   // 巡台寻敌
    ROBOT_ATTACK,    // 攻击
    ROBOT_ESCAPE,    // 边缘逃离
    ROBOT_FALLEN     // 掉台重上台
} RobotState;

void Robot_Control_Update(RobotState *state);

#endif
