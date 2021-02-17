#ifndef __ADC_READER_DEF_H__
#define __ADC_READER_DEF_H__

#include "app_common.h"
#include "adc_u.h"

typedef enum
{
  adc_channel_0,
  adc_channel_1,
  adc_channel_2,
} adc_channel_t;

#define ADC_MAX_CHANNELS    (adc_channel_2 + 1)

typedef void (*adc_listener)(adc_channel_t ch, adcsample_t sample, void* arg);

extern void adc_reader_init(void);
extern void adc_reader_listen(adc_channel_t ch, adc_listener listener, void* arg);
extern adcsample_t adc_get(adc_channel_t ch);
extern float adc_get_volt(adc_channel_t ch);

#endif /* !__ADC_READER_DEF_H__ */
