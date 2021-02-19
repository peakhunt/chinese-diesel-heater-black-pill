#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "stm32f4xx_hal.h"

#include "app_common.h"
#include "shell.h"
#include "shell_if_usb.h"
#include "gpio_app.h"
#include "pwm.h"
#include "adc_reader.h"

#include "heater.h"
#include "settings.h"
#include "utilities.h"
#include "version.h"

////////////////////////////////////////////////////////////////////////////////
//
// private definitions
//
////////////////////////////////////////////////////////////////////////////////

#define SHELL_MAX_COLUMNS_PER_LINE      128
#define SHELL_COMMAND_MAX_ARGS          4

typedef void (*shell_command_handler)(ShellIntf* intf, int argc, const char** argv);

typedef struct
{
  const char*           command;
  const char*           description;
  shell_command_handler handler;
} ShellCommand;

////////////////////////////////////////////////////////////////////////////////
//
// private prototypes
//
////////////////////////////////////////////////////////////////////////////////
static void shell_command_help(ShellIntf* intf, int argc, const char** argv);
static void shell_command_version(ShellIntf* intf, int argc, const char** argv);
static void shell_command_uptime(ShellIntf* intf, int argc, const char** argv);

static void shell_command_gpio_out(ShellIntf* intf, int argc, const char** argv);
static void shell_command_gpio_in(ShellIntf* intf, int argc, const char** argv);
static void shell_command_pwm(ShellIntf* intf, int argc, const char** argv);
static void shell_command_adc(ShellIntf* intf, int argc, const char** argv);

static void shell_command_start(ShellIntf* intf, int argc, const char** argv);
static void shell_command_stop(ShellIntf* intf, int argc, const char** argv);
static void shell_command_status(ShellIntf* intf, int argc, const char** argv);

static void shell_command_glow(ShellIntf* intf, int argc, const char** argv);
static void shell_command_oil(ShellIntf* intf, int argc, const char** argv);
static void shell_command_fan(ShellIntf* intf, int argc, const char** argv);
static void shell_command_set_oil_pump_freq(ShellIntf* intf, int argc, const char** argv);
static void shell_command_set_fan_power(ShellIntf* intf, int argc, const char** argv);
static void shell_command_settings(ShellIntf* intf, int argc, const char** argv);
static void shell_command_mod(ShellIntf* intf, int argc, const char** argv);
static void shell_command_save(ShellIntf* intf, int argc, const char** argv);
static void shell_command_reset(ShellIntf* intf, int argc, const char** argv);
static void shell_command_step(ShellIntf* intf, int argc, const char** argv);
static void shell_command_set(ShellIntf* intf, int argc, const char** argv);

////////////////////////////////////////////////////////////////////////////////
//
// private variables
//
////////////////////////////////////////////////////////////////////////////////
const uint8_t                 _welcome[] = "\r\n**** Welcome ****\r\n";
const uint8_t                 _prompt[]  = "\r\nHeater> ";

static char                   _print_buffer[SHELL_MAX_COLUMNS_PER_LINE + 1];

static LIST_HEAD(_shell_intf_list);

static const ShellCommand     _commands[] = 
{
  {
    "help",
    "show this command",
    shell_command_help,
  },
  {
    "version",
    "show version",
    shell_command_version,
  },
  {
    "uptime",
    "show system uptime",
    shell_command_uptime,
  },
  {
    "gpio_out",
    "control gpio out",
    shell_command_gpio_out,
  },
  {
    "gpio_in",
    "show gpio in",
    shell_command_gpio_in,
  },
  {
    "pwm",
    "control pwm",
    shell_command_pwm,
  },
  {
    "adc",
    "show adc values",
    shell_command_adc,
  },
  {
    "start",
    "start heater",
    shell_command_start,
  },
  {
    "stop",
    "stop heater",
    shell_command_stop,
  },
  {
    "status",
    "show heater status",
    shell_command_status,
  },
  {
    "glow",
    "control glow plug",
    shell_command_glow,
  },
  {
    "oil",
    "control oil pump",
    shell_command_oil,
  },
  {
    "fan",
    "control fan",
    shell_command_fan,
  },
  {
    "pump_freq",
    "change oil pump frequency",
    shell_command_set_oil_pump_freq,
  },
  {
    "fan_power",
    "change fan power",
    shell_command_set_fan_power,
  },
  {
    "settings",
    "show settings",
    shell_command_settings,
  },
  {
    "mod",
    "modify settings",
    shell_command_mod,
  },
  {
    "save",
    "save settings",
    shell_command_save,
  },
  {
    "reset",
    "reset settings",
    shell_command_reset,
  },
  {
    "step",
    "modify step parameters",
    shell_command_step,
  },
  {
    "set",
    "change control step",
    shell_command_set,
  },

};


