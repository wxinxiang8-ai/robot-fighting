/*
 * @Author: Xiang xin wang wxinxiang8@gmail.com
 * @Date: 2026-03-18 16:12:12
 * @LastEditors: Xiang xin wang wxinxiang8@gmail.com
 * @LastEditTime: 2026-03-26 18:07:05
 * @FilePath: \MDK-ARMd:\robot fighting\robot\Core\Src\motor.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "motor.h"
#include "tim.h"

void MOTOR_Init(void)
{
     // 启动TIM4 PWM - 电机1和电机2
      HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);  // PD12 - Motor1_A
      HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_2);  // PD13 - Motor1_B
      HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3);  // PD14 - Motor2_A
      HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4);  // PD15 - Motor2_B

      // 初始停止所有电机
      MOTOR_StopAll();
}

void MOTOR_SetSpeed(MOTOR_ID motor_id, int16_t speed)
{
    uint32_t pulse = (speed > 0) ? speed : -speed; // 计算PWM脉冲宽度
    pulse =  (pulse * PWM_MAX_VALUE) / SPEED_MAX_INPUT; // 速度映射
    if (pulse > PWM_MAX_VALUE) pulse = PWM_MAX_VALUE; // 限制最大值

    switch(motor_id)
    {
        case MOTOR_1:
        case MOTOR_2:
            if (speed >= 0)
            {
                __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, pulse); // Motor1_A
                __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_2, 0);     // Motor1_B
            }
            else
            {
                __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, 0);     // Motor1_A
                __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_2, pulse); // Motor1_B
            }
            break;

        case MOTOR_3:
        case MOTOR_4:
            if (speed >= 0)
            {
                __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, pulse); // Motor4_A
                __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_4, 0);     // Motor4_B
            }
            else
            {
                __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, 0);     // Motor4_A
                __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_4, pulse); // Motor4_B
            }
            break;
        }
    }

void MOTOR_StopAll(void)
{
    MOTOR_SetSpeed(MOTOR_1, 0);
    MOTOR_SetSpeed(MOTOR_2, 0);
    MOTOR_SetSpeed(MOTOR_3, 0);
    MOTOR_SetSpeed(MOTOR_4, 0);
}

void drive_For_L(void)//前进(低中高)
{
    MOTOR_SetSpeed(MOTOR_1, SPEED_LOW);
    MOTOR_SetSpeed(MOTOR_2, SPEED_LOW);
    MOTOR_SetSpeed(MOTOR_3, SPEED_LOW);
    MOTOR_SetSpeed(MOTOR_4, SPEED_LOW);
}

void drive_For_M(void)
{
    MOTOR_SetSpeed(MOTOR_1, SPEED_MEDIUM);
    MOTOR_SetSpeed(MOTOR_2, SPEED_MEDIUM);
    MOTOR_SetSpeed(MOTOR_3, SPEED_MEDIUM);
    MOTOR_SetSpeed(MOTOR_4, SPEED_MEDIUM);
}

void drive_For_H(void)
{
    MOTOR_SetSpeed(MOTOR_1, SPEED_HIGH);
    MOTOR_SetSpeed(MOTOR_2, SPEED_HIGH);
    MOTOR_SetSpeed(MOTOR_3, SPEED_HIGH);
    MOTOR_SetSpeed(MOTOR_4, SPEED_HIGH);
}

void drive_Back_L(void)//后退(低中高)
{
    MOTOR_SetSpeed(MOTOR_1, -SPEED_LOW);
    MOTOR_SetSpeed(MOTOR_2, -SPEED_LOW);
    MOTOR_SetSpeed(MOTOR_3, -SPEED_LOW);
    MOTOR_SetSpeed(MOTOR_4, -SPEED_LOW);
}

void drive_Back_M(void)
{
    MOTOR_SetSpeed(MOTOR_1, -SPEED_MEDIUM);
    MOTOR_SetSpeed(MOTOR_2, -SPEED_MEDIUM);
    MOTOR_SetSpeed(MOTOR_3, -SPEED_MEDIUM);
    MOTOR_SetSpeed(MOTOR_4, -SPEED_MEDIUM);
}

void drive_Back_H(void)
{
    MOTOR_SetSpeed(MOTOR_1, -SPEED_HIGH);
    MOTOR_SetSpeed(MOTOR_2, -SPEED_HIGH);
    MOTOR_SetSpeed(MOTOR_3, -SPEED_HIGH);
    MOTOR_SetSpeed(MOTOR_4, -SPEED_HIGH);
}

void drive_Left_L(void)//慢左右转
{
   MOTOR_SetSpeed(MOTOR_1, -SPEED_TURN_L);
   MOTOR_SetSpeed(MOTOR_2, -SPEED_TURN_L);
   MOTOR_SetSpeed(MOTOR_3, SPEED_TURN_L);
   MOTOR_SetSpeed(MOTOR_4, SPEED_TURN_L);
}

void drive_Right_L(void)
{
    MOTOR_SetSpeed(MOTOR_1, SPEED_TURN_L);
    MOTOR_SetSpeed(MOTOR_2, SPEED_TURN_L);
    MOTOR_SetSpeed(MOTOR_3, -SPEED_TURN_L);
    MOTOR_SetSpeed(MOTOR_4, -SPEED_TURN_L);
}

void drive_Left_M(void)//微左右转
{
   MOTOR_SetSpeed(MOTOR_1, -SPEED_TURN_R);
   MOTOR_SetSpeed(MOTOR_2, -SPEED_TURN_R);
   MOTOR_SetSpeed(MOTOR_3, SPEED_TURN_R);
   MOTOR_SetSpeed(MOTOR_4, SPEED_TURN_R);
}

void drive_Right_M(void)
{
    MOTOR_SetSpeed(MOTOR_1, SPEED_TURN_R);
    MOTOR_SetSpeed(MOTOR_2, SPEED_TURN_R);
    MOTOR_SetSpeed(MOTOR_3, -SPEED_TURN_R);
    MOTOR_SetSpeed(MOTOR_4, -SPEED_TURN_R);
}

void drive_Left_S(void)//超级左右转
{
    MOTOR_SetSpeed(MOTOR_1, -SPEED_TURN_S);
    MOTOR_SetSpeed(MOTOR_2, -SPEED_TURN_S);
    MOTOR_SetSpeed(MOTOR_3, SPEED_TURN_S);
    MOTOR_SetSpeed(MOTOR_4, SPEED_TURN_S);
}

void drive_Right_S(void)
{
    MOTOR_SetSpeed(MOTOR_1, SPEED_TURN_S);
    MOTOR_SetSpeed(MOTOR_2, SPEED_TURN_S);
    MOTOR_SetSpeed(MOTOR_3, -SPEED_TURN_S);
    MOTOR_SetSpeed(MOTOR_4, -SPEED_TURN_S);
}

void drive_Left_Roaming(void)//漫游左右转
{
    MOTOR_SetSpeed(MOTOR_1, -SPEED_TURN_R);
    MOTOR_SetSpeed(MOTOR_2, -SPEED_TURN_R);
    MOTOR_SetSpeed(MOTOR_3, SPEED_TURN_R);
    MOTOR_SetSpeed(MOTOR_4, SPEED_TURN_R);
}

void drive_Right_Roaming(void)
{
    MOTOR_SetSpeed(MOTOR_1, SPEED_TURN_R);
    MOTOR_SetSpeed(MOTOR_2, SPEED_TURN_R);
    MOTOR_SetSpeed(MOTOR_3, -SPEED_TURN_R);
    MOTOR_SetSpeed(MOTOR_4, -SPEED_TURN_R);
}

void drive_Retreat_L(void)//后退左转
{
    MOTOR_SetSpeed(MOTOR_1, -SPEED_TURN_S);
    MOTOR_SetSpeed(MOTOR_2, -SPEED_TURN_S);
    MOTOR_SetSpeed(MOTOR_3, -SPEED_TURN_S+200);
    MOTOR_SetSpeed(MOTOR_4, -SPEED_TURN_S+200);   
}

void drive_Retreat_R(void)//后退右转
{
    MOTOR_SetSpeed(MOTOR_1, -SPEED_TURN_S+200);
    MOTOR_SetSpeed(MOTOR_2, -SPEED_TURN_S+200);
    MOTOR_SetSpeed(MOTOR_3, -SPEED_TURN_S);
    MOTOR_SetSpeed(MOTOR_4, -SPEED_TURN_S);   
}

void drive_user_defined(int16_t left_speed, int16_t right_speed)//自定义速度
{
    MOTOR_SetSpeed(MOTOR_1, left_speed);
    MOTOR_SetSpeed(MOTOR_2, left_speed);
    MOTOR_SetSpeed(MOTOR_3, right_speed);
    MOTOR_SetSpeed(MOTOR_4, right_speed);
}
