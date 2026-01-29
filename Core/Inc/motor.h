#ifndef __MOTOR_H__
#define __MOTOR_H__
#include <stdint.h>

// PWM配置
#define PWM_MAX_VALUE       4199        // PWM最大值(对应定时器ARR)
#define SPEED_MAX_INPUT     1000        // 速度输入最大值

// 预定义速度等级
#define SPEED_LOW           300         // 低速(30%)
#define SPEED_MEDIUM        600         // 中速(60%)
#define SPEED_HIGH          900         // 高速(90%)

// 转向速度
#define SPEED_TURN_M        400         // 中等转向速度(40%)
#define SPEED_TURN_S        800         // 快速转向速度(80%)

// 电机方向校正（左侧1,2和右侧3,4方向相反）                                              │
#define MOTOR_DIR_1     1    // 左前                                                     │
#define MOTOR_DIR_2     1    // 左后                                                     │
#define MOTOR_DIR_3    -1    // 右前（反向）                                             │
#define MOTOR_DIR_4    -1    // 右后（反向）

typedef enum{
    //left_f
    MOTOR_1=1,//PD12,PD13
    //left_b
    MOTOR_2=2,//PD12,PD13
    //right_f
    MOTOR_3=3,//PD14,PD15
    //right_b
    MOTOR_4=4 //PD14,PD15
}MOTOR_ID;

void MOTOR_Init(void);
void MOTOR_SetSpeed(MOTOR_ID motor_id, int16_t speed);

void drive_For_L(void);//前进(低中高)
void drive_For_M(void);
void drive_For_H(void);

void drive_Back_L(void);//后退(低中高)
void drive_Back_M(void);
void drive_Back_H(void);

void drive_Left_M(void);//微左右转
void drive_Right_M(void);

void drive_Left_S(void);//超级左右转
void drive_Right_S(void);

void MOTOR_StopAll(void);

#endif