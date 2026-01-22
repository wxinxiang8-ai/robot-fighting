/*
 * @Author: Xiang xin wang wxinxiang8@gmail.com
 * @Date: 2026-01-22 15:56:22
 * @LastEditors: Xiang xin wang wxinxiang8@gmail.com
 * @LastEditTime: 2026-01-22 15:56:22
 * @FilePath: \MDK-ARMd:\robot fighting\robot\Core\Inc\dis_sensor.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef DIS_SENSOR_H
#define DIS_SENSOR_H

#include "main.h"

extern uint32_t dis_sensor_raw[4];
extern float dis_sensor_distance[4];

void Dis_Sensor_Init(void);
void Dis_Sensor_Process(void);

#endif // DIS_SENSOR_H
