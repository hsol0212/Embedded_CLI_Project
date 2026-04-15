#include "temp.h"
#include "adc.h"
#include "stm32f4xx_hal_adc.h"
#include <stdint.h>

bool tempInit(){
    return true;
}

float tempRead(){
    uint32_t adc_val=0;
    float temp_celsius=0.0f;

    HAL_ADC_Start(&hadc1);

    if(HAL_ADC_PollForConversion(&hadc1, 10)==HAL_OK){
        adc_val=HAL_ADC_GetValue(&hadc1);

        float vsense = (adc_val/4095.0f)*3.3f;
        temp_celsius=((vsense-0.76f)/0.0025f)+25.0f;
    

    }
    HAL_ADC_Stop(&hadc1);
    //HAL_ADC_Start(&hadc1);

    return temp_celsius;

}