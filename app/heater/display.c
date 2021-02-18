#include "display.h"
#include "ssd1306.h"
#include "event_dispatcher.h"
#include "event_list.h"
#include "mainloop_timer.h"

#define TEST_INTERVAL   500

static SoftTimerElem    _test_tmr;
static ssd1306_color_t  _color = ssd1306_color_white;

static void
screen_update_complete(uint32_t event)
{
  if(_color == ssd1306_color_white)
  {
    _color = ssd1306_color_black;
  }
  else
  {
    _color = ssd1306_color_white;
  }
  mainloop_timer_schedule(&_test_tmr, TEST_INTERVAL);
}

static void
test_callback(SoftTimerElem* te)
{
  ssd1306_fill(_color);
  ssd1306_update_screen_async(NULL);
}

void
display_init(void)
{
  ssd1306_init();
  event_register_handler(screen_update_complete, DISPATCH_EVENT_SCREEN_UPDATE_DONE);

  soft_timer_init_elem(&_test_tmr);
  _test_tmr.cb = test_callback;

  mainloop_timer_schedule(&_test_tmr, TEST_INTERVAL);
}
