#include "accel_highg.h"

static void adc_stab_sleep();

void accel_highg_init()
{
    adc_set_dual_mode(ADC_CR1_DUALMOD_RSM);

    adc_enable_external_trigger_regular(ADC1, ADC_CR2_EXTSEL_TIM1_CC3);
    adc_enable_external_trigger_regular(ADC2, ADC_CR2_EXTSEL_TIM1_CC3);
    adc_enable_external_trigger_regular(ADC3, ADC_CR2_EXTSEL_TIM1_CC3);

    adc_enable_dma(ADC1);
    adc_enable_dma(ADC3);

    adc_on(ADC1);
    adc_on(ADC2);
    adc_on(ADC3);
    
    adc_stab_sleep();

    adc_reset_calibration(ADC1);
    adc_reset_calibration(ADC2);
    adc_reset_calibration(ADC3);
    adc_calibration(ADC1);
    adc_calibration(ADC2);
    adc_calibration(ADC3);


    /* Delay to allow stabilisation */
}

void accel_highg_go()
{
    
}

static void adc_stab_sleep()
{

}
