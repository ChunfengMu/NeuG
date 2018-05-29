#ifndef  __ADC_H__
#define  __ADC_H__

extern uint32_t adc_buf[64];

int adc_init (void);
void adc_start (void);
void adc_start_conversion (int offset, int count);
int adc_wait_completion (void);
void adc_stop (void);

#endif
