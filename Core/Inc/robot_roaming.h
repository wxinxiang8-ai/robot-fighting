#ifndef ROBOT_ROAMING_H
#define ROBOT_ROAMING_H

#include <stdbool.h>
#include "main.h"

/*======漫游状态======*/
typedef enum{
    ROAMING_FORWARD,      // 前进状态
    ROAMING_BACK,         // 后退状态
    ROAMING_TURN_LEFT,    // 左转状态
    ROAMING_TURN_RIGHT,   // 右转状态
    ROAMING_DONE          // 完成状态（掉落擂台）
}RoamingState;

/*======时间参数======*/
#define ROAMING_BACK_TIME  500   // 后退时间
#define ROAMING_TURN_TIME  500   // 转向时间
#define ROAMING_FORWARD_TIME 500 // 前进时间

void Roaming_Init(void);
void Roaming_Update(void);
bool Roaming_IsDone(void);

#endif // ROBOT_ROAMING_H