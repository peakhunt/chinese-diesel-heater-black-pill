#include "stm32f4xx_hal.h"
#include "tim.h"

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "app_common.h"
#include "pwm.h"

//
// PWM clock is running at 16 Mhz using no clock divider out of 16 MHz system clock.
//
// 15.625 KHz is the target PWM frequency and we wanna be able to control the duty cycle from 0 to 100%
//

////////////////////////////////////////////////////////////////////////////////
//
// private definitions
//
////////////////////////////////////////////////////////////////////////////////
#define PWM_TICKS_FOR_FREQUENCY   (5376 - 1)            // 84 Mhz / 15.625 Khz - 1
#define PWM_PCT_TO_TICKS(pct)     ((uint16_t)((float)pct * PWM_TICKS_FOR_FREQUENCY / 100.0f))


static bool
_chnl_running[PWM_MAX_CHANNEL] = 
{
  false,
  false,
  false,
};


////////////////////////////////////////////////////////////////////////////////
//
// public interfaces 
//
////////////////////////////////////////////////////////////////////////////////
void
pwm_init(void)
{
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, 0);

  HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
  HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_2);
  HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_3);

  HAL_TIM_Base_Start(&htim1);
}

void
pwm_control(pwm_channel_t chnl, uint8_t pct)
{
  uint16_t    ticks;
  uint32_t    tchnl;

  ticks = PWM_PCT_TO_TICKS(pct);

  switch(chnl)
  {
  case pwm_channel_0:
    tchnl = TIM_CHANNEL_1;
    break;

  case pwm_channel_1:
    tchnl = TIM_CHANNEL_2;
    break;

  case pwm_channel_2:
    tchnl = TIM_CHANNEL_3;
    break;

  default:
    return;
  }

  if(ticks == 0)
  {
    _chnl_running[chnl] = false;
    HAL_TIM_PWM_Stop(&htim1, tchnl);
  }
  else
  {
    __HAL_TIM_SET_COMPARE(&htim1, tchnl, ticks);
    if(_chnl_running[chnl] == false)
    {
      HAL_TIM_PWM_Start(&htim1, tchnl);
    }
    _chnl_running[chnl] = true;
  }
}
