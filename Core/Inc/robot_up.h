#ifndef __ROBOT_UP_H
#define __ROBOT_UP_H

#include <stdbool.h>
#include "main.h"

/*======上台状态======*/
typedef enum{
    GOUP_RUSH,//全速冲台
    GOUP_TURN,//差速掉头
    GOUP_DONE
}GoUpState;

/*======时间参数======*/
//#define GOUP_SPEED_TIME 500
#define GOUP_RUSH_TIME 1000
#define GOUP_TURN_TIME 1000


void GoUp_Init(void);
void GoUp_Update(void);
bool GoUp_IsDone(void);


#endif  