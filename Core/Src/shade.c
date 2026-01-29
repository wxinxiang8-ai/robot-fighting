/*
 * @Author: Xiang xin wang wxinxiang8@gmail.com
 * @Date: 2026-01-22 15:56:26
 * @LastEditors: Xiang xin wang wxinxiang8@gmail.com
 * @LastEditTime: 2026-01-29 15:29:12
 * @FilePath: \MDK-ARMd:\robot fighting\robot\Core\Src\shade.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "shade.h"
#include "adc.h"
#include "dma.h"

// MX_ADC2_Init();
// MX_DMA_Init(); // Do not call init functions at global scope. They are called in main.c

uint32_t shade[2];//adc value
float voltage[2];//voltage value

void Shade_Sensor_Init(void)
{
    HAL_ADC_Start_DMA(&hadc2,(uint32_t*)shade,2);//start adc2 dma
}

void site_detect_shade()
{
    for(int i=0;i<2;i++)
    {
        voltage[i]=(float)(shade[i]*3.3f)/4095.0f;//convert adc value to voltage value
    }
}