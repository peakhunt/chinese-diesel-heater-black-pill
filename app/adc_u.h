#ifndef __ACD_DEF_H__
#define __ACD_DEF_H__

typedef uint16_t adcsample_t;
typedef uint8_t adc_channels_num_t;

typedef void (*adc_callback_t)(adc_channels_num_t chnl, adcsample_t sample);

extern void adc_init(void);
extern void adc_start_conversion(adc_channels_num_t chnl, adc_callback_t callback);

#endif //!__ACD_DEF_H__
