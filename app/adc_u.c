#include "stm32f4xx_hal.h"
#include "adc.h"

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "adc_u.h"

////////////////////////////////////////////////////////////////////////////////
//
// module privates
//
////////////////////////////////////////////////////////////////////////////////
static adc_callback_t     _complete_callback = NULL;
static adc_channels_num_t _current_chnl = 0;

////////////////////////////////////////////////////////////////////////////////
//
// private utilities
//
////////////////////////////////////////////////////////////////////////////////
static inline void
adc_select_channel(adc_channels_num_t chnl)
{
  ADC_ChannelConfTypeDef sConfig = {0};

  switch(chnl)
  {
  case 0:
    sConfig.Channel = ADC_CHANNEL_0;
    break;

  case 1:
    sConfig.Channel = ADC_CHANNEL_1;
    break;

  case 2:
    sConfig.Channel = ADC_CHANNEL_2;
    break;

  default:
    return;
  }

  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;

  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// ADC Complete ISR
//
////////////////////////////////////////////////////////////////////////////////
void
HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
  uint16_t result = 0;

  result = (uint16_t)HAL_ADC_GetValue(hadc);
  if(_complete_callback != NULL)
  {
    _complete_callback(_current_chnl, result);
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// public interfaces
//
////////////////////////////////////////////////////////////////////////////////
void
adc_init(void)
{
}

void
adc_start_conversion(adc_channels_num_t chnl, adc_callback_t callback)
{
  _complete_callback  = callback;
  _current_chnl       = chnl;

  adc_select_channel(chnl);

  // start conversion
  HAL_ADC_Start_IT(&hadc1);
}
