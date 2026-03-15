#ifndef ROBOT_FIGHT_H
#define ROBOT_FIGHT_H

#include "main.h"

/*======引脚配置======*/
#define FIGHT_IR_NW_PIN IR_3_Pin //左前角
#define FIGHT_IR_NW_PORT      IR_3_GPIO_Port
#define FIGHT_IR_NE_PIN IR_4_Pin //右前角
#define FIGHT_IR_NE_PORT      IR_4_GPIO_Port
#define FIGHT_IR_L_PIN IR_5_Pin //左侧角
#define FIGHT_IR_L_PORT       IR_5_GPIO_Port
#define FIGHT_IR_R_PIN IR_6_Pin //右侧角
#define FIGHT_IR_R_PORT       IR_6_GPIO_Port
#define FIGHT_IR_SW_PIN IR_7_Pin //左后角
#define FIGHT_IR_SW_PORT      IR_7_GPIO_Port
#define FIGHT_IR_SE_PIN IR_8_Pin //右后角
#define FIGHT_IR_SE_PORT      IR_8_GPIO_Port

/*======触发电平======*/
#define FIGHT_IR_TRIGGERED GPIO_PIN_SET


#endif // ROBOT_FIGHT_H