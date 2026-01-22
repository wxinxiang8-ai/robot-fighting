#include "shade.h"
#include "adc.h"
#include "dma.h"

// MX_ADC2_Init();
// MX_DMA_Init(); // Do not call init functions at global scope. They are called in main.c

uint32_t shade[4];//adc value
float voltage[4];//voltage value

void Shade_Sensor_Init(void)
{
    HAL_ADC_Start_DMA(&hadc2,(uint32_t*)shade,4);//start adc2 dma
}

void site_detect_shade()
{
    for(int i=0;i<4;i++)
    {
        voltage[i]=(float)(shade[i]*3.3f)/4095.0f;//convert adc value to voltage value
    }
}