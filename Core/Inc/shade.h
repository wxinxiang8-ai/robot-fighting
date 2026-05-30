/*
 * @Author: Xiang xin wang wxinxiang8@gmail.com
 * @Date: 2026-01-22 14:05:37
 * @LastEditors: Xiang xin wang wxinxiang8@gmail.com
 * @LastEditTime: 2026-01-22 18:24:17
 * @FilePath: \MDK-ARMd:\robot fighting\robot\Core\Inc\shade.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef SHADE_H
#define SHADE_H

#include "main.h"
extern uint16_t shade_v0;//adc value
extern uint16_t shade_v1;//adc value
extern float voltage_v0;//voltage value
extern float voltage_v1;//voltage value

#define SHADE_OFFSTAGE_SCENE_MODE 0
#define SHADE_OFFSTAGE_SCENE_CONFIRM_TIME 300U
#define SHADE_FAULT_HIGH_VOLTAGE 3.25f
#define SHADE_FAULT_CONFIRM_TIME 1000U

#define SHADE_OFF_FLOOR_V0_MIN      2.80f
#define SHADE_OFF_FLOOR_V0_MAX      3.10f
#define SHADE_OFF_FLOOR_V1_MIN      2.80f
#define SHADE_OFF_FLOOR_V1_MAX      3.10f

#define SHADE_OFF_YELLOW_V0_MIN     0.00f
#define SHADE_OFF_YELLOW_V0_MAX     0.00f
#define SHADE_OFF_YELLOW_V1_MIN     0.00f
#define SHADE_OFF_YELLOW_V1_MAX     0.00f

#define SHADE_OFF_BLUE_V0_MIN       0.00f
#define SHADE_OFF_BLUE_V0_MAX       0.00f
#define SHADE_OFF_BLUE_V1_MIN       0.00f
#define SHADE_OFF_BLUE_V1_MAX       0.00f

typedef enum {
    SHADE_SCENE_ON_STAGE = 0,
    SHADE_SCENE_OFF_FLOOR,
    SHADE_SCENE_OFF_YELLOW,
    SHADE_SCENE_OFF_BLUE,
    SHADE_SCENE_UNKNOWN
} ShadeScene_t;

void Shade_Sensor_Init(void);
void site_detect_shade(void);
ShadeScene_t Shade_GetScene(void);
uint8_t Shade_IsOffStageScene(void);
uint8_t Shade_V0_IsFault(void);
uint8_t Shade_V1_IsFault(void);
uint8_t Shade_HasValidSensor(void);
void Shade_TestInject_Enable(uint16_t v0_adc, uint16_t v1_adc);
void Shade_TestInject_Disable(void);

#endif // SHADE_H