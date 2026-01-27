#include "pid.h"
#include "encoder.h"

/**
 * @description: PID控制器初始化
 * @param PID_TypeDef *pid, float Kp, float Ki, float Kd,float output_min, float output_max
 * @return void
 */
void PID_Init(PID_TypeDef *pid, float Kp, float Ki, float Kd,float output_min, float output_max)
{
    pid->Kp = Kp;
    pid->Ki = Ki;
    pid->Kd = Kd;
    pid->setpoint = 0.0f;
    pid->integral = 0.0f;
    pid->last_error = 0.0f;
    pid->output_max = output_max;
    pid->output_min = output_min;
    pid->integral_max = output_max; // 默认积分限幅与输出限幅相同
}

/**
 * @description: PID增量式输出
 * @param PID_TypeDef *pid, float measured
 * @return OutPut
 */
float PID_Compute(PID_TypeDef *pid, float measured)
{
    float error = pid->setpoint - measured;
    
    //积分累加
    pid->integral += error;
    if(pid->integral > pid->integral_max)
    {
        pid->integral = pid->integral_max;
    }
    else if(pid->integral < -pid->integral_max)
    {
        pid->integral = -pid->integral_max;
    }

    //微分
    float d =error - pid->last_error;
    pid->last_error = error;
    
    //PID输出
    float OutPut = pid->Kp * error
     + pid->Ki * pid->integral
     + pid->Kd * d;

    //输出限幅
    if(OutPut > pid->output_max)
    {
        OutPut = pid->output_max;
    }
    else if(OutPut < pid->output_min)
    {
        OutPut = pid->output_min;
    }
    return OutPut;
}