#ifndef __OLED_FONT_H
#define __OLED_FONT_H

#include <stdint.h>

/*OLED字模库,宽8像素,高16像素*/
extern const uint8_t OLED_F8x16[][16];

/*24×24中文字模库 - 用于任务码显示*/
/*每个汉字72字节(24×24点阵 = 576位 ÷ 8 = 72字节)*/
typedef struct {
    uint16_t code;          /**< 汉字GB2312编码(双字节) */
    const uint8_t data[72]; /**< 24×24点阵数据 */
} ChineseFont24x24_t;

/*16×16中文字模库 - 用于常规文字显示*/
/*每个汉字32字节(16×16点阵 = 256位 ÷ 8 = 32字节)*/
typedef struct {
    uint16_t code;          /**< 汉字GB2312编码(双字节) */
    const uint8_t data[32]; /**< 16×16点阵数据 */
} ChineseFont16x16_t;

#define OLED_GB2312_TONG   0xCDAC  /**< "同" GB2312编码 */
#define OLED_GB2312_YI     0xD2EC  /**< "异" GB2312编码 */
#define OLED_GB2312_SE     0xC9AB  /**< "色" GB2312编码 */

extern const ChineseFont24x24_t OLED_Chinese24x24[];
extern const uint8_t OLED_Chinese24x24_Count;

extern const ChineseFont16x16_t OLED_Chinese16x16[];
extern const uint8_t OLED_Chinese16x16_Count;

#endif /* __OLED_FONT_H */