static const char* 
heater_state_desc[] = 
{
  "off",
  "glowing for start",
  "priming",
  "running",
  "glowing for stop",
  "cooling",
};

static const char*
on_off_str[] =
{
  "off",
  "on",
};
////////////////////////////////////////////////////////////////////////////////
//
// shell utilities
//
////////////////////////////////////////////////////////////////////////////////
static inline void
shell_prompt(ShellIntf* intf)
{
  shell_printf(intf, "%s", _prompt);
}

////////////////////////////////////////////////////////////////////////////////
//
// shell command handlers
//
////////////////////////////////////////////////////////////////////////////////
static void
shell_command_help(ShellIntf* intf, int argc, const char** argv)
{
  size_t i;

  shell_printf(intf, "\r\n");

  for(i = 0; i < sizeof(_commands)/sizeof(ShellCommand); i++)
  {
    shell_printf(intf, "%-20s: ", _commands[i].command);
    shell_printf(intf, "%s\r\n", _commands[i].description);
  }
}

static void
shell_command_version(ShellIntf* intf, int argc, const char** argv)
{
  shell_printf(intf, "\r\n");
  shell_printf(intf, "%s\r\n", VERSION);
}

static void
shell_command_uptime(ShellIntf* intf, int argc, const char** argv)
{
  shell_printf(intf, "\r\n");
  shell_printf(intf, "System Uptime: %lu\r\n", __uptime);
}

static void
shell_command_gpio_out(ShellIntf* intf, int argc, const char** argv)
{
  uint8_t pin, v;

  shell_printf(intf, "\r\n");

  if(argc != 3)
  {
    shell_printf(intf, "invalid %s <pin> <value>\r\n", argv[0]);
    return;
  }

  pin = atoi(argv[1]);
  v = atoi(argv[2]);
  v = v != 0 ? 1 : 0;

  if(pin >= GPIO_MAX_OUTPUT)
  {
    shell_printf(intf, "Invalid pin %d\r\n", pin);
    return;
  }

  gpio_set((gpio_out_pin_t)pin, v);

  shell_printf(intf, "set gpio out %d to %d\r\n", pin, v);
}

static void
shell_command_gpio_in(ShellIntf* intf, int argc, const char** argv)
{
  static const char* pin_states[] =
  {
    "low",
    "low to high",
    "high",
    "high to low",
  };

  shell_printf(intf, "\r\n");

  for(uint8_t i = 0; i < GPIO_MAX_INPUT; i++)
  {
    shell_printf(intf, "gpio in %d - %d, %s\r\n",
      i,
      gpio_get((gpio_in_pin_t)i),
      pin_states[gpio_get_state((gpio_in_pin_t)i)]
    );
  }
}

static void
shell_command_pwm(ShellIntf* intf, int argc, const char** argv)
{
	pwm_channel_t chnl;
	uint8_t       pct;

	shell_printf(intf, "\r\n");

	if(argc != 3)
	{
		shell_printf(intf, "invalid %s <chnl> <percent>\r\n", argv[0]);
		return;
	}

	chnl = (pwm_channel_t)atoi(argv[1]);
	pct = atoi(argv[2]);

	if(chnl < 0 || chnl >= PWM_MAX_CHANNEL)
	{
		shell_printf(intf, "Invalid channel %d\r\n", chnl);
		return;
	}

	if(pct < 0 || pct > 100)
	{
		shell_printf(intf, "Invalid percent %d\r\n", pct);
		return;
	}

	pwm_control(chnl, pct);

	shell_printf(intf, "set pwm channel %d to %d percent\r\n", chnl, pct);
}

