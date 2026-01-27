#ifndef __PID_H__
#define __PID_H__

typedef struct{
    float Kp;//比例系数
    float Ki;//积分系数
    float Kd;//微分系数
    float setpoint;//目标值
    float integral;//积分累计
    float last_error;//上次误差
    float output_max;//最大输出值
    float output_min;//最小输出值
    float integral_max;//积分限幅
}PID_TypeDef;

// 初始化PID控制器                                                 
void PID_Init(PID_TypeDef *pid, float Kp, float Ki, float Kd,float output_min, float output_max);
// 计算PID输出（增量式）
float PID_Compute(PID_TypeDef *pid, float measured);

#endif /* __PID_H__ */