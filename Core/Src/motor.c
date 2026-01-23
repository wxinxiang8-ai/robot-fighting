#include "motor.h"
#include "tim.h"
#include "pid.h"
#include "encoder.h"

// PID参数 
#define DEFAULT_KP  2.0f                                        
#define DEFAULT_KI  0.5f                                
#define DEFAULT_KD  0.1f      

//全局变量
MotorCtrl_TypeDef MOTOR_CTRL[4] = {0};
PID_TypeDef MOTOR_PID[4] = {0};
static const int8_t MOTOR_DIR[4] = {MOTOR_DIR_1,MOTOR_DIR_2, MOTOR_DIR_3, MOTOR_DIR_4};

/**
 * @description: PID初始化
 * @param void
 * @return void
 */
void MOTOR_PID_Init(void)
{
    for(int i=0;i<=3;i++)
    {
        PID_Init(&MOTOR_PID[i], DEFAULT_KP, DEFAULT_KI, DEFAULT_KD,
             -SPEED_MAX_INPUT, SPEED_MAX_INPUT);
    }
}

/**
 * @description: 电机初始化
 * @param void
 * @return void
 */
void MOTOR_Init(void)
{
     // 启动TIM4 PWM - 电机1和电机2
      HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);  // PD12 - Motor1_A
      HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_2);  // PD13 - Motor1_B
      HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3);  // PD14 - Motor2_A
      HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4);  // PD15 - Motor2_B

      // 启动TIM2 PWM - 电机3
      HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);  // PA2 - Motor3_A
      HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4);  // PA3 - Motor3_B

      // 启动TIM9 PWM - 电机4
      HAL_TIM_PWM_Start(&htim9, TIM_CHANNEL_1);  // PE5 - Motor4_A
      HAL_TIM_PWM_Start(&htim9, TIM_CHANNEL_2);  // PE6 - Motor4_B

      // 初始停止所有电机
      MOTOR_StopAll();
}

/**
 * @description: 设置电机速度
 * @param 
 * @return 
 */
void MOTOR_SetSpeed(MOTOR_ID motor_id, int16_t speed)
{
   MOTOR_CTRL[motor_id - 1].target_speed = speed;
}

/**
 * @description: PID控制电机速度
 * @param 
 * @return 
 */
void MOTOR_PID_Control(void)
{
    for(int i=0;i<4;i++)
    {
        //设置目标速度
        MOTOR_PID[i].setpoint = MOTOR_CTRL[i].target_speed;
        
        //计算PID输出
        float pid_output = PID_Compute(&MOTOR_PID[i], MOTOR_CTRL[i].current_speed);

        // 方向校正
        int16_t corrected_speed = (int16_t)(pid_output) * MOTOR_DIR[i];

        // 设置PWM输出
        uint32_t pulse = (corrected_speed > 0) ? corrected_speed : -corrected_speed;
        pulse = (pulse * PWM_MAX_VALUE) / SPEED_MAX_INPUT;
        if (pulse > PWM_MAX_VALUE) pulse = PWM_MAX_VALUE;

        //设置PWM
        switch(i)
        {
            case 0:
                if(corrected_speed > 0)
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
            case 1:
                if(corrected_speed > 0)
                {
                    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, pulse); // Motor2_A
                    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_4, 0);     // Motor2_B
                }
                else
                {
                    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, 0);     // Motor2_A
                    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_4, pulse); // Motor2_B
                }
                break;
            case 2:
                if(corrected_speed > 0)
                {
                    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, pulse); // Motor3_A
                    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_4, 0);     // Motor3_B
                }
                else
                {
                    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, 0);     // Motor3_A
                    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_4, pulse); // Motor3_B
                }
                break;
            case 3:
                if(corrected_speed > 0)
                {
                    __HAL_TIM_SET_COMPARE(&htim9, TIM_CHANNEL_1, pulse); // Motor4_A
                    __HAL_TIM_SET_COMPARE(&htim9, TIM_CHANNEL_2, 0);     // Motor4_B
                }
                else
                {
                    __HAL_TIM_SET_COMPARE(&htim9, TIM_CHANNEL_1, 0);     // Motor4_A
                    __HAL_TIM_SET_COMPARE(&htim9, TIM_CHANNEL_2, pulse); // Motor4_B
                }
                break;               
        }
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

void drive_Left_M(void)//微左右转
{
   MOTOR_SetSpeed(MOTOR_1, -SPEED_TURN_M);
   MOTOR_SetSpeed(MOTOR_2, -SPEED_TURN_M);
   MOTOR_SetSpeed(MOTOR_3, SPEED_TURN_M);
   MOTOR_SetSpeed(MOTOR_4, SPEED_TURN_M);
}

void drive_Right_M(void)
{
    MOTOR_SetSpeed(MOTOR_1, SPEED_TURN_M);
    MOTOR_SetSpeed(MOTOR_2, SPEED_TURN_M);
    MOTOR_SetSpeed(MOTOR_3, -SPEED_TURN_M);
    MOTOR_SetSpeed(MOTOR_4, -SPEED_TURN_M);
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
