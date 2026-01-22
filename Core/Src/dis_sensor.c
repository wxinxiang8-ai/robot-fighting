/*
 * @Author: Xiang xin wang wxinxiang8@gmail.com
 * @Date: 2026-01-22 15:56:26
 * @LastEditors: Xiang xin wang wxinxiang8@gmail.com
 * @LastEditTime: 2026-01-22 18:22:18
 * @FilePath: \MDK-ARMd:\robot fighting\robot\Core\Src\dis_sensor.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "dis_sensor.h"
#include "adc.h"
#include "math.h"

uint32_t dis_sensor_raw[4] = {0};
float dis_sensor_distance[4] = {0};

void Dis_Sensor_Init(void)
{
    // Start ADC1 with DMA, buffer size 4
    HAL_ADC_Start_DMA(&hadc1, dis_sensor_raw, 4);
}

void Dis_Sensor_Process(void)
{
    float voltage;
    for(int i = 0; i < 4; i++)
    {
        // Convert raw ADC value to voltage
        voltage = (float)dis_sensor_raw[i] * 3.3f / 4095.0f;

        // GP2Y0A02YK0F Distance Calculation
        // Approx Formula: Distance (cm) = 60.495 * V^(-1.1904)
        // Range: 20cm - 150cm (Voltage approx 2.5V - 0.4V)
        
        if (voltage > 2.8f) // Too close (< 20cm)
        {
            dis_sensor_distance[i] = 20.0f;
        }
        else if (voltage < 0.4f) // Too far (> 150cm)
        {
            dis_sensor_distance[i] = 150.0f;
        }
        else
        {
            dis_sensor_distance[i] = 60.495f * powf(voltage, -1.1904f);
        }
    }
}
