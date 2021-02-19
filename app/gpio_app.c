#include "stm32f4xx_hal.h"

#include "gpio_app.h"
#include "event_dispatcher.h"
#include "event_list.h"
#include "soft_timer.h"
#include "mainloop_timer.h"

#define PIN_CHANGE_DEBOUNCE_TIME          10    // 10ms
typedef struct
{
  gpio_in_pin_t           pin_name;
  gpio_input_state_t      state;
  SoftTimerElem           debounce_tmr;
  GPIO_TypeDef*           port;           
  uint16_t                pin;
  bool                    use_debounce;
  gpio_listener           listener;
  void*                   arg;
} gpio_input_t;

static gpio_input_t   _inputs[GPIO_MAX_INPUT] =
{
  {
    .pin_name = gpio_in_pin_pb0,
    .port = GPIOB,
    .pin = GPIO_PIN_0,
    .use_debounce = true,
  },
  {
    .pin_name = gpio_in_pin_pb1,
    .port = GPIOB,
    .pin = GPIO_PIN_1,
    .use_debounce = true,
  },
  {
    .pin_name = gpio_in_pin_pb3,
    .port = GPIOB,
    .pin = GPIO_PIN_3,
    .use_debounce = true,
  },
  {
    .pin_name = gpio_in_pin_pb4,
    .port = GPIOB,
    .pin = GPIO_PIN_4,
    .use_debounce = true,
  },
  {
    .pin_name = gpio_in_pin_pb5,
    .port = GPIOB,
    .pin = GPIO_PIN_5,
    .use_debounce = true,
  },
};

#define INVOKE_CALLBACK(i)                        \
  if(i->listener != NULL)                         \
  {                                               \
    i->listener(i->pin_name, i->state, i->arg);   \
  }

void
HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
#if 0
  if(GPIO_Pin == GPIO_PIN_0 ||
     GPIO_Pin == GPIO_PIN_1 ||
     GPIO_Pin == GPIO_PIN_2 ||
     GPIO_Pin == GPIO_PIN_3 ||
     GPIO_Pin == GPIO_PIN_5)
#endif
  {
    event_set(1 << DISPATCH_EVENT_PIN_CHANGE);
  }
}

static void
handle_pin_change(uint32_t event)
{
  gpio_input_t*   input;
  uint8_t         pv;

  for(uint8_t i = 0; i < GPIO_MAX_INPUT; i++)
  {
    input = &_inputs[i];

    pv = HAL_GPIO_ReadPin(input->port, input->pin) == GPIO_PIN_RESET ?  0 : 1;

    if(input->use_debounce)
    {
      switch(input->state)
      {
      case gpio_input_state_low:
        if(pv)
        {
          // goes high
          input->state = gpio_input_state_low_to_high;
          mainloop_timer_schedule(&input->debounce_tmr, PIN_CHANGE_DEBOUNCE_TIME);
        }
        break;

      case gpio_input_state_low_to_high:
        if(pv == 0)
        {
          // goes low again
          input->state = gpio_input_state_low;
          mainloop_timer_cancel(&input->debounce_tmr);
        }
        break;

      case gpio_input_state_high:
        if(pv == 0)
        {
          // goes low
          input->state = gpio_input_state_high_to_low;
          mainloop_timer_schedule(&input->debounce_tmr, PIN_CHANGE_DEBOUNCE_TIME);
        }
        break;

      case gpio_input_state_high_to_low:
        if(pv)
        {
          // goes high again
          input->state = gpio_input_state_high;
          mainloop_timer_cancel(&input->debounce_tmr);
        }
        break;
      }
    }
    else
    {
      switch(input->state)
      {
      case gpio_input_state_low:
        if(pv)
        {
          // goes high
          input->state = gpio_input_state_high;
          INVOKE_CALLBACK(input);
        }
        break;

      case gpio_input_state_high:
        if(pv == 0)
        {
          // goes low
          input->state = gpio_input_state_low;
          INVOKE_CALLBACK(input);
        }
        break;

      default:
        break;
      }
    }

  }
}

static void
debounce_tmr_callback(SoftTimerElem* te)
{
  gpio_input_t*   input = (gpio_input_t*)te->priv;

  if(input->state == gpio_input_state_low_to_high)
  {
    input->state = gpio_input_state_high;
    INVOKE_CALLBACK(input);
  }
  else if(input->state == gpio_input_state_high_to_low)
  {
    input->state = gpio_input_state_low;
    INVOKE_CALLBACK(input);
  }
}

static void
init_input_pins(void)
{
  for(uint8_t i = 0; i < GPIO_MAX_INPUT; i++)
  {
    _inputs[i].state = HAL_GPIO_ReadPin(_inputs[i].port, _inputs[i].pin) == GPIO_PIN_RESET ?
      gpio_input_state_low : gpio_input_state_high;

    soft_timer_init_elem(&_inputs[i].debounce_tmr);
    _inputs[i].debounce_tmr.cb = debounce_tmr_callback;
    _inputs[i].debounce_tmr.priv = &_inputs[i];
    _inputs[i].listener = NULL;
    _inputs[i].arg = NULL;
  }
}

void
gpio_init(void)
{
  event_register_handler(handle_pin_change, DISPATCH_EVENT_PIN_CHANGE);
  init_input_pins();
}

void
gpio_set(gpio_out_pin_t pin, bool v)
{
  switch(pin)
  {
  case gpio_out_pin_pc13:
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, v ? GPIO_PIN_SET : GPIO_PIN_RESET);
    break;

  case gpio_out_pin_pc14:
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_14, v ? GPIO_PIN_SET : GPIO_PIN_RESET);
    break;

  case gpio_out_pin_pc15:
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_15, v ? GPIO_PIN_SET : GPIO_PIN_RESET);
    break;

  default:
    return;
  }
}

bool
gpio_get(gpio_in_pin_t pin)
{
  switch(_inputs[pin].state)
  {
  case gpio_input_state_low:
  case gpio_input_state_low_to_high:
    return false;
  default:
    break;
  }
  return true;
}

void
gpio_set_debounce(gpio_in_pin_t pin, bool debounce)
{
  gpio_input_t*   input;

  input = &_inputs[pin];

  input->use_debounce = debounce;
  mainloop_timer_cancel(&input->debounce_tmr);

  if(input->state == gpio_input_state_low_to_high)
  {
    input->state = gpio_input_state_high;
  }
  else if(input->state == gpio_input_state_high_to_low)
  {
    input->state = gpio_input_state_low;
  }
}

gpio_input_state_t
gpio_get_state(gpio_in_pin_t pin)
{
  return _inputs[pin].state;
}

void
gpio_listen(gpio_in_pin_t pin, gpio_listener listener, void* arg)
{
  _inputs[pin].listener = listener;
  _inputs[pin].arg      = arg;
}
