#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_flash.h"
#include "stm32f4xx_hal_flash_ex.h"

#include "settings.h"
#include "heater.h"
#include "glow_plug.h"
#include "oil_pump.h"

#define SETTINGS_MAGIC          0x1122334f

//
//
// sector 1. 16KB
// looks like this is the best option we have.
// the penalty we have to pay is we gotta
// modify linker script
//
//
#define SETTINGS_SECTOR         1
#define SETTINGS_ADDRESS        (0x08004000)


static settings_t   _settings;

static inline uint16_t
crc16_ccitt(uint16_t crc, unsigned char a)
{
  crc ^= (uint16_t)a << 8;

  for (int ii = 0; ii < 8; ++ii) {
    if (crc & 0x8000) {
      crc = (crc << 1) ^ 0x1021;
    } else {
      crc = crc << 1;
    }
  }
  return crc;
}

static inline uint16_t
calcCRC(uint16_t crc, const void *data, uint32_t length)
{
  const uint8_t *p = (const uint8_t *)data;
  const uint8_t *pend = p + length;

  for (; p != pend; p++) {
    crc = crc16_ccitt(crc, *p);
  }
  return crc;
}

static uint16_t
settings_calc_crc(settings_t* s)
{
  uint16_t    crc;

  crc = calcCRC(0, (void*)&s->glow_plug_on_duration_for_start, sizeof(settings_t) - 8);

  return crc;
}


static bool
settings_check_valid(void)
{
  uint16_t    crc;

  if(_settings.magic != SETTINGS_MAGIC)
  {
    return false;
  }

  crc = settings_calc_crc(&_settings);

  if(crc != _settings.crc)
  {
    return false;
  }
  return true;
}

static void
erase_program_settings_to_flash(void)
{
  FLASH_EraseInitTypeDef    eraseStruct;
  uint32_t                  pageErr;
  uint32_t                  *data_ptr;
  uint32_t                  addr;

  eraseStruct.TypeErase     = FLASH_TYPEERASE_SECTORS;
  eraseStruct.Banks         = FLASH_BANK_1;
  eraseStruct.Sector        = SETTINGS_SECTOR;
  eraseStruct.NbSectors     = 1;

  HAL_FLASH_Unlock();

  HAL_FLASHEx_Erase(&eraseStruct, &pageErr);

  data_ptr  = (uint32_t*)&_settings;
  addr      = SETTINGS_ADDRESS;

  while(addr < (SETTINGS_ADDRESS + sizeof(settings_t)))
  {
    if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, *data_ptr) != HAL_OK)
    {
      while(1); // infinite loop to indicate something went wrong
    }
    addr += 4;
    data_ptr++;
  }
  HAL_FLASH_Lock();
}

static volatile void*
memcpy_v(volatile void *restrict dest, const volatile void *restrict src, size_t n)
{
  const volatile unsigned char *src_c = src;
  volatile unsigned char *dest_c      = dest;

  while (n > 0) {
    n--;
    dest_c[n] = src_c[n];
  }
  return  dest;
}

void
settings_init(void)
{
  //
  // load from flash memory
  // to avoid all the volatile whatever
  //
  volatile settings_t* s = (volatile settings_t*)SETTINGS_ADDRESS;

  memcpy_v(&_settings, s, sizeof(settings_t));

  if(settings_check_valid() == false)
  {
    // invalid settings
    // reset everything to default
    settings_reset();
  }
}

void
settings_reset(void)
{
  _settings.magic                             = SETTINGS_MAGIC;
  _settings.glow_plug_on_duration_for_start   = HEATER_HEATING_GLOW_PLUG_BEFORE_OIL_PUMP_PRIMING;
  _settings.oil_pump_priming_duration         = HEATER_OIL_PUMP_PRIMING_BEFORE_GLOW_PLUG_TURN_OFF;
  _settings.glow_plug_on_duration_for_stop    = HEATER_HEATING_GLOW_PLUG_BEFORE_COOLING_DOWN;
  _settings.cooling_down_period               = HEATER_COOLING_DOWN_AFTER_GLOW_PLUG_TURN_OFF;

  _settings.startup_fan_power                 = HEATER_STARTUP_FAN_POWER;
  _settings.stop_fan_power                    = HEATER_STOP_FAN_POWER;

  _settings.glow_plug_pwm_freq                = GLOW_PLUG_PWM_FREQ;
  _settings.glow_plug_pwm_duty                = GLOW_PLUG_PWM_DUTY;

  _settings.oil_pump_startup_freq             = OIL_PUMP_DEF_FREQ;
  _settings.oil_pump_pulse_length             = OIL_PUMP_PULSE_LENGTH;

  // optimal settings I found after some experiment
  _settings.steps[0].pump_freq  = 1.0f;
  _settings.steps[0].fan_pwr    = 45;

  _settings.steps[1].pump_freq  = 1.8f;
  _settings.steps[1].fan_pwr    = 50;

  _settings.steps[2].pump_freq  = 2.5f;
  _settings.steps[2].fan_pwr    = 60;

  _settings.steps[3].pump_freq  = 3.5f;
  _settings.steps[3].fan_pwr    = 70;

  _settings.steps[4].pump_freq  = 4.5f;
  _settings.steps[4].fan_pwr    = 75;

  settings_update();
}

void
settings_update(void)
{
  uint32_t    old_primask;

  _settings.crc = settings_calc_crc(&_settings);

  old_primask = __get_PRIMASK();
  __disable_irq();

  erase_program_settings_to_flash();

  __set_PRIMASK(old_primask);
}

settings_t*
settings_get(void)
{
  return &_settings;
}