static void
shell_command_adc(ShellIntf* intf, int argc, const char** argv)
{
  shell_printf(intf, "\r\n");

  for(uint8_t i = 0; i < ADC_MAX_CHANNELS; i++)
  {
    shell_printf(intf, "ADC CH %d - %d\r\n", i, adc_get(i));
  }
}

static void
shell_command_start(ShellIntf* intf, int argc, const char** argv)
{
  heater_start();
}

static void
shell_command_stop(ShellIntf* intf, int argc, const char** argv)
{
  heater_stop();
}

static void
shell_command_status(ShellIntf* intf, int argc, const char** argv)
{
  const heater_t* heater = heater_get();
  static const char* motor_state_str[] = 
  {
    "not rotating",
    "starting",
    "rotating",
  };

  shell_printf(intf, "\r\n");

  shell_printf(intf, "state : %s\r\n", heater_state_desc[heater->state]);
  shell_printf(intf, "pump  : %s, freq %.1f Hz\r\n",
      on_off_str[heater->oil_pump.on],
      heater->oil_pump.freq);
  shell_printf(intf, "glow  : %s\r\n", on_off_str[heater->glow_plug.on]);
  shell_printf(intf, "fan   : %s, %s, power %d%%\r\n",
      on_off_str[heater->fan.on],
      motor_state_str[heater->fan.motor_state],
      heater->fan.pwr);
  shell_printf(intf, "step  : %d\r\n", heater->step);

  {
    int i_part, d_part;

    i_part = (int)heater->outlet_temp.temp;
    d_part = abs((heater->outlet_temp.temp - i_part) * 10);

    shell_printf(intf, "outlet : %d.%d C\r\n",
        i_part, d_part);
  }
}

static void
shell_command_glow(ShellIntf* intf, int argc, const char** argv)
{
  shell_printf(intf, "\r\n");

  if(argc != 2)
  {
    shell_printf(intf, "invalid %s on|off\r\n", argv[0]);
    return;
  }

  if(strcmp(argv[1], "on") == 0)
  {
    heater_glow_plug_on();
  }
  else if(strcmp(argv[1], "off") == 0)
  {
    heater_glow_plug_off();
  }
  else
  {
    shell_printf(intf, "invalid %s on|off\r\n", argv[0]);
    return;
  }
  shell_printf(intf, "turned %s glow plug\r\n", argv[1]);
}

static void
shell_command_oil(ShellIntf* intf, int argc, const char** argv)
{
  shell_printf(intf, "\r\n");

  if(argc != 2)
  {
    shell_printf(intf, "invalid %s on|off\r\n", argv[0]);
    return;
  }

  if(strcmp(argv[1], "on") == 0)
  {
    heater_oil_pump_on();
  }
  else if(strcmp(argv[1], "off") == 0)
  {
    heater_oil_pump_off();
  }
  else
  {
    shell_printf(intf, "invalid %s on|off\r\n", argv[0]);
    return;
  }
  shell_printf(intf, "turned %s oil pump\r\n", argv[1]);
}

static void
shell_command_fan(ShellIntf* intf, int argc, const char** argv)
{
  shell_printf(intf, "\r\n");

  if(argc != 2)
  {
    shell_printf(intf, "invalid %s on|off\r\n", argv[0]);
    return;
  }

  if(strcmp(argv[1], "on") == 0)
  {
    heater_fan_on();
  }
  else if(strcmp(argv[1], "off") == 0)
  {
    heater_fan_off();
  }
  else
  {
    shell_printf(intf, "invalid %s on|off\r\n", argv[0]);
    return;
  }
  shell_printf(intf, "turned %s fan\r\n", argv[1]);
}

