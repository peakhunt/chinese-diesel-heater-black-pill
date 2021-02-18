#include "display.h"
#include "ssd1306.h"
#include "event_dispatcher.h"
#include "event_list.h"
#include "mainloop_timer.h"
#include "gpio_app.h"

typedef enum
{
  display_mode_welcome,
  display_mode_status,
  display_mode_glow_plug,
  display_mode_oil_pump,
  display_mode_fan,
} display_mode_t;

typedef enum
{
  display_input_mode,
  display_input_select,
  display_input_up,
  display_input_down,
} display_input_t;

typedef void (*display_handler)(void);
typedef void (*input_handler)(display_input_t);

#define WELCOME_INTERVAL          2000
#define STATUS_UPDATE_INTERVAL    1000

static void welcome_display(void);
static void welcome_input(display_input_t input);
static void status_display(void);
static void status_input(display_input_t input);
static void glow_plug_display(void);
static void glow_plug_input(display_input_t input);
static void oil_pump_display(void);
static void oil_pump_input(display_input_t input);
static void fan_display(void);
static void fan_input(display_input_t input);

static void display_switch(display_mode_t mode);

static SoftTimerElem    _tmr;
static display_mode_t   _mode = display_mode_welcome;
static bool             _update_in_prog = false;

static display_handler  _display_handlers[] = 
{
  welcome_display,
  status_display,
  glow_plug_display,
  oil_pump_display,
  fan_display,
};

static input_handler _input_handlers[] =
{
  welcome_input,
  status_input,
  glow_plug_input,
  oil_pump_input,
  fan_input,
};


/////////////////////////////////////////////////////////////////////////////////////////
//
// welcome state
//
/////////////////////////////////////////////////////////////////////////////////////////
static void
welcome_display(void)
{
  ssd1306_set_cursor(38, 20);
  ssd1306_write_string("Chinese", Font_7x10, ssd1306_color_white);
  ssd1306_set_cursor(42, 30);
  ssd1306_write_string("Diesel", Font_7x10, ssd1306_color_white);
  ssd1306_set_cursor(42, 40);
  ssd1306_write_string("Heater", Font_7x10, ssd1306_color_white);
}

static void
welcome_input(display_input_t input)
{
  //
  // nothing to do
  //
}

/////////////////////////////////////////////////////////////////////////////////////////
//
// status state
//
/////////////////////////////////////////////////////////////////////////////////////////
static void
status_display(void)
{
  ssd1306_set_cursor(5, 5);
  ssd1306_write_string("Status", Font_7x10, ssd1306_color_white);
}

static void
status_input(display_input_t input)
{
  switch(input)
  {
  case display_input_mode:
    display_switch(display_mode_glow_plug);
    break;

  default:
    break;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
//
// glow plug state
//
/////////////////////////////////////////////////////////////////////////////////////////
static void
glow_plug_display(void)
{
  ssd1306_set_cursor(5, 5);
  ssd1306_write_string("Glow Plug", Font_7x10, ssd1306_color_white);
}

static void
glow_plug_input(display_input_t input)
{
  switch(input)
  {
  case display_input_mode:
    display_switch(display_mode_oil_pump);
    break;

  default:
    break;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
//
// oil pump state
//
/////////////////////////////////////////////////////////////////////////////////////////
static void
oil_pump_display(void)
{
  ssd1306_set_cursor(5, 5);
  ssd1306_write_string("Oil Pump", Font_7x10, ssd1306_color_white);
}

static void
oil_pump_input(display_input_t input)
{
  switch(input)
  {
  case display_input_mode:
    display_switch(display_mode_fan);
    break;

  default:
    break;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
//
// fan state
//
/////////////////////////////////////////////////////////////////////////////////////////
static void
fan_display(void)
{
  ssd1306_set_cursor(5, 5);
  ssd1306_write_string("Fan", Font_7x10, ssd1306_color_white);
}

static void
fan_input(display_input_t input)
{
  switch(input)
  {
  case display_input_mode:
    display_switch(display_mode_status);
    break;

  default:
    break;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
//
// utilities
//
/////////////////////////////////////////////////////////////////////////////////////////
static void
display_show(void)
{
  ssd1306_fill(ssd1306_color_black);

  _display_handlers[_mode]();

  ssd1306_update_screen_async();
  _update_in_prog = true;
}

static void
display_switch(display_mode_t mode)
{
  _mode = mode;
  display_show();
}

/////////////////////////////////////////////////////////////////////////////////////////
//
// event handlers
//
/////////////////////////////////////////////////////////////////////////////////////////
static void
screen_update_complete(uint32_t event)
{
  _update_in_prog = false;

  switch(_mode)
  {
  case display_mode_welcome:
    mainloop_timer_schedule(&_tmr, WELCOME_INTERVAL);
    break;

  case display_mode_status:
    mainloop_timer_schedule(&_tmr, STATUS_UPDATE_INTERVAL);
    break;

  case display_mode_glow_plug:
  case display_mode_oil_pump:
  case display_mode_fan:
    break;
  }
}
static void
timer_callback(SoftTimerElem* te)
{
  switch(_mode)
  {
  case display_mode_welcome:
    display_switch(display_mode_status);
    break;

  case display_mode_status:
  case display_mode_glow_plug:
  case display_mode_oil_pump:
  case display_mode_fan:
    display_show();
    break;
  }
}

static void
button_handler(gpio_in_pin_t pin, gpio_input_state_t state, void* arg)
{
  display_input_t input;

  if(_update_in_prog || state != gpio_input_state_low)
  {
    return;
  }

  switch(pin)
  {
  case gpio_in_pin_pb1:   // mode button
    input = display_input_mode;
    break;

  case gpio_in_pin_pb2:   // select button
    input = display_input_select;
    break;

  case gpio_in_pin_pb3:   // up button
    input = display_input_up;
    break;

  case gpio_in_pin_pb4:   // down button
    input = display_input_down;
    break;

  default:
    return;
  }

  _input_handlers[_mode](input);
}

/////////////////////////////////////////////////////////////////////////////////////////
//
// public
//
/////////////////////////////////////////////////////////////////////////////////////////
void
display_init(void)
{
  ssd1306_init();
  event_register_handler(screen_update_complete, DISPATCH_EVENT_SCREEN_UPDATE_DONE);

  soft_timer_init_elem(&_tmr);
  _tmr.cb = timer_callback;

  gpio_listen(gpio_in_pin_pb1, button_handler, NULL);
  gpio_listen(gpio_in_pin_pb2, button_handler, NULL);
  gpio_listen(gpio_in_pin_pb3, button_handler, NULL);
  gpio_listen(gpio_in_pin_pb4, button_handler, NULL);

  display_show();
}
