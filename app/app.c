#include "stm32f4xx_hal.h"
#include "app_common.h"

#include "sys_timer.h"
#include "event_dispatcher.h"
#include "mainloop_timer.h"
#include "shell.h"
#include "misc.h"
#include "gpio_app.h"
#include "pwm.h"
#include "adc_reader.h"

#include "heater.h"
#include "display.h"

void
app_init(void)
{
  __disable_irq();

  event_dispatcher_init();
  mainloop_timer_init();

  sys_timer_init();
  shell_init();
  misc_init();
  gpio_init();
  pwm_init();
  adc_reader_init();

  heater_init();

  __enable_irq();

  display_init();
}

void
app_run(void)
{
  while(1)
  {
    event_dispatcher_dispatch();
  }
}