static void
shell_command_set_oil_pump_freq(ShellIntf* intf, int argc, const char** argv)
{
  float freq;

  shell_printf(intf, "\r\n");

  if(argc != 2)
  {
    shell_printf(intf, "invalid %s <freq>\r\n", argv[0]);
    return;
  }

  freq = atof(argv[1]);

  if(fcompare(freq, OIL_PUMP_MIN_FREQ) < 0 || fcompare(freq, OIL_PUMP_MAX_FREQ) > 0)
  {
    shell_printf(intf, "Invalid frequency\r\n");
    return;
  }

  heater_oil_pump_freq(freq);
}

static void
shell_command_set_fan_power(ShellIntf* intf, int argc, const char** argv)
{
  uint8_t pwr;

  shell_printf(intf, "\r\n");

  if(argc != 2)
  {
    shell_printf(intf, "invalid %s <pwr>\r\n", argv[0]);
    return;
  }

  pwr = atoi(argv[1]);

  if(pwr < 0 || pwr > 100)
  {
    shell_printf(intf, "invalid: should be between 0 and 100\r\n");
    return;
  }

  heater_fan_power(pwr);
  shell_printf(intf, "set power to %d\r\n", pwr);
}

static void
shell_command_settings(ShellIntf* intf, int argc, const char** argv)
{
  settings_t* s = settings_get();

  shell_printf(intf, "\r\n");

  shell_printf(intf, "0. glow plug on duration for start %ld sec\r\n", s->glow_plug_on_duration_for_start / 1000);
  shell_printf(intf, "1. oil pump priming duratuin       %ld sec\r\n", s->oil_pump_priming_duration / 1000);
  shell_printf(intf, "2. glow plug on duration for stop  %ld sec\r\n", s->glow_plug_on_duration_for_stop / 1000);
  shell_printf(intf, "3. cooling down period             %ld sec\r\n", s->cooling_down_period / 1000);
  shell_printf(intf, "4. start-up fan power              %d %%\r\n", s->startup_fan_power);
  shell_printf(intf, "5. stop fan power                  %d %%\r\n", s->stop_fan_power);
  shell_printf(intf, "6. glow plug PWM frequency         %d Hz\r\n", s->glow_plug_pwm_freq);
  shell_printf(intf, "7. glow plug PWM duty              %d %%\r\n", s->glow_plug_pwm_duty);

  shell_printf(intf, "8. oil pump startup frequency      %.1f Hz\r\n", s->oil_pump_startup_freq);
  shell_printf(intf, "9. oil pump pulse length           %d ms\r\n", s->oil_pump_pulse_length);

  shell_printf(intf, "\r\n");
  for(uint8_t i = 0; i < MAX_OIL_PUMP_FAN_STEPS; i++)
  {
    shell_printf(intf, "step %d, oil pump freq %.1f Hz, Fan %d%%\r\n", 
          i,
          s->steps[i].pump_freq,
          s->steps[i].fan_pwr);
  }
}

