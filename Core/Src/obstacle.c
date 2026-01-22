#include "obstacle.h"
#include "gpio.h"

// 定义全局变量实例
Obs_Sensors_t Obs_Data = {0};

/**
  * @brief  初始化函数 (目前仅需要在main中调用GPIO初始化即可，此处留空或做额外设置)
  */
void Obs_Sensor_Init(void)
{
    // GPIO已经在 MX_GPIO_Init 中初始化为输入模式
    // 此处可以添加其他的初始化代码，如果需要的话
}

/**
  * @brief  读取所有红外传感器状态
  *         建议在主循环或定时器中调用
  */
void Obs_Sensor_ReadAll(void)
{
    // 读取 GPIOE 上的 IR_1 到 IR_8 引脚状态
    // 注意：根据你的 main.h 定义，这些引脚都在 GPIOE 上
    
    Obs_Data.IR1 = HAL_GPIO_ReadPin(IR_1_GPIO_Port, IR_1_Pin);
    Obs_Data.IR2 = HAL_GPIO_ReadPin(IR_2_GPIO_Port, IR_2_Pin);
    Obs_Data.IR3 = HAL_GPIO_ReadPin(IR_3_GPIO_Port, IR_3_Pin);
    Obs_Data.IR4 = HAL_GPIO_ReadPin(IR_4_GPIO_Port, IR_4_Pin);
    Obs_Data.IR5 = HAL_GPIO_ReadPin(IR_5_GPIO_Port, IR_5_Pin);
    Obs_Data.IR6 = HAL_GPIO_ReadPin(IR_6_GPIO_Port, IR_6_Pin);
    Obs_Data.IR7 = HAL_GPIO_ReadPin(IR_7_GPIO_Port, IR_7_Pin);
    Obs_Data.IR8 = HAL_GPIO_ReadPin(IR_8_GPIO_Port, IR_8_Pin);
}
