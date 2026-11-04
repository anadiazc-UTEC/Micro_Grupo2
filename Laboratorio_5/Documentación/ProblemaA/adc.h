#ifndef ADC_H_
#define ADC_H_
#include <stdint.h>

// Inicializar ADC
void adc_init(void);
// Leer datos adc
uint16_t adc_read(uint8_t channel);
#endif