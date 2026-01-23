#ifndef __ENCODER_H__
#define __ENCODER_H__
#include <stdint.h>

typedef enum{
    ENCODER_1=1,
    ENCODER_2=2,
    ENCODER_3=3,
    ENCODER_4=4
}ENCODER_ID;

typedef struct {
      int32_t  position;          // 当前位置(累计脉冲数)
      int32_t  last_count;        // 上次计数值
      int16_t  speed;             // 当前速度(脉冲数/采样周期)
      int8_t   direction;         // 方向: 1=正转, -1=反转, 0=静止
} Encoder_TypeDef;

extern Encoder_TypeDef ENCODER[4];

void ENCODER_Init(void);
int32_t ENCODER_GetCount(ENCODER_ID encoder_id);
void ENCODER_ResetCount(ENCODER_ID encoder_id);
void ENCODER_ResetAll(void);
void ENCODER_Update(void);

#endif