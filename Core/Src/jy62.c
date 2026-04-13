#include "jy62.h"
#include "i2c.h"
#include <string.h>

#define JY62_I2C_ADDR_7BIT           0x50u
#define JY62_I2C_ADDR_8BIT           (JY62_I2C_ADDR_7BIT << 1)
#define JY62_REG_AX                  0x34u
#define JY62_REG_GX                  0x37u
#define JY62_REG_ROLL                0x3Du
#define JY62_READ_BLOCK_WORDS        12u
#define JY62_READ_BLOCK_BYTES        (JY62_READ_BLOCK_WORDS * 2u)
#define JY62_UPDATE_PERIOD_MS        20u
#define JY62_OFFLINE_TIMEOUT_MS      100u
#define JY62_I2C_TIMEOUT_MS          10u
#define JY62_ACC_SCALE_G             (16.0f / 32768.0f)
#define JY62_GYRO_SCALE_DPS          (2000.0f / 32768.0f)
#define JY62_ANGLE_SCALE_DEG         (180.0f / 32768.0f)

volatile JY62_Data_t jy62_data = {0};

static uint32_t jy62_last_poll_ms = 0u;

static int16_t jy62_unpack_i16(const uint8_t *buf, uint8_t word_index)
{
    uint16_t offset = (uint16_t)word_index * 2u;
    return (int16_t)(((uint16_t)buf[offset + 1u] << 8) | buf[offset]);
}

void JY62_Init(void)
{
    memset((void *)&jy62_data, 0, sizeof(jy62_data));
    jy62_last_poll_ms = 0u;
}

void JY62_Update(void)
{
    uint32_t now = HAL_GetTick();
    uint8_t raw[JY62_READ_BLOCK_BYTES] = {0};

    if ((now - jy62_last_poll_ms) < JY62_UPDATE_PERIOD_MS)
    {
        if (jy62_data.last_update_ms != 0u &&
            (now - jy62_data.last_update_ms) > JY62_OFFLINE_TIMEOUT_MS)
        {
            jy62_data.online = 0u;
        }
        return;
    }

    jy62_last_poll_ms = now;

    if (HAL_I2C_Mem_Read(&hi2c2,
                         JY62_I2C_ADDR_8BIT,
                         JY62_REG_AX,
                         I2C_MEMADD_SIZE_8BIT,
                         raw,
                         JY62_READ_BLOCK_BYTES,
                         JY62_I2C_TIMEOUT_MS) != HAL_OK)
    {
        if (jy62_data.last_update_ms != 0u &&
            (now - jy62_data.last_update_ms) > JY62_OFFLINE_TIMEOUT_MS)
        {
            jy62_data.online = 0u;
        }
        return;
    }

    jy62_data.ax_raw = jy62_unpack_i16(raw, 0u);
    jy62_data.ay_raw = jy62_unpack_i16(raw, 1u);
    jy62_data.az_raw = jy62_unpack_i16(raw, 2u);
    jy62_data.gx_raw = jy62_unpack_i16(raw, 3u);
    jy62_data.gy_raw = jy62_unpack_i16(raw, 4u);
    jy62_data.gz_raw = jy62_unpack_i16(raw, 5u);
    jy62_data.roll_raw = jy62_unpack_i16(raw, 9u);
    jy62_data.pitch_raw = jy62_unpack_i16(raw, 10u);
    jy62_data.yaw_raw = jy62_unpack_i16(raw, 11u);

    jy62_data.ax_g = (float)jy62_data.ax_raw * JY62_ACC_SCALE_G;
    jy62_data.ay_g = (float)jy62_data.ay_raw * JY62_ACC_SCALE_G;
    jy62_data.az_g = (float)jy62_data.az_raw * JY62_ACC_SCALE_G;
    jy62_data.gx_dps = (float)jy62_data.gx_raw * JY62_GYRO_SCALE_DPS;
    jy62_data.gy_dps = (float)jy62_data.gy_raw * JY62_GYRO_SCALE_DPS;
    jy62_data.gz_dps = (float)jy62_data.gz_raw * JY62_GYRO_SCALE_DPS;
    jy62_data.roll_deg = (float)jy62_data.roll_raw * JY62_ANGLE_SCALE_DEG;
    jy62_data.pitch_deg = (float)jy62_data.pitch_raw * JY62_ANGLE_SCALE_DEG;
    jy62_data.yaw_deg = (float)jy62_data.yaw_raw * JY62_ANGLE_SCALE_DEG;
    jy62_data.last_update_ms = now;
    jy62_data.online = 1u;
}

uint8_t JY62_IsOnline(void)
{
    if (jy62_data.online == 0u)
    {
        return 0u;
    }

    return ((HAL_GetTick() - jy62_data.last_update_ms) <= JY62_OFFLINE_TIMEOUT_MS) ? 1u : 0u;
}

uint8_t JY62_IsStable(float max_roll_deg, float max_pitch_deg)
{
    if (!JY62_IsOnline())
    {
        return 0u;
    }

    if (jy62_data.roll_deg > max_roll_deg || jy62_data.roll_deg < -max_roll_deg)
    {
        return 0u;
    }

    if (jy62_data.pitch_deg > max_pitch_deg || jy62_data.pitch_deg < -max_pitch_deg)
    {
        return 0u;
    }

    return 1u;
}