static void
shell_command_mod(ShellIntf* intf, int argc, const char** argv)
{
  uint8_t       num;
  uint32_t      iv;
  float         fv;
  settings_t*   s = settings_get();

  shell_printf(intf, "\r\n");

  if(argc != 3)
  {
    shell_printf(intf, "invalid %s <setting number> <value>\r\n", argv[0]);
    return;
  }

  num = atoi(argv[1]);
  if(num < 0 || num > 9)
  {
    shell_printf(intf, "invalid: setting number should be between 0 and 10\r\n");
    return;
  }

  switch(num)
  {
  case 0: // glow plug on duration for start
    iv = atol(argv[2]);
    if(iv < 30 || iv > 150)
    {
      shell_printf(intf, "invalid: value should be between  30 and 150\r\n");
      return;
    }
    s->glow_plug_on_duration_for_start = iv * 1000;
    break;

  case 1: // oil pump priming duration
    iv = atol(argv[2]);
    if(iv < 5 || iv > 60)
    {
      shell_printf(intf, "invalid: value should be between  5 and 60\r\n");
      return;
    }
    s->oil_pump_priming_duration = iv * 1000;
    break;

  case 2: // glow plug on duration for stop
    iv = atol(argv[2]);
    if(iv < 20 || iv > 60)
    {
      shell_printf(intf, "invalid: value should be between  20 and 60\r\n");
      return;
    }
    s->glow_plug_on_duration_for_start = iv * 1000;
    break;

  case 3: // cooling down period
    iv = atol(argv[2]);
    if(iv < 60 || iv > 180)
    {
      shell_printf(intf, "invalid: value should be between  60 and 180\r\n");
      return;
    }
    s->cooling_down_period = iv * 1000;
    break;

  case 4: // start up fan power
    iv = atol(argv[2]);
    if(iv < 10 || iv > 100)
    {
      shell_printf(intf, "invalid: value should be between  10 and 100\r\n");
      return;
    }
    s->startup_fan_power = iv;
    break;

  case 5: // stop fan power
    iv = atol(argv[2]);
    if(iv < 10 || iv > 100)
    {
      shell_printf(intf, "invalid: value should be between  10 and 100\r\n");
      return;
    }
    s->stop_fan_power = iv;
    break;

  case 6: // glow plug PWM frequency
    iv = atol(argv[2]);
    if(iv < 2 || iv > 20)
    {
      shell_printf(intf, "invalid: value should be between 2 and 20\r\n");
      return;
    }
    s->glow_plug_pwm_freq = iv;
    break;

  case 7: // glow plug PWM duty
    iv = atol(argv[2]);
    if(iv < 5 || iv > 95)
    {
      shell_printf(intf, "invalid: value should be between 5 and 95\r\n");
      return;
    }
    s->glow_plug_pwm_duty = iv;
    break;

  case 8: // oil pump frequency
    fv = atof(argv[2]);
    if(fcompare(fv, OIL_PUMP_MIN_FREQ) < 0 || fcompare(fv, OIL_PUMP_MAX_FREQ) > 0)
    {
      shell_printf(intf, "invalid: value should be between 0.8 and 5.0\r\n");
      return;
    }
    s->oil_pump_startup_freq = fv;
    break;

  case 9: // oil pump pulse length
    iv = atol(argv[2]);
    if(iv < 10 || iv > 100)
    {
      shell_printf(intf, "invalid: value should be between  10 and 100\r\n");
      return;
    }
    s->oil_pump_pulse_length = iv;
    break;

  default:
    return;
  }

  shell_printf(intf, "done changing setting #%d. Be sure to save\r\n", num);
}

static void
shell_command_save(ShellIntf* intf, int argc, const char** argv)
{
  shell_printf(intf, "\r\nsaving changes to EEPROM...\r\n");
  settings_update();
  shell_printf(intf, "done saving changes to EEPROM...\r\n");
}

static void
shell_command_reset(ShellIntf* intf, int argc, const char** argv)
{
  shell_printf(intf, "\r\nresettinging settings...\r\n");
  settings_reset();
  shell_printf(intf, "done resetting settings...\r\n");
}

static void
shell_command_step(ShellIntf* intf, int argc, const char** argv)
{
  float   freq;
  uint8_t pwr;
  uint8_t step;
  settings_t*   s = settings_get();

  shell_printf(intf, "\r\n");

  if(argc != 4)
  {
    shell_printf(intf, "invalid %s <step number> <pump freq> <fan pwr>\r\n", argv[0]);
    return;
  }

  step = atoi(argv[1]);
  if(step < 0 || step >= MAX_OIL_PUMP_FAN_STEPS)
  {
    shell_printf(intf, "invalid step number\r\n");
    return;
  }

  freq = atof(argv[2]);
  if(fcompare(freq, OIL_PUMP_MIN_FREQ) < 0 || fcompare(freq, OIL_PUMP_MAX_FREQ) > 0)
  {
    shell_printf(intf, "Invalid frequency\r\n");
    return;
  }

  pwr = atoi(argv[3]);
  if(pwr < 0 || pwr > 100)
  {
    shell_printf(intf, "Invalid power\r\n");
    return;
  }

  s->steps[step].pump_freq = freq;
  s->steps[step].fan_pwr = pwr;

  shell_printf(intf, "changed settings of step #%d\r\n", step);
}

static void
shell_command_set(ShellIntf* intf, int argc, const char** argv)
{
  uint8_t step;

  shell_printf(intf, "\r\n");

  if(argc != 2)
  {
    shell_printf(intf, "invalid %s <step number>\r\n", argv[0]);
    return;
  }

  step = atoi(argv[1]);
  if(step < 0 || step >= MAX_OIL_PUMP_FAN_STEPS)
  {
    shell_printf(intf, "invalid step number\r\n");
    return;
  }

  heater_set_step(step);

  shell_printf(intf, "changed current step to %d\r\n", step);
}





////////////////////////////////////////////////////////////////////////////////
//
// shell core
//
////////////////////////////////////////////////////////////////////////////////
static void
shell_execute_command(ShellIntf* intf, char* cmd)
{
  static const char*    argv[SHELL_COMMAND_MAX_ARGS];
  int                   argc = 0;
  size_t                i;
  char                  *s, *t;

  while((s = strtok_r(argc  == 0 ? cmd : NULL, " \t", &t)) != NULL)
  {
    if(argc >= SHELL_COMMAND_MAX_ARGS)
    {
      shell_printf(intf, "\r\nError: too many arguments\r\n");
      return;
    }
    argv[argc++] = s;
  }

  if(argc == 0)
  {
    return;
  }

  for(i = 0; i < sizeof(_commands)/sizeof(ShellCommand); i++)
  {
    if(strcmp(_commands[i].command, argv[0]) == 0)
    {
      shell_printf(intf, "\r\nExecuting %s\r\n", argv[0]);
      _commands[i].handler(intf, argc, argv);
      return;
    }
  }
  shell_printf(intf, "%s", "\r\nUnknown Command: ");
  shell_printf(intf, "%s", argv[0]);
  shell_printf(intf, "%s", "\r\n");
}


void
shell_printf(ShellIntf* intf, const char* fmt, ...)
{
  va_list   args;
  int       len;

  va_start(args, fmt);
  len = vsnprintf(_print_buffer, SHELL_MAX_COLUMNS_PER_LINE, fmt, args);
  va_end(args);

  do
  {
  } while(intf->put_tx_data(intf, (uint8_t*)_print_buffer, len) == false);
}


////////////////////////////////////////////////////////////////////////////////
//
// public interface
//
////////////////////////////////////////////////////////////////////////////////
void
shell_init(void)
{
  shell_if_usb_init();
}

void
shell_start(void)
{
  ShellIntf* intf;

  list_for_each_entry(intf, &_shell_intf_list, lh)
  {
    shell_printf(intf, "%s", _welcome);
    shell_prompt(intf);
  }
}


void
shell_if_register(ShellIntf* intf)
{
  list_add_tail(&intf->lh, &_shell_intf_list);
}

void
shell_handle_rx(ShellIntf* intf)
{
  uint8_t   b;

  while(1)
  {
    if(intf->get_rx_data(intf, &b) == false)
    {
      return;
    }

    if(b != '\r' && intf->cmd_buffer_ndx < SHELL_MAX_COMMAND_LEN)
    {
      if(b == '\b' || b == 0x7f)
      {
        if(intf->cmd_buffer_ndx > 0)
        {
          shell_printf(intf, "%c%c%c", b, 0x20, b);
          intf->cmd_buffer_ndx--;
        }
      }
      else
      {
        shell_printf(intf, "%c", b);
        intf->cmd_buffer[intf->cmd_buffer_ndx++] = b;
      }
    }
    else if(b == '\r')
    {
      intf->cmd_buffer[intf->cmd_buffer_ndx++] = '\0';

      shell_execute_command(intf, (char*)intf->cmd_buffer);

      intf->cmd_buffer_ndx = 0;
      shell_prompt(intf);
    }
  }
}

struct list_head*
shell_get_intf_list(void)
{
  return &_shell_intf_list;
}
